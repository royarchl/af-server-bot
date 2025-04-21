package commands

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strings"

	"github.com/bwmarrin/discordgo"
	"github.com/royarchl/af-server-bot/bot/shared"
)

var (
	dmPermission                   = false
	defaultMemberPermissions int64 = discordgo.PermissionManageServer

	commands = []*discordgo.ApplicationCommand{
		{
			Name:        "lobby",
			Description: "Queries the lobby code of the game server",
			Type:        discordgo.ChatApplicationCommand,
		},
		{
			Name:        "tag-snapshot",
			Description: "Marks a snapshot for manual restore",
			Type:        discordgo.ChatApplicationCommand,
		},
		{
			Name:        "sandbox-settings",
			Description: "Display the current settings for the game server",
			Type:        discordgo.ChatApplicationCommand,
		},
		{
			Name:        "join",
			Description: "Provides a direct link for opening game (may open browser)",
			Type:        discordgo.ChatApplicationCommand,
		},
	}

	commandHandlers = map[string]func(s *discordgo.Session, i *discordgo.InteractionCreate){
		"lobby": func(s *discordgo.Session, i *discordgo.InteractionCreate) {
			shared.ServerSettings = shared.GetServerRules(shared.IPAddr, shared.IPPort).RulesMap
			s.InteractionRespond(i.Interaction, &discordgo.InteractionResponse{
				Type: discordgo.InteractionResponseChannelMessageWithSource,
				Data: &discordgo.InteractionResponseData{
					Content: shared.ServerSettings["ShortCode_s"],
				},
			})
		},
		"tag-snapshot": func(s *discordgo.Session, i *discordgo.InteractionCreate) {
			if shared.LastSnapshotMessageID == "" {
				s.InteractionRespond(i.Interaction, &discordgo.InteractionResponse{
					Type: discordgo.InteractionResponseChannelMessageWithSource,
					Data: &discordgo.InteractionResponseData{
						Content: "There are no snapshots to store.\n-# This usually occurs when the bot was recently restarted.",
						Flags:   discordgo.MessageFlagsEphemeral,
					},
				})
			} else {
				embed := &discordgo.MessageEmbed{
					Type:        discordgo.EmbedTypeRich,
					Title:       ":pushpin: Snapshot tagged.",
					Description: fmt.Sprintf("The snapshot (https://discord.com/channels/%s/%s/%s) is marked for manual restore.", i.GuildID, shared.SnapshotChannelID, shared.LastSnapshotMessageID),
				}

				s.InteractionRespond(i.Interaction, &discordgo.InteractionResponse{
					Type: discordgo.InteractionResponseChannelMessageWithSource,
					Data: &discordgo.InteractionResponseData{
						Embeds: []*discordgo.MessageEmbed{embed},
					},
				})
				s.MessageReactionAdd(shared.SnapshotChannelID, shared.LastSnapshotMessageID, "📌")
			}
		},
		"sandbox-settings": func(s *discordgo.Session, i *discordgo.InteractionCreate) {
			settings := os.Getenv("SANDBOX_SETTINGS_PATH")

			file, err := os.Open(settings)
			shared.CheckNilErr(err)
			defer file.Close()

			var lines []string

			// https://stackoverflow.com/questions/8757389/reading-a-file-line-by-line-in-go
			scanner := bufio.NewScanner(file)
			for scanner.Scan() {
				lines = append(lines, scanner.Text())
			}

			joinedContent := strings.Join(lines, "\n")

			documentationURL := "https://github.com/DFJacob/AbioticFactorDedicatedServer/wiki/Technical-%%E2%%80%%90-Sandbox-Options"

			s.InteractionRespond(i.Interaction, &discordgo.InteractionResponse{
				Type: discordgo.InteractionResponseChannelMessageWithSource,
				Data: &discordgo.InteractionResponseData{
					Content: fmt.Sprintf("```ini\n%s\n```\n-# Refer to the [official documentation](<%s>) for setting details.", joinedContent, documentationURL),
				},
			})
		},
		"join": func(s *discordgo.Session, i *discordgo.InteractionCreate) {
			s.InteractionRespond(i.Interaction, &discordgo.InteractionResponse{
				Type: discordgo.InteractionResponseChannelMessageWithSource,
				Data: &discordgo.InteractionResponseData{
					// Content: "Huh?",
					Components: []discordgo.MessageComponent{
						discordgo.ActionsRow{
							Components: []discordgo.MessageComponent{
								discordgo.Button{
									Label: "Join Server",
									Style: discordgo.LinkButton,
									URL:   "http://127.0.0.1:8080",
									// URL:   "`steam://run/427410//+connect 47.152.10.229:27015`",
								},
							},
						},
					},
				},
			})
		},
	}
)

func SetCommands(session *discordgo.Session, interaction *discordgo.InteractionCreate) {
	if handler, ok := commandHandlers[interaction.ApplicationCommandData().Name]; ok {
		handler(session, interaction)
	}
}

func RegisterCommands(s *discordgo.Session) {
	for _, v := range commands {
		guildID := os.Getenv("GUILD_ID")
		_, err := s.ApplicationCommandCreate(s.State.User.ID, guildID, v)
		if err != nil {
			log.Panicf("Cannot create '%v' command: %v", v.Name, err)
		}
	}
}
