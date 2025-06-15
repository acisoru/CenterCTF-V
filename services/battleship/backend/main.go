package main

import (
	"battleship/internal/server"
	"crypto/rand"
	"encoding/base64"
	"os"
)

func main() {
	key := make([]byte, 32)
	if _, err := rand.Read(key); err != nil {
	}

	jwtSecret := base64.StdEncoding.EncodeToString(key)
	os.Setenv("JWT_SECRET", jwtSecret)

	addr := os.Getenv("SERVER_ADDR")
	if addr == "" {
		addr = ":8080"
	}

	s, _ := server.NewServer()

	if err := s.Start(addr); err != nil {
	}
}
