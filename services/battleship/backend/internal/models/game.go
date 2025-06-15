package models

import (
	"battleship/internal/db"
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"math"
	"sync"
	"time"
)

type Position struct {
	X int `json:"x"`
	Y int `json:"y"`
}

type Ship struct {
	Start Position
	End   Position
	Hits  int
	Size  int
}

type Player struct {
	ID       string
	Name     string
	Treasure string
}

type Game struct {
	ID        string
	Player    Player
	Opponent  Player
	Status    string
	CreatedAt time.Time
}

type Shot struct {
	X   int  `json:"x"`
	Y   int  `json:"y"`
	Hit bool `json:"hit"`
}

type GameManager struct {
	mu sync.RWMutex
}

func NewGameManager() *GameManager {
	return &GameManager{}
}

func (gm *GameManager) CreateGame(playerID, opponentID, opponentName string) (*Game, error) {
	gm.mu.Lock()
	defer gm.mu.Unlock()

	if playerID == opponentID {
		return nil, fmt.Errorf("cannot create game with yourself")
	}

	gameID := generateGameID()

	_, err := db.DB.Exec(`
		INSERT INTO games (id, player_id, opponent_id, status, created_at)
		VALUES (?, ?, ?, ?, ?)
	`, gameID, playerID, opponentID, "active", time.Now())
	if err != nil {
		return nil, fmt.Errorf("failed to create game: %v", err)
	}

	ships := autoPlaceShips()

	for _, ship := range ships {
		_, err := db.DB.Exec(`
			INSERT INTO ships (game_id, start_x, start_y, end_x, end_y, size)
			VALUES (?, ?, ?, ?, ?, ?)
		`, gameID, ship.Start.X, ship.Start.Y, ship.End.X, ship.End.Y, ship.Size)
		if err != nil {
			return nil, fmt.Errorf("failed to place ships: %v", err)
		}
	}

	go func() {
		time.Sleep(5 * time.Minute)
		if err := gm.CleanupGame(gameID); err != nil {
		}
	}()

	return &Game{
		ID:        gameID,
		CreatedAt: time.Now(),
	}, nil
}

func (gm *GameManager) MakeShot(gameID string, pos Position) (bool, string, error) {
	var game Game
	var opponentID string
	var playerID string
	var createdAt time.Time
	err := db.DB.QueryRow(`
		SELECT id, player_id, opponent_id, status, created_at
		FROM games
		WHERE id = ?
	`, gameID).Scan(&game.ID, &playerID, &opponentID, &game.Status, &createdAt)
	if err != nil {
		return false, "", fmt.Errorf("game not found: %v", err)
	}

	game.Player.ID = playerID

	if time.Since(createdAt) > 5*time.Minute {
		go func() {
			if err := gm.CleanupGame(gameID); err != nil {
			}
		}()
		return false, "", fmt.Errorf("game has expired")
	}

	if game.Status != "active" {
		return false, "", fmt.Errorf("game is not active")
	}

	var shotExists bool
	err = db.DB.QueryRow(`
		SELECT EXISTS(
			SELECT 1 FROM shots 
			WHERE game_id = ? AND x = ? AND y = ?
		)
	`, gameID, pos.X, pos.Y).Scan(&shotExists)
	if err != nil {
		return false, "", fmt.Errorf("failed to check shot: %v", err)
	}
	if shotExists {
		return false, "", fmt.Errorf("you already shot at this position")
	}

	tx, err := db.DB.Begin()
	if err != nil {
		return false, "", fmt.Errorf("failed to start transaction: %v", err)
	}
	defer tx.Rollback()

	var hit bool
	var hitShipID int
	var hitShipSize int
	var hitShipHits int

	rows, err := tx.Query(`
		SELECT id, size, hits FROM ships
		WHERE game_id = ? AND
		? BETWEEN start_x AND end_x AND
		? BETWEEN start_y AND end_y
	`, gameID, pos.X, pos.Y)
	if err != nil {
		return false, "", fmt.Errorf("failed to check hit: %v", err)
	}
	defer rows.Close()

	if rows.Next() {
		hit = true
		err = rows.Scan(&hitShipID, &hitShipSize, &hitShipHits)
		if err != nil {
			return false, "", fmt.Errorf("failed to process hit: %v", err)
		}
	}

	_, err = tx.Exec(`
		INSERT INTO shots (game_id, x, y, hit)
		VALUES (?, ?, ?, ?)
	`, gameID, pos.X, pos.Y, hit)
	if err != nil {
		return false, "", fmt.Errorf("failed to record shot: %v", err)
	}

	if hit {
		newHits := hitShipHits + 1
		_, err = tx.Exec(`
			UPDATE ships 
			SET hits = ?
			WHERE id = ?
		`, newHits, hitShipID)
		if err != nil {
			return false, "", fmt.Errorf("failed to update ship hits: %v", err)
		}

		if newHits >= hitShipSize {

			var totalShips int
			var sunkShips int
			err = tx.QueryRow(`
				SELECT COUNT(*), 
				(SELECT COUNT(*) FROM ships WHERE game_id = ? AND hits >= size)
				FROM ships WHERE game_id = ?
			`, gameID, gameID).Scan(&totalShips, &sunkShips)
			if err != nil {
				return false, "", fmt.Errorf("failed to check game status: %v", err)
			}

			if totalShips == sunkShips {
				_, err = tx.Exec(`
					UPDATE games
					SET status = 'completed'
					WHERE id = ?
				`, gameID)
				if err != nil {
					return false, "", fmt.Errorf("failed to update game status: %v", err)
				}

				_, err = tx.Exec(`
					UPDATE users
					SET wins = wins + 1
					WHERE id = ?
				`, playerID)
				if err != nil {
					return false, "", fmt.Errorf("failed to update player wins: %v", err)
				}

				var opponent Player
				err = tx.QueryRow(`
					SELECT id, username, treasure
					FROM users
					WHERE id = ?
				`, opponentID).Scan(&opponent.ID, &opponent.Name, &opponent.Treasure)
				if err != nil {
					return false, "", fmt.Errorf("opponent not found: %v", err)
				}

				if err := tx.Commit(); err != nil {
					return false, "", fmt.Errorf("failed to commit transaction: %v", err)
				}

				go func() {
					time.Sleep(5 * time.Second)
					if err := gm.CleanupGame(gameID); err != nil {
					}
				}()

				return true, "win: " + opponent.Treasure, nil
			}

			if err := tx.Commit(); err != nil {
				return false, "", fmt.Errorf("failed to commit transaction: %v", err)
			}

			return true, "hit", nil
		}

		if err := tx.Commit(); err != nil {
			return false, "", fmt.Errorf("failed to commit transaction: %v", err)
		}

		return true, "hit", nil
	}

	_, err = tx.Exec(`
		UPDATE games
		SET status = 'completed'
		WHERE id = ?
	`, gameID)
	if err != nil {
		return false, "", fmt.Errorf("failed to update game status: %v", err)
	}

	_, err = tx.Exec(`
		UPDATE users
		SET losses = losses + 1
		WHERE id = ?
	`, playerID)
	if err != nil {
		return false, "", fmt.Errorf("failed to update player losses: %v", err)
	}

	if err := tx.Commit(); err != nil {
		return false, "", fmt.Errorf("failed to commit transaction: %v", err)
	}

	go func() {
		time.Sleep(5 * time.Second)
		if err := gm.CleanupGame(gameID); err != nil {
		}
	}()

	return false, "miss", nil
}

func (gm *GameManager) GetActiveGames(userID string) ([]string, error) {
	gm.mu.RLock()
	defer gm.mu.RUnlock()

	rows, err := db.DB.Query(`
		SELECT game.id
		FROM games game
		JOIN users user ON game.opponent_id = user.id
		WHERE game.status = 'active' 
		AND user.id = ?
		AND game.created_at > datetime('now', '-5 minutes')
	`, userID)

	if err != nil {
		return nil, fmt.Errorf("failed to get active games: %v", err)
	}
	defer rows.Close()

	var gameIDs []string
	for rows.Next() {
		var gameID string
		err := rows.Scan(&gameID)
		if err != nil {
			return nil, fmt.Errorf("failed to scan game ID: %v", err)
		}

		gameIDs = append(gameIDs, gameID)
	}

	return gameIDs, nil
}

func (gm *GameManager) GetGame(gameID string) (*Game, error) {
	gm.mu.RLock()
	defer gm.mu.RUnlock()

	var game Game
	var opponentID string
	var createdAt time.Time
	err := db.DB.QueryRow(`
		SELECT id, player_id, opponent_id, status, created_at
		FROM games
		WHERE id = ?
	`, gameID).Scan(&game.ID, &game.Player.ID, &opponentID, &game.Status, &createdAt)
	if err != nil {
		return nil, fmt.Errorf("game not found")
	}

	game.CreatedAt = createdAt

	if time.Since(createdAt) > 5*time.Minute {
		go func() {
			if err := gm.CleanupGame(gameID); err != nil {
			}
		}()
		return nil, fmt.Errorf("game has expired")
	}

	var opponent Player
	err = db.DB.QueryRow(`
		SELECT id, username, treasure
		FROM users
		WHERE id = ?
	`, opponentID).Scan(&opponent.ID, &opponent.Name, &opponent.Treasure)
	if err != nil {
		return nil, fmt.Errorf("opponent not found: %v", err)
	}
	game.Opponent = opponent

	var player Player
	err = db.DB.QueryRow(`
		SELECT id, username
		FROM users
		WHERE id = ?
	`, game.Player.ID).Scan(&player.ID, &player.Name)
	if err != nil {
		return nil, fmt.Errorf("player not found: %v", err)
	}
	game.Player = player

	return &game, nil
}

func (gm *GameManager) GetShips(game_id string) ([]Ship, error) {
	var createdAt time.Time
	err := db.DB.QueryRow(`
		SELECT created_at
		FROM games
		WHERE id = ?
	`, game_id).Scan(&createdAt)
	if err != nil {
		return nil, fmt.Errorf("game not found: %v", err)
	}

	if time.Since(createdAt) > 5*time.Minute {
		go func() {
			if err := gm.CleanupGame(game_id); err != nil {
			}
		}()
		return nil, fmt.Errorf("game has expired")
	}

	rows, err := db.DB.Query(`
		SELECT start_x, start_y, end_x, end_y, size
		FROM ships
		WHERE game_id = ?
	`, game_id)
	if err != nil {
		return nil, fmt.Errorf("failed to get ships: %v", err)
	}
	defer rows.Close()

	var ships []Ship

	for rows.Next() {
		var ship Ship
		err := rows.Scan(&ship.Start.X, &ship.Start.Y, &ship.End.X, &ship.End.Y, &ship.Size)
		if err != nil {
			return nil, fmt.Errorf("failed to scan ship: %v", err)
		}
		ships = append(ships, ship)
	}

	return ships, nil
}

func (gm *GameManager) GetShots(gameID string) ([]Shot, error) {
	gm.mu.RLock()
	defer gm.mu.RUnlock()

	var createdAt time.Time
	err := db.DB.QueryRow(`
		SELECT created_at
		FROM games
		WHERE id = ?
	`, gameID).Scan(&createdAt)
	if err != nil {
		return nil, fmt.Errorf("game not found: %v", err)
	}

	if time.Since(createdAt) > 5*time.Minute {
		go func() {
			if err := gm.CleanupGame(gameID); err != nil {
			}
		}()
		return nil, fmt.Errorf("game has expired")
	}

	rows, err := db.DB.Query(`
		SELECT x, y, hit
		FROM shots
		WHERE game_id = ?
		ORDER BY id ASC
	`, gameID)
	if err != nil {
		return nil, fmt.Errorf("failed to get shots: %v", err)
	}
	defer rows.Close()

	var shots []Shot
	for rows.Next() {
		var shot Shot
		err := rows.Scan(&shot.X, &shot.Y, &shot.Hit)
		if err != nil {
			return nil, fmt.Errorf("failed to scan shot: %v", err)
		}
		shots = append(shots, shot)
	}

	return shots, nil
}

func generateGameID() string {
	b := make([]byte, 16)
	rand.Read(b)
	return hex.EncodeToString(b)
}

func autoPlaceShips() []Ship {
	shipTypes := []struct {
		Size  int
		Count int
	}{
		{Size: 4, Count: 1},
		{Size: 3, Count: 2},
		{Size: 2, Count: 3},
		{Size: 1, Count: 4},
	}

	var result []Ship

	for _, shipType := range shipTypes {
		for count := 0; count < shipType.Count; count++ {
			for {
				startX := randInt(0, 9)
				startY := randInt(0, 9)
				horizontal := randBool()

				endX := startX
				endY := startY
				if horizontal {
					endX += shipType.Size - 1
				} else {
					endY += shipType.Size - 1
				}

				if endX >= 10 || endY >= 10 {
					continue
				}

				ship := Ship{
					Start: Position{X: startX, Y: startY},
					End:   Position{X: endX, Y: endY},
					Size:  shipType.Size,
				}

				overlaps := false
				for j := range result {
					if shipsOverlap(ship, result[j]) {
						overlaps = true
						break
					}
				}

				if !overlaps {
					result = append(result, ship)
					break
				}
			}
		}
	}

	return result
}

func randInt(min, max int) int {
	b := make([]byte, 4)
	rand.Read(b)
	return min + int(float64(max-min+1)*float64(uint32(b[0])|uint32(b[1])<<8|uint32(b[2])<<16|uint32(b[3])<<24)/float64(math.MaxUint32))
}

func randBool() bool {
	b := make([]byte, 1)
	rand.Read(b)
	return b[0]&1 == 0
}

func shipsOverlap(ship1, ship2 Ship) bool {
	for x := ship1.Start.X; x <= ship1.End.X; x++ {
		for y := ship1.Start.Y; y <= ship1.End.Y; y++ {
			if x >= ship2.Start.X && x <= ship2.End.X &&
				y >= ship2.Start.Y && y <= ship2.End.Y {
				return true
			}
		}
	}

	for x := ship1.Start.X - 1; x <= ship1.End.X+1; x++ {
		for y := ship1.Start.Y - 1; y <= ship1.End.Y+1; y++ {
			if x < 0 || x > 9 || y < 0 || y > 9 {
				continue
			}

			if x >= ship2.Start.X && x <= ship2.End.X &&
				y >= ship2.Start.Y && y <= ship2.End.Y {
				return true
			}
		}
	}

	return false
}

func isHit(pos Position, ship Ship) bool {
	if ship.Start.X == ship.End.X {
		return pos.X == ship.Start.X &&
			pos.Y >= min(ship.Start.Y, ship.End.Y) &&
			pos.Y <= max(ship.Start.Y, ship.End.Y)
	}

	return pos.Y == ship.Start.Y &&
		pos.X >= min(ship.Start.X, ship.End.X) &&
		pos.X <= max(ship.Start.X, ship.End.X)
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}

func (gm *GameManager) CleanupGame(gameID string) error {

	_, err := db.DB.Exec("DELETE FROM shots WHERE game_id = ?", gameID)
	if err != nil {
		return fmt.Errorf("failed to delete shots: %v", err)
	}

	_, err = db.DB.Exec("DELETE FROM ships WHERE game_id = ?", gameID)
	if err != nil {
		return fmt.Errorf("failed to delete ships: %v", err)
	}

	_, err = db.DB.Exec("DELETE FROM games WHERE id = ?", gameID)
	if err != nil {
		return fmt.Errorf("failed to delete game: %v", err)
	}

	return nil
}
