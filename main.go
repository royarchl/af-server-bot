package main

import (
	"log"
	"os"

	// This is an alias for a local directory
	bot "github.com/royarchl/af-server-bot/bot"

	"github.com/joho/godotenv"
)

func main() {
	err := godotenv.Load()
	if err != nil {
		log.Fatal(".env file could not be loaded")
	}

	bot.BotToken = os.Getenv("BOT_TOKEN")
	bot.Run()
}
