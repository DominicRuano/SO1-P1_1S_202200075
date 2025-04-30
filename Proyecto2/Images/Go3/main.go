package main

import (
	"context"
	"log"
	"net"
	"time"

	"KafkaWritter/proto"

	"github.com/segmentio/kafka-go"
	"google.golang.org/grpc"
)

type server struct {
	proto.UnimplementedTweetServiceServer
}

func publishToKafka(tweet *proto.Tweet) error {
	writer := kafka.Writer{
		Addr:     kafka.TCP("kafka-service.proyecto2.svc.cluster.local:9092"),
		Topic:    "tweets-clima",
		Balancer: &kafka.LeastBytes{},
	}

	defer writer.Close()

	msg := kafka.Message{
		Value: []byte(tweet.Description + "|" + tweet.Country + "|" + tweet.Weather),
		Time:  time.Now(),
	}

	return writer.WriteMessages(context.Background(), msg)
}

func (s *server) SendTweet(ctx context.Context, tweet *proto.Tweet) (*proto.Response, error) {
	log.Println("Recibido Tweet gRPC KafkaWritter: ", tweet)
	err := publishToKafka(tweet)
	if err != nil {
		log.Println("Error al publicar en Kafka:", err)
		return &proto.Response{Message: "Error en Kafka"}, nil
	}
	return &proto.Response{Message: "Publicado en Kafka"}, nil
}

func main() {
	lis, err := net.Listen("tcp", ":6000")
	if err != nil {
		log.Fatalf("Fallo al escuchar: %v", err)
	}

	s := grpc.NewServer()
	proto.RegisterTweetServiceServer(s, &server{})

	log.Println("Servidor gRPC KafkaWritter escuchando en :6000")
	if err := s.Serve(lis); err != nil {
		log.Fatalf("Fallo al servir: %v", err)
	}
}
