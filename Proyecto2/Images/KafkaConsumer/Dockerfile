# Etapa 1: Build
FROM golang:1.23 as builder

WORKDIR /app

COPY go.mod .
COPY go.sum .
RUN go mod download

COPY . .

RUN go build -o kafkaconsumer

# Etapa 2: Imagen final
FROM debian:bookworm-slim

WORKDIR /usr/local/bin

COPY --from=builder /app/kafkaconsumer .

EXPOSE 6000

CMD ["./kafkaconsumer"]


