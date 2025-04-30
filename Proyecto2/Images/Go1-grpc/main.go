package main

import (
	"context"
	"log"
	"net"

	"github.com/tuusuario/Go1-grpc/proto"

	"google.golang.org/grpc"
)

type server struct {
	proto.UnimplementedTweetServiceServer
}

func (s *server) SendTweet(ctx context.Context, tweet *proto.Tweet) (*proto.Response, error) {
	log.Println("Tweet recibido en el servidor gRPC:")
	log.Println("Descripción:", tweet.Description)
	log.Println("País:", tweet.Country)
	log.Println("Clima:", tweet.Weather)

	// Aquí a Go2 y Go3 (Kafka y RabbitMQ)

	return &proto.Response{Message: "Tweet recibido en gRPC Server"}, nil
}

func main() {
	lis, err := net.Listen("tcp", ":50051")
	if err != nil {
		log.Fatalf("Fallo al escuchar: %v", err)
	}

	s := grpc.NewServer()
	proto.RegisterTweetServiceServer(s, &server{})

	log.Println("Servidor gRPC escuchando en :50051")
	if err := s.Serve(lis); err != nil {
		log.Fatalf("Fallo al servir: %v", err)
	}
}

