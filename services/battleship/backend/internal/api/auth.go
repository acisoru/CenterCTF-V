package api

import (
	"battleship/internal/models"
	"encoding/json"
	"net/http"
	"os"
	"time"

	"github.com/golang-jwt/jwt/v5"
)

type AuthHandler struct {
	userManager *models.UserManager
}

func NewAuthHandler(userManager *models.UserManager) *AuthHandler {
	return &AuthHandler{
		userManager: userManager,
	}
}

type RegisterRequest struct {
	Username string `json:"username"`
	Password string `json:"password"`
	Treasure string `json:"treasure,omitempty"`
}

type LoginRequest struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

type LoginResponse struct {
	Username string `json:"username"`
	Avatar   string `json:"avatar"`
}

func (h *AuthHandler) Register(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req RegisterRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.Username == "" || req.Password == "" {
		http.Error(w, "Username and password are required", http.StatusBadRequest)
		return
	}

	user, err := h.userManager.Register(req.Username, req.Password, req.Treasure)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	token, err := generateUserJWT(user)
	if err != nil {
		http.Error(w, "Failed to generate token", http.StatusInternalServerError)
		return
	}

	user.Avatar = "https://api.dicebear.com/9.x/croodles/svg?seed=" + user.ID + "&size=64&backgroundColor=b6e3f4,c0aede,d1d4f9,ffd5dc,ffdfbf&beardProbability=50&mustacheProbability=50&top=variant23,variant24,variant11,variant18,variant09"

	filename, err := LoadAvatar(user.Avatar, user.ID, r.Cookies())
	if err != nil {
	} else if filename != "" {
		user.Avatar = filename
	}

	err = h.userManager.UpdateUser(user)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	userResp := LoginResponse{
		Username: user.Username,
		Avatar:   user.Avatar,
	}

	setAuthCookie(w, token)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"success": true,
		"user":    userResp,
	})
}

func (h *AuthHandler) Login(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req LoginRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	user, err := h.userManager.Login(req.Username, req.Password)
	if err != nil {
		http.Error(w, err.Error(), http.StatusUnauthorized)
		return
	}

	token, err := generateUserJWT(user)
	if err != nil {
		http.Error(w, "Failed to generate token", http.StatusInternalServerError)
		return
	}

	setAuthCookie(w, token)

	avatar, err := LoadAvatar("https://api.dicebear.com/9.x/croodles/svg?seed="+user.ID+"&size=64&backgroundColor=b6e3f4,c0aede,d1d4f9,ffd5dc,ffdfbf&beardProbability=50&mustacheProbability=50&top=variant23,variant24,variant11,variant18,variant09", user.ID, nil)
	if err != nil {
	} else {
		user.Avatar = avatar
	}

	err = h.userManager.UpdateUser(user)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	userResp := LoginResponse{
		Username: user.Username,
		Avatar:   user.Avatar,
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"success": true,
		"user":    userResp,
	})
}

func (h *AuthHandler) Logout(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	http.SetCookie(w, &http.Cookie{
		Name:     "auth_token",
		Value:    "",
		Path:     "/",
		HttpOnly: true,
		Secure:   false,
		MaxAge:   -1,
		SameSite: http.SameSiteLaxMode,
	})

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"success": true,
	})
}

func generateUserJWT(user *models.User) (string, error) {
	claims := jwt.MapClaims{
		"user_id":  user.ID,
		"username": user.Username,
		"exp":      time.Now().Add(time.Hour * 72).Unix(),
		"iat":      time.Now().Unix(),
		"nbf":      time.Now().Unix(),
		"iss":      "battleship-game",
	}

	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)

	token.Header["typ"] = "JWT"

	secret := os.Getenv("JWT_SECRET")
	return token.SignedString([]byte(secret))
}

func setAuthCookie(w http.ResponseWriter, token string) {
	http.SetCookie(w, &http.Cookie{
		Name:     "auth_token",
		Value:    token,
		Path:     "/",
		HttpOnly: true,
		Secure:   false,
		MaxAge:   3600 * 24 * 3,
		SameSite: http.SameSiteLaxMode,
	})
}
