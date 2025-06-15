#  Battleship | easy | Golang

## Деплой

```sh
docker-compose up --build -d
```
## Уязвимости

Все уязвимости в этом таске были связаны с функционалом загрузки аватарок.

### Уязвимость 1. SSTI

Так как из трафика мы знаем, что чекер кладет флаги в сокровище пользователя, получить его можно победив данного юзера.

Так как маловероятно угадать позиции всех кораблей на поле ни разу не ошибившись, можно попробовать достать информацию о кораблях противника. 

найдем в ручках api ручку 
```go
mux.HandleFunc("/api/game/get-game-info", s.withAuth(s.withGame(s.gameHandler.GetGameInfo)))
```

Если реквест сделан от авторизованного пользователя, с указанием id игры, и если RemoteAddr - Локальный:

```go
if isLocalhost(r.RemoteAddr) {
	ships, err = h.gameManager.GetShips(game.ID)
	...
}

func isLocalhost(addr string) bool {
	return host == "127.0.0.1" || host == "localhost" || host == "::1"
}
```

Этот функционал использовался в ручке api `/api/profile/get-active-games` для получения списка активных игр и позиций кораблей в них, нереализованной на фронте.

Так как JWT токен был неизвестен, через ручку `/api/profile/get-active-games` вытащить позиции чужого игрока было нельзя, а `/api/game/get-game-info` не выдавала корабли не локалхосту.

Поэтому надо найти другую ручку которая делает реквесты с localhost, при этом сохраняя куки пользователя и игры. Такой функционал реализован в функции обновления профиля при загрузке аватарки по url. 

```go
if isURL(req.Avatar) {
	filename, err := LoadAvatar(req.Avatar, user.ID, r.Cookies())
	...
}

func LoadAvatar(avatarURL string, userID string, originalCookies []*http.Cookie) (string, error) {
	...
	req, err := http.NewRequest("GET", avatarURL, nil)
	...
}
```

Чтобы получить список кораблей необходимо:
1. Создать новую игру.
2. Отправить POST запрос на `/api/profile/update-profile` с кукой игры и кукой пользователя с data:
```json
   {'avatar': "http://localhost:8080/api/game/get-game-info"}
```
3. Получить список кораблей и выстрелить по ним
4. Получить флаг.
### Уязвимость 2. Path Traversal

В той же функции загрузки аватарки, если переданное значение - не url, бэкенд при получении аватарки пытается загрузить файл с таким названием и передать его в реквесте.

```go
else if filename != "" {
	user.Avatar = filename
}

func loadAvatarFile(filename string) (string, error) {
	avatarsDir := "/app/avatars"
	filePath := filepath.Join(avatarsDir, filename)
	if _, err := os.Stat(avatarsDir); os.IsNotExist(err) {
		avatarsDir = "avatars"
		filePath = filepath.Join(avatarsDir, filename)
	}
	content, err := os.ReadFile(filePath)
	return base64.StdEncoding.EncodeToString(content), nil
}
```

Из-за отсутствия проверок тут можно получить path traversal и вытащить системные файлы, например базу данных пользователей.

### Уязвимость 3. DOS

С помощью этой уязвимости нельзя получить флаг, но можно вызвать красный статус у чекера, уменьшая SLA сервиса у соперников.

Все в той же ручке загрузки аватарок по url не проверятся размер файла, который пытается загрузить бекенд.

То есть можно послать реквест на обновление аватарки с файлом который весит несколько гигабайт, что остановит сервис. 
