package db

import (
	"database/sql"
	"os"
	"path/filepath"

	_ "github.com/mattn/go-sqlite3"
)

var DB *sql.DB

func InitDB() error {
	dataDir := "/app/data"
	if _, err := os.Stat(dataDir); os.IsNotExist(err) {
		if err := os.MkdirAll(dataDir, 0755); err != nil {
			dataDir = "data"
			if err := os.MkdirAll(dataDir, 0755); err != nil {
				return err
			}
		}
	}

	dbPath := filepath.Join(dataDir, "battleship.db")

	var err error
	DB, err = sql.Open("sqlite3", dbPath)
	if err != nil {
		return err
	}

	if err := DB.Ping(); err != nil {
		return err
	}

	if err := createTables(); err != nil {
		return err
	}

	var count int
	err = DB.QueryRow("SELECT COUNT(*) FROM users").Scan(&count)
	if err != nil {
	}

	return nil
}

func createTables() error {
	_, err := DB.Exec(`
		CREATE TABLE IF NOT EXISTS users (
			id TEXT PRIMARY KEY,
			username TEXT UNIQUE NOT NULL,
			password TEXT NOT NULL,
			avatar TEXT,
			treasure TEXT NOT NULL,
			wins INTEGER NOT NULL DEFAULT 0,
			losses INTEGER NOT NULL DEFAULT 0,
			created_at DATETIME DEFAULT CURRENT_TIMESTAMP
		)
	`)
	if err != nil {
		return err
	}

	_, err = DB.Exec(`
		CREATE TABLE IF NOT EXISTS games (
			id TEXT PRIMARY KEY,
			player_id TEXT NOT NULL,
			opponent_id TEXT NOT NULL,
			status TEXT NOT NULL,
			created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
			FOREIGN KEY (player_id) REFERENCES users(id),
			FOREIGN KEY (opponent_id) REFERENCES users(id)
		)
	`)
	if err != nil {
		return err
	}

	_, err = DB.Exec(`
		CREATE TABLE IF NOT EXISTS ships (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			game_id TEXT NOT NULL,
			start_x INTEGER NOT NULL,
			start_y INTEGER NOT NULL,
			end_x INTEGER NOT NULL,
			end_y INTEGER NOT NULL,
			hits INTEGER NOT NULL DEFAULT 0,
			size INTEGER NOT NULL,
			FOREIGN KEY (game_id) REFERENCES games(id)
		)
	`)
	if err != nil {
		return err
	}

	_, err = DB.Exec(`
		CREATE TABLE IF NOT EXISTS shots (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			game_id TEXT NOT NULL,
			x INTEGER NOT NULL,
			y INTEGER NOT NULL,
			hit BOOLEAN NOT NULL,
			FOREIGN KEY (game_id) REFERENCES games(id)
		)
	`)
	if err != nil {
		return err
	}

	return nil
}
