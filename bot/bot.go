package bot

/*
#cgo CXXFLAGS: -std=c++11
#cgo LDFLAGS: -L${SRCDIR}/../lib -lA2SQuery -lstdc++
#cgo CFLAGS: -I${SRCDIR}/../lib
#include "a2s_query_handler_wrapper.h"
*/
import "C"

import (
	"fmt"
	"log"
	"os"
	"os/signal"
	"strconv"
	"time"

	"unsafe"

	"github.com/bwmarrin/discordgo"
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

func getServerRules(ip string, port int) map[string]string {
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

	return mapGoRules
}

func Run() {
	session, err := discordgo.New("Bot " + BotToken)
	CheckNilErr(err)

	err = session.Open()
	CheckNilErr(err)
	defer session.Close()

	log.Println("Bot running... (Press Ctrl-C to exit)")

	ipAddr := os.Getenv("IP_ADDR")
	port, err := strconv.Atoi(os.Getenv("PORT"))
	CheckNilErr(err)

	go func() {
		log.Println("Beginning server query loop...")

		now := time.Now()

		minutesPast := now.Minute() % 5
		sleepDuration := time.Minute * time.Duration(5-minutesPast)

		for true {
			tempMap := getServerRules(ipAddr, port)

			statusMessage := fmt.Sprintf("Online - Active Players: %s/6", tempMap["PlayerCount_i"])
			session.UpdateCustomStatus(statusMessage)

			time.Sleep(sleepDuration)
		}
	}()

	// keep bot running while no OS interruption
	channel := make(chan os.Signal, 1)
	signal.Notify(channel, os.Interrupt)
	<-channel

	log.Println("Shutting down gracefully.")
}
