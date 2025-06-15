package models

import (
	"battleship/internal/db"
	"fmt"
	"sync"
)

type LeaderboardEntry struct {
	Username string `json:"username"`
	Wins     int    `json:"wins"`
	Losses   int    `json:"losses"`
	Rank     int    `json:"rank"`
}

type Leaderboard struct {
	mu sync.RWMutex
}

func NewLeaderboard() *Leaderboard {
	return &Leaderboard{}
}

func (lb *Leaderboard) GetLeaderboard(page int) ([]LeaderboardEntry, int, error) {
	lb.mu.RLock()
	defer lb.mu.RUnlock()

	if page < 1 {
		page = 1
	}

	offset := (page - 1) * 10
	limit := 10

	var totalUsers int
	err := db.DB.QueryRow("SELECT COUNT(*) FROM users").Scan(&totalUsers)
	if err != nil {
		return nil, 0, fmt.Errorf("failed to count users: %v", err)
	}

	rows, err := db.DB.Query(`
		SELECT username, wins, losses
		FROM users
		ORDER BY wins * 3 + losses * -2 DESC
		LIMIT ? OFFSET ?
	`, limit, offset)
	if err != nil {
		return nil, 0, fmt.Errorf("failed to fetch leaderboard: %v", err)
	}
	defer rows.Close()

	var entries []LeaderboardEntry
	for rows.Next() {
		var entry LeaderboardEntry
		err := rows.Scan(&entry.Username, &entry.Wins, &entry.Losses)
		if err != nil {
			return nil, 0, fmt.Errorf("failed to scan leaderboard entry: %v", err)
		}

		entry.Rank = offset + len(entries) + 1
		entries = append(entries, entry)
	}

	totalPages := (totalUsers + limit - 1) / limit

	return entries, totalPages, nil
}
