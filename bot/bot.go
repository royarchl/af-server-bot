package bot

import (
	"fmt"
	"io"
	"io/fs"
	"log"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"regexp"
	"strings"
	"time"

	"github.com/bwmarrin/discordgo"
	"github.com/nxadm/tail"
	"github.com/royarchl/af-server-bot/bot/commands"
	"github.com/royarchl/af-server-bot/bot/shared"
)

// [31 JAN 2025 @ 3:39 PM GMT-8] Forgive me for the shitshow that is about to ensue
// [31 JAN 2025 @ 7:53 PM GMT-8] Turns out this worked out pretty nicely
// [01 FEB 2025 @ 5:11 PM GMT-8] This came out very clean, ngl

var BotToken string

func Run() {
	session, err := discordgo.New("Bot " + BotToken)
	shared.CheckNilErr(err)

	session.AddHandler(commands.SetCommands)

	err = session.Open()
	shared.CheckNilErr(err)
	defer session.Close()

	commands.RegisterCommands(session)

	log.Println("Bot running... (Press Ctrl-C to exit)")

	shared.SetEnvVariables()

	go func() {
		shared.InitServerQueryLoop(session)
	}()

	// https://pkg.go.dev/github.com/nxadm/tail#section-readme
	// https://pkg.go.dev/io#SectionReader.Seek
	go func() {
		backupsEnabled := false

		sourceDirAbs := os.Getenv("SRVR_SAVE_DIR")
		tailedFile := os.Getenv("LOG_FILE")
		tailedFilePath := filepath.Join(sourceDirAbs, tailedFile)

		t, _ := tail.TailFile(tailedFilePath, tail.Config{
			Follow: true,
			ReOpen: true,
			Location: &tail.SeekInfo{
				Offset: 0,
				Whence: io.SeekEnd,
			},
		})

		// Update to do something that isn't panicking
		// if err != nil {
		// 	panic(err)
		// }

		NO_PLAYERS := "0"
		for line := range t.Lines {
			if strings.Contains(line.Text, "PlayerCount") {
				re := regexp.MustCompile(`PlayerCount.*\((\d+)\)`)
				matches := re.FindStringSubmatch(line.Text)

				if matches[1] != NO_PLAYERS {
					backupsEnabled = true
					session.ChannelMessageSendEmbed(shared.BotChannelID, &discordgo.MessageEmbed{
						Type:        discordgo.EmbedTypeRich,
						Title:       "Backups enabled.",
						Description: "Player presence detected. Server snapshots will begin being saved.\n\n-# Use the command `/tag-snapshot` to mark a backup for manual restore.",
						Color:       0x00FF00,
					})
				} else {
					backupsEnabled = false
					session.ChannelMessageSendEmbed(shared.BotChannelID, &discordgo.MessageEmbed{
						Type:        discordgo.EmbedTypeRich,
						Title:       "Backups suspended.",
						Description: "No players detected. Saves will be ignored to preserve system resources.",
						Color:       0x808080,
					})
				}
			}

			if backupsEnabled == true {
				if !strings.Contains(line.Text, "autosaving") {
					continue
				}

				// STEPS
				// 0. Assign the required variables
				// 1. Make the archive/ directory, in case it doesn't exist
				// 2. Grab all the files from the Saved/ directory
				// 3. Generate the backup name using timestamp and server info
				// 4. Create the archive with 7z in the proper directory
				// 5. Send the message to [REDACTED]
				// 6. Delete the uploaded file

				// [ 0 ]
				saveDirRel := "./archives"
				archiveEncPwd := os.Getenv("ENC_PASSWD")
				backupsChannelID := os.Getenv("BKP_CHANNEL_ID")

				// [ 1 ]
				err := os.MkdirAll(saveDirRel, 0755)
				shared.CheckNilErr(err)

				// [ 2 ]
				var files []string
				err = filepath.WalkDir(sourceDirAbs, func(path string, d fs.DirEntry, err error) error {
					if err != nil {
						fmt.Printf("Error accessing path %q: %v\n", path, err)
						return err // stops recurse
					}

					if path == sourceDirAbs {
						return nil // SKIP
					}

					relPath, _ := filepath.Rel(sourceDirAbs, path)
					if filepath.Dir(relPath) == "." {
						files = append(files, path)
					}

					return nil
				})

				// [ 3 ]
				timestamp := time.Now().Format("2006.01.02-15.04.05")
				archiveName := fmt.Sprintf("Backup_%s_%s.locked.7z", timestamp, shared.ServerSettings["ServerName_s"])
				archivePath := filepath.Join(saveDirRel, archiveName)

				// [ 4 ]
				args := []string{
					"a",                  // add files to archive
					"-t7z",               // 7z archive format
					"-m0=lzma2",          // compression method
					"-mx=9",              // max compression
					"-ms=on",             // solid compression
					"-md=64m",            // 64 MB dictionary size
					"-ssp",               // prevent modifying Last Access Time
					"-stl",               // set archive timestamp to last modified file
					"-p" + archiveEncPwd, // set encryption password
					"-mhe=on",            // encrypt file content AND names
					archivePath,
				}
				args = append(args, files...)

				cmd := exec.Command("7z", args...)
				_, err = cmd.CombinedOutput()
				shared.CheckNilErr(err)

				// [ 5 ]
				file, err := os.Open(archivePath)
				shared.CheckNilErr(err)
				defer file.Close()

				backupFile := &discordgo.File{
					Name:   archiveName,
					Reader: file,
				}

				m, err := session.ChannelMessageSendComplex(backupsChannelID, &discordgo.MessageSend{
					Content: "** **\n-# This is an automated backup of the game state.",
					Files:   []*discordgo.File{backupFile},
				})
				shared.SnapshotChannelID = m.ChannelID
				shared.LastSnapshotMessageID = m.ID

				// [ 6 ]
				if err == nil {
					os.Remove(archivePath)
				}
			}
		}
	}()

	// keep bot running while no OS interruption
	channel := make(chan os.Signal, 1)
	signal.Notify(channel, os.Interrupt)
	<-channel

	log.Println("Shutting down gracefully.")
}
