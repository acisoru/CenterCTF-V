package api

import (
	"battleship/internal/models"
	"encoding/json"
	"net"
	"net/http"
	"strings"
)

type GameHandler struct {
	gameManager *models.GameManager
	userManager *models.UserManager
}

func NewGameHandler(gameManager *models.GameManager, userManager *models.UserManager) *GameHandler {
	return &GameHandler{
		gameManager: gameManager,
		userManager: userManager,
	}
}

type CreateGameRequest struct {
	OpponentUsername string `json:"opponent_username,omitempty"`
}

type MakeShotRequest struct {
	Position models.Position `json:"position"`
}

func ClearGameCookie(w http.ResponseWriter) {
	http.SetCookie(w, &http.Cookie{
		Name:     "game_id",
		Value:    "",
		Path:     "/",
		HttpOnly: false,
		MaxAge:   -1,
		SameSite: http.SameSiteLaxMode,
	})
}

func (h *GameHandler) CreateGame(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	user, ok := r.Context().Value("user").(*models.User)
	if !ok {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	var req CreateGameRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	var opponent *models.User
	var err error

	if req.OpponentUsername != "" {
		opponent, err = h.userManager.GetOpponentByUsername(req.OpponentUsername)
		if err != nil {
			http.Error(w, "Opponent not found", http.StatusNotFound)
			return
		}
	} else {
		opponent, err = h.userManager.GetRandomOpponent(user.ID)
		if err != nil {
			http.Error(w, "No opponents available", http.StatusNotFound)
			return
		}
	}

	newGame, err := h.gameManager.CreateGame(
		user.ID,
		opponent.ID,
		opponent.Username,
	)

	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	gameCookie := &http.Cookie{
		Name:     "game_id",
		Value:    newGame.ID,
		Path:     "/",
		HttpOnly: false,
		Secure:   false,
		MaxAge:   3600 * 24,
		SameSite: http.SameSiteLaxMode,
		Domain:   "",
	}
	http.SetCookie(w, gameCookie)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"game_id":  newGame.ID,
		"username": opponent.Username,
	})
}

func (h *GameHandler) MakeShot(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req MakeShotRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	_, ok := r.Context().Value("user").(*models.User)
	if !ok {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	game, ok := r.Context().Value("game").(*models.Game)
	if !ok {
		http.Error(w, "No active game found", http.StatusBadRequest)
		return
	}

	hit, message, err := h.gameManager.MakeShot(game.ID, req.Position)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	if message == "miss" {
		ClearGameCookie(w)
	} else if strings.Contains(message, "win") {
		ClearGameCookie(w)
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"hit":     hit,
		"message": message,
	})
}

func (h *GameHandler) GetGameInfo(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	game, ok := r.Context().Value("game").(*models.Game)
	if !ok {
		http.Error(w, "No active game found", http.StatusBadRequest)
		return
	}

	var ships []models.Ship
	var err error
	if isLocalhost(r.RemoteAddr) {
		ships, err = h.gameManager.GetShips(game.ID)
		if err != nil {
			ships = nil
			http.Error(w, "Game not found", http.StatusNotFound)
		}
	}

	shots, err := h.gameManager.GetShots(game.ID)
	if err != nil {
		http.Error(w, "Failed to get game data", http.StatusInternalServerError)
		return
	}

	isCompleted := game.Status == "completed"

	response := map[string]interface{}{
		"game_id":       game.ID,
		"status":        game.Status,
		"is_completed":  isCompleted,
		"player_name":   game.Player.Name,
		"opponent_name": game.Opponent.Name,
		"shots":         shots,
	}

	if ships != nil {
		response["ships"] = ships
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(response)
}

func isLocalhost(addr string) bool {
	host := addr
	if h, _, err := net.SplitHostPort(addr); err == nil {
		host = h
	}

	return host == "127.0.0.1" || host == "localhost" || host == "::1"
}
