package main

/*
 * NOTE!
 * Although this works for everything else in the Steam Server response, it does
 * not provide accurate results for *only* Player count. That requires usage of
 * the Steamworks SDK (C++).
 */

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
)

// Struct to hold the API response
type SteamServerResponse struct {
	Response struct {
		// Address    string `json:"addr"`
		// GamePort   int    `json:"gameport"`
		// SteamID    string `json:"steamid"`
		// ServerName string `json:"name"`
		// AppID      int    `json:"appid"`
		// GameDir    string `json:"gamedir"`
		// Version    string `json:"version"`
		// Game       string `json:"product"`
		// Region     int    `json:"region"`
		Players int `json:"players"`
		// MaxPlayers int    `json:"max_players"`
		// Bots       int    `json:"bots"`
		// Secure     bool   `json:"secure"`
		// Dedicated  bool   `json:"dedicated"`
		// OS         string `json:"os"`
	} `json:"response"`
}

// func GetServerDetails(url string) error {
//   // Send GET request
// }

func main() {
	steamApiKey := "10AC4516C46E62219EE29DAE015F7431"
	steamServerIP := "127.0.0.1"
	steamServerPort := "27015"
	// appID := "427410"

	url := fmt.Sprintf("https://api.steampowered.com/IGameServersService/GetServerList/v1/?key=%s&filter=addr%%5c%s:%s",
		steamApiKey, steamServerIP, steamServerPort)

	// Send GET request
	resp, err := http.Get(url)
	if err != nil {
		log.Fatalf("Error making request: %v", err)
	}
	defer resp.Body.Close()

	// Print the status code to check if the response was successful
	fmt.Printf("[%d] ", resp.StatusCode)
	if resp.StatusCode != http.StatusOK {
		log.Fatalf("\nReceived error status: %d", resp.StatusCode)
	}

	// Read the response body
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		log.Fatalf("Error reading response body: %v", err)
	}

	// Parse JSON response
	var serverInfo SteamServerResponse
	if err := json.Unmarshal(body, &serverInfo); err != nil {
		log.Fatalf("Error parsing JSON: %v", err)
	}

	// Print the server details
	fmt.Printf("Players online: %d\n", serverInfo.Response.Players)
}
