FROM golang:1.21

RUN apt-get update && apt-get install -y \
    sqlite3 \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

RUN go mod init battleship

COPY . .

RUN go mod tidy
RUN go mod download

ENV CGO_ENABLED=1
RUN go build -o battleship /app/main.go

RUN mkdir -p /app/data && \
    chmod 777 /app/data && \
    mkdir -p /app/avatars && \
    chmod 777 /app/avatars

EXPOSE 8080

CMD ["./battleship"] 