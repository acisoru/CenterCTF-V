package server

import (
	"battleship/internal/api"
	"battleship/internal/db"
	"battleship/internal/models"
	"context"
	"log"
	"net/http"
)

type Server struct {
	userManager        *models.UserManager
	gameManager        *models.GameManager
	leaderboard        *models.Leaderboard
	authHandler        *api.AuthHandler
	gameHandler        *api.GameHandler
	profileHandler     *api.ProfileHandler
	leaderboardHandler *api.LeaderboardHandler
}

func NewServer() (*Server, error) {
	if err := db.InitDB(); err != nil {
		return nil, err
	}

	userManager := models.NewUserManager()
	gameManager := models.NewGameManager()
	leaderboard := models.NewLeaderboard()

	userManager.SetGameManager(gameManager)

	authHandler := api.NewAuthHandler(userManager)
	profileHandler := api.NewProfileHandler(userManager, gameManager)
	gameHandler := api.NewGameHandler(gameManager, userManager)
	leaderboardHandler := api.NewLeaderboardHandler(leaderboard)

	return &Server{
		userManager:        userManager,
		gameManager:        gameManager,
		leaderboard:        leaderboard,
		authHandler:        authHandler,
		gameHandler:        gameHandler,
		profileHandler:     profileHandler,
		leaderboardHandler: leaderboardHandler,
	}, nil
}

func (s *Server) Start(addr string) error {
	mux := http.NewServeMux()

	mux.HandleFunc("/api/auth/register", s.authHandler.Register)
	mux.HandleFunc("/api/auth/login", s.authHandler.Login)
	mux.HandleFunc("/api/auth/logout", s.authHandler.Logout)

	mux.HandleFunc("/api/profile/get-user-info", s.withAuth(s.profileHandler.GetUserInfo))
	mux.HandleFunc("/api/profile/get-user-treasure", s.withAuth(s.profileHandler.GetUserTreasure))
	mux.HandleFunc("/api/profile/get-active-games", s.withAuth(s.profileHandler.GetActiveGames))
	mux.HandleFunc("/api/profile/update-profile", s.withAuth(s.profileHandler.UpdateProfile))

	mux.HandleFunc("/api/game/create-game", s.withAuth(s.gameHandler.CreateGame))
	mux.HandleFunc("/api/game/make-shot", s.withAuth(s.withGame(s.gameHandler.MakeShot)))
	mux.HandleFunc("/api/game/get-game-info", s.withAuth(s.withGame(s.gameHandler.GetGameInfo)))

	mux.HandleFunc("/api/leaderboard/get-leaderboard", s.withAuth(s.leaderboardHandler.GetLeaderboard))

	handler := s.corsMiddleware(mux)

	log.Printf("Server starting on %s", addr)
	return http.ListenAndServe(addr, handler)
}

func (s *Server) withAuth(handler http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "http://localhost:8081")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization, Accept")
		w.Header().Set("Access-Control-Allow-Credentials", "true")

		for _, cookie := range r.Cookies() {
			valPreview := cookie.Value
			if len(valPreview) > 10 {
				valPreview = valPreview[:5] + "..."
			}
		}

		cookie, err := r.Cookie("auth_token")
		if err != nil {
			http.Error(w, "Unauthorized - no auth cookie", http.StatusUnauthorized)
			return
		}

		token := cookie.Value
		if token == "" {
			http.Error(w, "Unauthorized - empty token", http.StatusUnauthorized)
			return
		}

		user, err := s.userManager.ValidateToken(token)
		if err != nil {
			http.Error(w, "Invalid token: "+err.Error(), http.StatusUnauthorized)
			return
		}

		r = r.WithContext(context.WithValue(r.Context(), "user", user))
		handler(w, r)
	}
}

func (s *Server) withGame(handler http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		cookie, err := r.Cookie("game_id")
		if err != nil {
			http.Error(w, "No active game found - missing game_id cookie", http.StatusBadRequest)
			return
		}

		gameID := cookie.Value
		if gameID == "" {
			http.Error(w, "No active game found - empty game_id", http.StatusBadRequest)
			return
		}

		game, err := s.gameManager.GetGame(gameID)
		if err != nil {
			api.ClearGameCookie(w)

			http.Error(w, "Invalid game: "+err.Error(), http.StatusBadRequest)
			return
		}

		if game.Status != "active" {
			api.ClearGameCookie(w)

			http.Error(w, "Game is not active", http.StatusBadRequest)
			return
		}

		r = r.WithContext(context.WithValue(r.Context(), "game", game))
		handler(w, r)
	}
}

func (s *Server) corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "http://localhost:8081")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization, Accept")
		w.Header().Set("Access-Control-Allow-Credentials", "true")

		if r.Method == "OPTIONS" {
			w.WriteHeader(http.StatusNoContent)
			return
		}

		next.ServeHTTP(w, r)
	})
}
