package models

import (
	"battleship/internal/db"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"os"
	"sync"
	"time"

	"github.com/golang-jwt/jwt/v5"
)

type User struct {
	ID       string
	Username string
	Password string
	Avatar   string
	Treasure string
	Wins     int
	Losses   int
}

type UserManager struct {
	mu          sync.RWMutex
	gameManager *GameManager
}

func NewUserManager() *UserManager {
	return &UserManager{}
}

func (um *UserManager) SetGameManager(gm *GameManager) {
	um.gameManager = gm
}

func (um *UserManager) Register(username, password string, userTreasure ...string) (*User, error) {
	um.mu.Lock()
	defer um.mu.Unlock()

	var exists bool
	err := db.DB.QueryRow("SELECT EXISTS(SELECT 1 FROM users WHERE username = ?)", username).Scan(&exists)
	if err != nil {
		return nil, fmt.Errorf("database error: %v", err)
	}
	if exists {
		return nil, fmt.Errorf("username already exists")
	}

	userID := generateUserID(username)

	var treasure string
	if len(userTreasure) > 0 && userTreasure[0] != "" {
		treasure = userTreasure[0]
	} else {
		treasure = ""
	}

	hashedPassword := HashPassword(password)

	_, err = db.DB.Exec(`
		INSERT INTO users (id, username, password, treasure)
		VALUES (?, ?, ?, ?)
	`, userID, username, hashedPassword, treasure)
	if err != nil {
		return nil, fmt.Errorf("failed to create user: %v", err)
	}

	return &User{
		ID:       userID,
		Username: username,
		Password: hashedPassword,
		Treasure: treasure,
	}, nil
}

func (um *UserManager) Login(username, password string) (*User, error) {
	um.mu.RLock()
	defer um.mu.RUnlock()

	var exists bool
	err := db.DB.QueryRow("SELECT EXISTS(SELECT 1 FROM users WHERE username = ?)", username).Scan(&exists)
	if err != nil {
		return nil, fmt.Errorf("database error: %v", err)
	}
	if !exists {
		return nil, fmt.Errorf("invalid credentials: user not found")
	}

	var user User

	err = db.DB.QueryRow(`
		SELECT id, username, password, avatar, treasure
		FROM users
		WHERE username = ?
	`, username).Scan(&user.ID, &user.Username, &user.Password, &user.Avatar, &user.Treasure)
	if err != nil {
		return nil, fmt.Errorf("database error: %v", err)
	}

	hashedPassword := HashPassword(password)
	if user.Password != hashedPassword {
		return nil, fmt.Errorf("invalid credentials: wrong password")
	}

	return &user, nil
}

func (um *UserManager) GetUserByID(id string) (*User, error) {
	um.mu.RLock()
	defer um.mu.RUnlock()

	var user User

	err := db.DB.QueryRow(`
		SELECT id, username, password, avatar, treasure, wins, losses
		FROM users
		WHERE id = ?
	`, id).Scan(&user.ID, &user.Username, &user.Password, &user.Avatar, &user.Treasure, &user.Wins, &user.Losses)
	if err != nil {
		return nil, fmt.Errorf("user not found")
	}

	return &user, nil
}

func (um *UserManager) GetOpponentByUsername(username string) (*User, error) {
	um.mu.RLock()
	defer um.mu.RUnlock()

	var user User

	err := db.DB.QueryRow(`
		SELECT id, username, password, treasure
		FROM users
		WHERE username = ?
	`, username).Scan(&user.ID, &user.Username, &user.Password, &user.Treasure)
	if err != nil {
		return nil, fmt.Errorf("opponent not found")
	}

	return &user, nil
}

func (um *UserManager) UpdateUser(user *User) error {
	um.mu.Lock()
	defer um.mu.Unlock()

	_, err := db.DB.Exec(`
		UPDATE users
		SET avatar = ?, username = ?, password = ?, treasure = ?
		WHERE id = ?
	`, user.Avatar, user.Username, user.Password, user.Treasure, user.ID)
	if err != nil {
		return fmt.Errorf("failed to update user: %v", err)
	}

	return nil
}

func (um *UserManager) ValidateToken(tokenString string) (*User, error) {
	um.mu.RLock()
	defer um.mu.RUnlock()

	token, err := jwt.Parse(tokenString, func(token *jwt.Token) (interface{}, error) {
		if _, ok := token.Method.(*jwt.SigningMethodHMAC); !ok {
			return nil, fmt.Errorf("unexpected signing method: %v", token.Header["alg"])
		}
		secret := os.Getenv("JWT_SECRET")
		return []byte(secret), nil
	}, jwt.WithValidMethods([]string{"HS256"}))

	if err != nil {
		return nil, fmt.Errorf("failed to parse token: %v", err)
	}

	if claims, ok := token.Claims.(jwt.MapClaims); ok && token.Valid {
		expClaim, ok := claims["exp"].(float64)
		if !ok {
			return nil, fmt.Errorf("invalid expiration claim")
		}

		exp := time.Unix(int64(expClaim), 0)
		if time.Now().After(exp) {
			return nil, fmt.Errorf("token expired")
		}

		userID, ok := claims["user_id"].(string)
		if !ok {
			return nil, fmt.Errorf("invalid user_id claim")
		}

		return um.GetUserByID(userID)
	}

	return nil, fmt.Errorf("invalid token")
}

func generateUserID(username string) string {
	hash := sha256.Sum256([]byte(username + time.Now().String()))
	return hex.EncodeToString(hash[:])
}

func HashPassword(password string) string {
	hash := sha256.Sum256([]byte(password))
	return hex.EncodeToString(hash[:])
}

func (um *UserManager) GetRandomOpponent(excludeID string) (*User, error) {
	um.mu.RLock()
	defer um.mu.RUnlock()

	var count int
	err := db.DB.QueryRow("SELECT COUNT(*) FROM users WHERE id != ?", excludeID).Scan(&count)
	if err != nil {
		return nil, fmt.Errorf("no opponents available: %v", err)
	}

	if count == 0 {
		return nil, fmt.Errorf("no opponents available")
	}

	var user User

	err = db.DB.QueryRow(`
		SELECT id, username, password, treasure
		FROM users
		WHERE id != ?
		ORDER BY RANDOM()
		LIMIT 1
	`, excludeID).Scan(&user.ID, &user.Username, &user.Password, &user.Treasure)
	if err != nil {
		return nil, fmt.Errorf("no opponents available")
	}

	return &user, nil
}
