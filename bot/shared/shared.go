package shared

/*
#cgo CXXFLAGS: -std=c++11
#cgo LDFLAGS: -L${SRCDIR}/../../lib -lA2SQuery -lstdc++
#cgo CFLAGS: -I${SRCDIR}/../../lib
#include "a2s_query_handler_wrapper.h"
*/
import "C"

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"strconv"
	"time"
	"unsafe"

	"github.com/bwmarrin/discordgo"
)

var (
	ServerSettings        map[string]string
	IPAddr                string
	IPPort                int
	BotChannelID          string
	SnapshotChannelID     string
	LastSnapshotMessageID string
	BackupsOn             bool
)

func CheckNilErr(e error) {
	if e != nil {
		log.Fatalf("Error message: %v", e)
	}
}

type GoPayload struct {
	RulesMap  map[string]string
	ErrorCode int
	ErrorMsg  string
}

func SetEnvVariables() {
	var err error

	IPAddr = os.Getenv("IP_ADDR")
	IPPort, err = strconv.Atoi(os.Getenv("PORT"))
	CheckNilErr(err)
	BotChannelID = os.Getenv("CHANNEL_ID")
}

func GetServerRules(ip string, port int) *GoPayload {
	payload := C.a2s_query_server_rules(C.CString(ip), C.uint16_t(port))

	pPayload := (*C.Payload)(payload)

	rules := pPayload.m_pMapRules
	size := pPayload.m_unRulesSize

	mapGoRules := make(map[string]string)

	for i := C.size_t(0); i < size; i++ {
		rulePtr := (*C.ServerRule)(unsafe.Pointer(uintptr(unsafe.Pointer(rules)) + uintptr(i)*unsafe.Sizeof(*rules)))

		goRule := C.GoString(rulePtr.m_pchRule)
		goValue := C.GoString(rulePtr.m_pchValue)

		mapGoRules[goRule] = goValue
	}

	success := C.a2s_free_rules_memory(payload)
	if !success {
		log.Fatalln("Memory wasn't freed")
	}

	return &GoPayload{
		RulesMap:  mapGoRules,
		ErrorCode: int(pPayload.m_nErrorCode),
		ErrorMsg:  C.GoString(pPayload.m_szErrorMsg),
	}
}

func GetServerVersion() int {
	type ServerResponse struct {
		Data map[string]struct {
			ChangeNumber int `json:"_change_number"`
		} `json:"data"`
	}

	resp, err := http.Get("https://api.steamcmd.net/v1/info/2857200")
	CheckNilErr(err)
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	CheckNilErr(err)

	var serverVersion ServerResponse
	if err := json.Unmarshal(body, &serverVersion); err != nil {
		log.Fatalf("Error parsing JSON: %v", err)
	}

	return serverVersion.Data["2857200"].ChangeNumber
}

func InitServerQueryLoop(session *discordgo.Session) {
	log.Println("Beginning server query loop...")

	prevPlayerCount := -1
	maxPlayers := os.Getenv("MAX_PLAYERS")

	serverVersion := GetServerVersion()

	errorMap := map[int]bool{
		4001: false, // No packets found
		4002: false, // Incomplete rule-value pair
		5001: false, // Failure to create socket
		5002: false, // Failue to apply socket timeout
		5003: false, // Response timeout
		5004: false, // Failure to receive data
		5005: false, // Failure to send initial query
		5006: false, // Server response too small
		5007: false, // Failure to send challenge query
		5008: false, // Challenge response too small
	}

	for {
		payload := GetServerRules(IPAddr, IPPort)

		if payload.ErrorCode != 0 {
			if errorMap[payload.ErrorCode] == false {
				// This check might not even be necessary because it seems everything returns as 5003
				if payload.ErrorCode == 5003 {

					if GetServerVersion() != serverVersion {
						activity := &discordgo.Activity{
							Name:  "template",
							Type:  discordgo.ActivityTypeCustom,
							State: "Server update available",
						}

						statusData := discordgo.UpdateStatusData{
							Status:     string(discordgo.StatusDoNotDisturb),
							Activities: []*discordgo.Activity{activity},
						}

						session.UpdateStatusComplex(statusData)
						session.ChannelMessageSendEmbed(BotChannelID, &discordgo.MessageEmbed{
							Type:        discordgo.EmbedTypeRich,
							Title:       "Server Error [E5004]",
							Description: "Server is out-of-date. **An update is available.**",
							Timestamp:   time.Now().Format(time.RFC3339),
							Color:       0xFF0000,
						})
					} else {
						activity := &discordgo.Activity{
							Name:  "template",
							Type:  discordgo.ActivityTypeCustom,
							State: fmt.Sprintf("Offline (err. %d)", payload.ErrorCode),
						}

						statusData := discordgo.UpdateStatusData{
							Status:     string(discordgo.StatusIdle),
							Activities: []*discordgo.Activity{activity},
						}

						session.UpdateStatusComplex(statusData)

						session.ChannelMessageSendEmbed(BotChannelID, &discordgo.MessageEmbed{
							Type:        discordgo.EmbedTypeRich,
							Title:       fmt.Sprintf("Server Error [E%d]", payload.ErrorCode),
							Description: payload.ErrorMsg,
							Timestamp:   time.Now().Format(time.RFC3339),
							Color:       0xFF0000,
						})
					}
				}
				errorMap[payload.ErrorCode] = true
			}
		} else { // SUCCESS - everything working properly
			if errorMap[5003] == true {
				serverVersion = GetServerVersion()
			}

			for key := range errorMap {
				errorMap[key] = false
			}

			ServerSettings = payload.RulesMap

			currentPlayerCount, err := strconv.Atoi(ServerSettings["PlayerCount_i"])
			CheckNilErr(err)

			if currentPlayerCount != prevPlayerCount {
				if BackupsOn == true {
					session.UpdateStatusComplex(discordgo.UpdateStatusData{
						Activities: []*discordgo.Activity{
							{
								Name: fmt.Sprintf(" - Active Players: %d/%s", currentPlayerCount, maxPlayers),
								Type: discordgo.ActivityTypeStreaming,
								URL:  "https://twitch.tv/idk",
							},
						},
						Status: "online",
					})
				} else {
					statusMessage := fmt.Sprintf("Online - Active Players: %d/%s", currentPlayerCount, maxPlayers)
					session.UpdateCustomStatus(statusMessage)
				}
			}
		}

		now := time.Now()

		nextFiveMinute := now.Truncate(5 * time.Minute).Add(5 * time.Minute)
		sleepDuration := nextFiveMinute.Sub(now)

		time.Sleep(sleepDuration)
	}
}
