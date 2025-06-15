FROM python:3.12-slim

WORKDIR /app

RUN head -c 32 /dev/urandom > /secret_key
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

ENV FLASK_APP=app.py
ENV FLASK_ENV=production

EXPOSE 5454

CMD ["./run.sh"]