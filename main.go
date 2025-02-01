package main

/*
#cgo CXXFLAGS: -std=c++11
#cgo LDFLAGS: -L${SRCDIR}/lib -lA2SQuery -lstdc++
#cgo CFLAGS: -I${SRCDIR}/lib
#include "a2s_query_handler_wrapper.h"
*/
import "C"

import (
	"fmt"
	"sync"
	"time"
	"unsafe"
)

func getServerRules(ip string, port int) {
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

	fmt.Println(mapGoRules)
}

// [31 JAN 2025 @ 3:39 PM GMT-8] Forgive me for the shitshow that is about to ensue
// [31 JAN 2025 @ 7:53 PM GMT-8] Turns out this worked out pretty nicely
func main() {
	var wg sync.WaitGroup
	wg.Add(1)

	go func() { // Creates a goroutine for concurrency
		defer wg.Done() // Mark the task done when the goroutine finishes
		for true {      // Make the condition the bot running(?)
			getServerRules("47.152.10.229", 27015)
			time.Sleep(time.Minute * 1)
		}

		// for range time.Tick(time.Minute * 1) {
		// 	getServerRules("47.152.10.229", 27015)

		// 	// In here, update the bot status based on players and (server) response
		// 	// - this is meant to decide between Online and DnD
		// }
	}()

	fmt.Println("This method is running at the same time.")

	wg.Wait() // Block the main goroutine for an exit that never comes
}
