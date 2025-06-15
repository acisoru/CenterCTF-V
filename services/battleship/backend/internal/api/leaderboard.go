package api

import (
	"battleship/internal/models"
	"encoding/json"
	"net/http"
	"strconv"
)

type LeaderboardHandler struct {
	leaderboard *models.Leaderboard
}

type LeaderboardResponse struct {
	Entries    []models.LeaderboardEntry `json:"entries"`
	Page       int                       `json:"page"`
	TotalPages int                       `json:"totalPages"`
}

func NewLeaderboardHandler(leaderboard *models.Leaderboard) *LeaderboardHandler {
	return &LeaderboardHandler{
		leaderboard: leaderboard,
	}
}

func (h *LeaderboardHandler) GetLeaderboard(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	_, ok := r.Context().Value("user").(*models.User)
	if !ok {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	pageStr := r.URL.Query().Get("page")
	page := 1
	if pageStr != "" {
		var err error
		page, err = strconv.Atoi(pageStr)
		if err != nil || page < 1 {
			page = 1
		}
	}

	entries, totalPages, err := h.leaderboard.GetLeaderboard(page)
	if err != nil {
		http.Error(w, "Failed to fetch leaderboard", http.StatusInternalServerError)
		return
	}

	response := LeaderboardResponse{
		Entries:    entries,
		Page:       page,
		TotalPages: totalPages,
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(response)
}
