package bot

/*
#cgo CXXFLAGS: -std=c++11
#cgo LDFLAGS: -L${SRCDIR}/../lib -lA2SQuery -lstdc++
#cgo CFLAGS: -I${SRCDIR}/../lib
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
	"os/signal"
	"strconv"
	"time"

	"unsafe"

	"github.com/bwmarrin/discordgo"
	"github.com/royarchl/af-server-bot/bot/commands"
)

// [31 JAN 2025 @ 3:39 PM GMT-8] Forgive me for the shitshow that is about to ensue
// [31 JAN 2025 @ 7:53 PM GMT-8] Turns out this worked out pretty nicely
// [01 FEB 2025 @ 5:11 PM GMT-8] This came out very clean, ngl

var (
	BotToken string
)

func CheckNilErr(e error) {
	if e != nil {
		log.Fatalf("Error message: %v", e)
	}
}

type GoPayload struct {
	rulesMap  map[string]string
	errorCode int
	errorMsg  string
}

func getServerRules(ip string, port int) *GoPayload {
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
		rulesMap:  mapGoRules,
		errorCode: int(pPayload.m_nErrorCode),
		errorMsg:  C.GoString(pPayload.m_szErrorMsg),
	}
}

func getServerVersion() int {
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

func Run() {
	session, err := discordgo.New("Bot " + BotToken)
	CheckNilErr(err)

	session.AddHandler(commands.SetCommands)

	err = session.Open()
	CheckNilErr(err)
	defer session.Close()

	commands.RegisterCommands(session)

	log.Println("Bot running... (Press Ctrl-C to exit)")

	ipAddr := os.Getenv("IP_ADDR")
	port, err := strconv.Atoi(os.Getenv("PORT"))
	CheckNilErr(err)

	channelID := os.Getenv("CHANNEL_ID")

	go func() {
		log.Println("Beginning server query loop...")

		prevPlayerCount := -1
		maxPlayers := os.Getenv("MAX_PLAYERS")

		serverVersion := getServerVersion()

		errorMap := map[int]bool{
			5001: false, // Failure to create socket
			5002: false, // Failue to apply socket timeout
			5003: false, // Response timeout
			5004: false, // Failure to receive data
			5005: false, // Failure to send initial query
			5006: false, // Server response too small
			5007: false, // Failure to send challenge query
			5008: false, // Challenge response too small
			4001: false, // No packets found
			4002: false, // Incomplete rule-value pair
		}

		for {
			payload := getServerRules(ipAddr, port)

			if payload.errorCode != 0 {
				if errorMap[payload.errorCode] == false {
					// This check might not even be necessary because it seems everything returns as 5003
					if payload.errorCode == 5003 {

						currentTime := time.Now()
						loc, _ := time.LoadLocation("America/Los_Angeles")
						localTime := currentTime.In(loc)

						formattedTime := localTime.Format("(02 Jan 2006 - 3:04 PM MST)")

						if getServerVersion() != serverVersion {
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
							session.ChannelMessageSend(channelID, fmt.Sprintf("`%s`  **Error:** Server is out of date. An update is available.", formattedTime))
						} else {
							statusMessage := fmt.Sprintf("%d - %s", payload.errorCode, payload.errorMsg)
							activity := &discordgo.Activity{
								Name:  "template",
								Type:  discordgo.ActivityTypeCustom,
								State: statusMessage,
							}

							statusData := discordgo.UpdateStatusData{
								Status:     string(discordgo.StatusIdle),
								Activities: []*discordgo.Activity{activity},
							}

							session.UpdateStatusComplex(statusData)

							session.ChannelMessageSend(channelID, fmt.Sprintf("`%s`  **Error:** %d - %s", formattedTime, payload.errorCode, payload.errorMsg))
						}
					}
					// Send a message into bot/logs channel about the error/exception
					errorMap[payload.errorCode] = true
				}
			} else {
				// SUCCESS - everything working properly

				if errorMap[5003] == true {
					serverVersion = getServerVersion()
				}

				for key := range errorMap {
					errorMap[key] = false
				}

				commands.ServerSettings = payload.rulesMap

				currentPlayerCount, err := strconv.Atoi(commands.ServerSettings["PlayerCount_i"])
				CheckNilErr(err)

				if currentPlayerCount != prevPlayerCount {
					statusMessage := fmt.Sprintf("Online - Active Players: %d/%s", currentPlayerCount, maxPlayers)
					session.UpdateCustomStatus(statusMessage)
				}
			}

			now := time.Now()

			nextFiveMinute := now.Truncate(5 * time.Minute).Add(5 * time.Minute)
			sleepDuration := nextFiveMinute.Sub(now)

			time.Sleep(sleepDuration)
		}
	}()

	// keep bot running while no OS interruption
	channel := make(chan os.Signal, 1)
	signal.Notify(channel, os.Interrupt)
	<-channel

	log.Println("Shutting down gracefully.")
}
