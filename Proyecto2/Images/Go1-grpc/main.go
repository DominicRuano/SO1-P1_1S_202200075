package main

import (
	"context"
	"log"
	"net"
	"time"

	"github.com/tuusuario/Go1-grpc/proto"

	"google.golang.org/grpc"
)

type server struct {
	proto.UnimplementedTweetServiceServer
}

func sendToExternalGRPC(addr string, tweet *proto.Tweet) {
	conn, err := grpc.Dial(addr, grpc.WithInsecure(), grpc.WithBlock(), grpc.WithTimeout(2*time.Second))
	if err != nil {
		log.Printf("❌ No se pudo conectar a %s: %v", addr, err)
		return
	}
	defer conn.Close()

	client := proto.NewTweetServiceClient(conn)
	ctx, cancel := context.WithTimeout(context.Background(), time.Second*2)
	defer cancel()

	// Si se quiere debugear este contenedor, se dene quitar el _ de la siguiente linea y poner res
	// Luego descomentar el printf y revisar la respuesta recibida.
	_, err = client.SendTweet(ctx, tweet)
	if err != nil {
		log.Printf("❌ Error al enviar a %s: %v", addr, err)
		return
	}

	// log.Printf("✅ Respuesta de %s: %s", addr, res.Message)
}

func (s *server) SendTweet(ctx context.Context, tweet *proto.Tweet) (*proto.Response, error) {
	// log.Println("Tweet recibido en el servidor gRPC:")
	// log.Println("Descripción:", tweet.Description)
	// log.Println("País:", tweet.Country)
	// log.Println("Clima:", tweet.Weather)

	// Enviar a Go2 (RabbitMQ)
	go sendToExternalGRPC("rabbitwritter.proyecto2.svc.cluster.local:6000", tweet)

	// Enviar a Go3 (Kafka)
	go sendToExternalGRPC("kafkawritter.proyecto2.svc.cluster.local:6000", tweet)

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

