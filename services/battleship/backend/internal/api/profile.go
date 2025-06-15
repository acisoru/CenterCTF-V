package api

import (
	"battleship/internal/models"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
)

type ProfileHandler struct {
	userManager *models.UserManager
	gameManager *models.GameManager
}

func NewProfileHandler(userManager *models.UserManager, gameManager *models.GameManager) *ProfileHandler {
	return &ProfileHandler{
		userManager: userManager,
		gameManager: gameManager,
	}
}

type ProfileUpdateRequest struct {
	Username string `json:"username,omitempty"`
	Password string `json:"password,omitempty"`
	Avatar   string `json:"avatar,omitempty"`
	Treasure string `json:"treasure,omitempty"`
}

type ProfileResponse struct {
	Username string `json:"username"`
	Avatar   string `json:"avatar"`
	Wins     int    `json:"wins"`
	Losses   int    `json:"losses"`
}

type TreasureResponse struct {
	Treasure string `json:"treasure"`
}

func (h *ProfileHandler) GetUserInfo(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	user, ok := r.Context().Value("user").(*models.User)
	if !ok {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	avatar, err := loadAvatarFile(user.Avatar)
	if err != nil {
	} else {
		user.Avatar = avatar
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(ProfileResponse{
		Username: user.Username,
		Avatar:   user.Avatar,
		Wins:     user.Wins,
		Losses:   user.Losses,
	})
}

func (h *ProfileHandler) GetUserTreasure(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	user, ok := r.Context().Value("user").(*models.User)
	if !ok {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(TreasureResponse{
		Treasure: user.Treasure,
	})
}

func (h *ProfileHandler) GetActiveGames(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	user, ok := r.Context().Value("user").(*models.User)
	if !ok {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	gameIDs, err := h.gameManager.GetActiveGames(user.ID)
	if err != nil {
		http.Error(w, "Failed to get active games", http.StatusInternalServerError)
		return
	}

	var gameInfos []map[string]interface{}

	for _, gameID := range gameIDs {
		gameInfo, err := h.fetchGameInfo(gameID, r.Cookies())
		if err != nil {
			continue
		}

		gameInfos = append(gameInfos, gameInfo)
	}

	response := map[string]interface{}{
		"games": gameInfos,
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(response)
}

func (h *ProfileHandler) fetchGameInfo(gameID string, cookies []*http.Cookie) (map[string]interface{}, error) {
	req, err := http.NewRequest("GET", "http://localhost:8080/api/game/get-game-info", nil)
	if err != nil {
		return nil, fmt.Errorf("error creating request: %v", err)
	}

	for _, cookie := range cookies {
		req.AddCookie(cookie)
	}

	gameCookie := &http.Cookie{
		Name:     "game_id",
		Value:    gameID,
		Path:     "/",
		HttpOnly: false,
		Secure:   false,
		MaxAge:   3600 * 24,
		SameSite: http.SameSiteLaxMode,
	}
	req.AddCookie(gameCookie)

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return nil, fmt.Errorf("error making request: %v", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		bodyBytes, err := io.ReadAll(resp.Body)
		if err != nil {
			return nil, fmt.Errorf("error response from server: status=%d, failed to read body: %v", resp.StatusCode, err)
		}
		resp.Body.Close()

		body := string(bodyBytes)
		return nil, fmt.Errorf("error response from server: status=%d, body=%s", resp.StatusCode, body)
	}

	var gameInfo map[string]interface{}
	if err := json.NewDecoder(resp.Body).Decode(&gameInfo); err != nil {
		return nil, fmt.Errorf("error parsing response: %v", err)
	}

	return gameInfo, nil
}

func (h *ProfileHandler) UpdateProfile(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost && r.Method != http.MethodPut {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	user, ok := r.Context().Value("user").(*models.User)
	if !ok {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	var req ProfileUpdateRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.Avatar != "" {
		if isURL(req.Avatar) {
			avatar, err := LoadAvatar(req.Avatar, user.ID, r.Cookies())
			if err != nil {
			} else if avatar != "" {
				user.Avatar = avatar
			}
		} else {
			user.Avatar = req.Avatar
		}
	}

	if req.Username != "" {
		user.Username = req.Username
	}
	if req.Password != "" {
		user.Password = models.HashPassword(req.Password)
	}
	if req.Treasure != "" {
		user.Treasure = req.Treasure
	}

	if err := h.userManager.UpdateUser(user); err != nil {
		http.Error(w, "Failed to update profile", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"success": true,
		"message": "Profile updated successfully",
	})
}

func LoadAvatar(avatarURL string, userID string, originalCookies []*http.Cookie) (string, error) {
	if avatarURL == "" {
		return "", nil
	}

	avatarsDir := "/app/avatars"
	if _, err := os.Stat(avatarsDir); os.IsNotExist(err) {
		if err := os.MkdirAll(avatarsDir, 0755); err != nil {
			avatarsDir = "avatars"
			if err := os.MkdirAll(avatarsDir, 0755); err != nil {
				return "", fmt.Errorf("failed to create avatars directory: %v", err)
			}
		}
	}

	hash := sha256.Sum256([]byte(userID))
	filename := hex.EncodeToString(hash[:]) + filepath.Ext(filepath.Base(avatarURL))
	filePath := filepath.Join(avatarsDir, filename)

	req, err := http.NewRequest("GET", avatarURL, nil)
	if err != nil {
		return "", fmt.Errorf("failed to create request: %v", err)
	}

	for _, cookie := range originalCookies {
		req.AddCookie(cookie)
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return "", fmt.Errorf("failed to fetch avatar from URL: %v", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return "", fmt.Errorf("failed to fetch avatar, status code: %d", resp.StatusCode)
	}

	file, err := os.Create(filePath)
	if err != nil {
		return "", fmt.Errorf("failed to create avatar file: %v", err)
	}
	defer file.Close()

	_, err = io.Copy(file, resp.Body)
	if err != nil {
		return "", fmt.Errorf("failed to save avatar: %v", err)
	}

	return filename, nil
}

func isURL(s string) bool {
	return len(s) >= 7 && (s[:7] == "http://" || s[:8] == "https://")
}

func loadAvatarFile(filename string) (string, error) {
	avatarsDir := "/app/avatars"
	filePath := filepath.Join(avatarsDir, filename)

	if _, err := os.Stat(avatarsDir); os.IsNotExist(err) {
		avatarsDir = "avatars"
		filePath = filepath.Join(avatarsDir, filename)
	}

	if _, err := os.Stat(filePath); os.IsNotExist(err) {
		LoadAvatar("https://api.dicebear.com/9.x/croodles/svg?seed="+filename+"&size=64&backgroundColor=b6e3f4,c0aede,d1d4f9,ffd5dc,ffdfbf&beardProbability=50&mustacheProbability=50&top=variant23,variant24,variant11,variant18,variant09", filename, nil)
	}

	content, err := os.ReadFile(filePath)
	if err != nil {
		return "", fmt.Errorf("failed to read avatar file: %v", err)
	}

	return base64.StdEncoding.EncodeToString(content), nil
}
