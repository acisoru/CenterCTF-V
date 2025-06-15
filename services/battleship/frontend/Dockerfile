FROM nginx:alpine

# Копируем все файлы из текущей директории в директорию для статических файлов Nginx
COPY . /usr/share/nginx/html/

# Настройка CORS и проксирование запросов к бэкенду
RUN echo ' \
server { \
    listen 80; \
    server_name localhost; \
    
    # Логирование для отладки
    access_log /var/log/nginx/access.log; \
    error_log /var/log/nginx/error.log; \
    
    # Основные настройки для статических файлов
    location / { \
        root /usr/share/nginx/html; \
        index index.html; \
        try_files $uri $uri/ /index.html; \
        
        # Базовые заголовки кэширования для статических файлов
        add_header Cache-Control "public, max-age=3600"; \
    } \
    
    # Отключаем кэширование для HTML файлов
    location ~* \\.html$ { \
        root /usr/share/nginx/html; \
        add_header Cache-Control "no-cache, no-store, must-revalidate"; \
        expires 0; \
    } \
    
    # Проксирование API запросов к бэкенду
    location /api/ { \
        proxy_pass http://battleship-backend:8080/api/; \
        proxy_http_version 1.1; \
        proxy_set_header Host $host; \
        proxy_set_header X-Real-IP $remote_addr; \
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for; \
        proxy_set_header X-Forwarded-Proto $scheme; \
        
        # Настройки для работы с сессиями и куками
        proxy_set_header Cookie $http_cookie; \
        proxy_cookie_path / /; \
        
        # CORS настройки
        add_header Access-Control-Allow-Origin http://localhost:8081 always; \
        add_header Access-Control-Allow-Methods "GET, POST, OPTIONS, PUT, DELETE" always; \
        add_header Access-Control-Allow-Headers "Origin, Content-Type, Accept, Authorization, X-Requested-With" always; \
        add_header Access-Control-Allow-Credentials true always; \
        add_header Access-Control-Max-Age 86400 always; \
        
        # Обработка OPTIONS запросов
        if ($request_method = OPTIONS ) { \
            add_header Access-Control-Allow-Origin http://localhost:8081 always; \
            add_header Access-Control-Allow-Methods "GET, POST, OPTIONS, PUT, DELETE" always; \
            add_header Access-Control-Allow-Headers "Origin, Content-Type, Accept, Authorization, X-Requested-With" always; \
            add_header Access-Control-Allow-Credentials true always; \
            add_header Access-Control-Max-Age 86400 always; \
            add_header Content-Length 0; \
            add_header Content-Type text/plain; \
            return 204; \
        } \
    } \
} \
' > /etc/nginx/conf.d/default.conf

# Создаем необходимые директории, если их нет
RUN mkdir -p /var/log/nginx

EXPOSE 80

# Запускаем Nginx в режиме foreground
CMD ["nginx", "-g", "daemon off;"] 