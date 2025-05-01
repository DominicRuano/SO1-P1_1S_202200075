package main

import (
	"context"
	"log"
	"net"

	"RabbitMQWritter/proto"

	"github.com/streadway/amqp"
	"google.golang.org/grpc"
)

type server struct {
	proto.UnimplementedTweetServiceServer
}

func publishToRabbit(tweet *proto.Tweet) error {
	conn, err := amqp.Dial("amqp://guest:guest@rabbitmq.proyecto2.svc.cluster.local:5672/")
	if err != nil {
		return err
	}
	defer conn.Close()

	ch, err := conn.Channel()
	if err != nil {
		return err
	}
	defer ch.Close()

	queue := "tweets-clima"
	_, err = ch.QueueDeclare(queue, false, false, false, false, nil)
	if err != nil {
		return err
	}

	body := tweet.Description + "|" + tweet.Country + "|" + tweet.Weather
	return ch.Publish("", queue, false, false, amqp.Publishing{
		ContentType: "text/plain",
		Body:        []byte(body),
	})
}

func (s *server) SendTweet(ctx context.Context, tweet *proto.Tweet) (*proto.Response, error) {
	log.Println("Recibido Tweet gRPC RabbitMQWritter : ", tweet)
	err := publishToRabbit(tweet)
	if err != nil {
		log.Println("Error al publicar en RabbitMQ:", err)
		return &proto.Response{Message: "Error en RabbitMQ"}, nil
	}
	return &proto.Response{Message: "Publicado en RabbitMQ"}, nil
}

func main() {
	lis, err := net.Listen("tcp", ":6000")
	if err != nil {
		log.Fatalf("Fallo al escuchar: %v", err)
	}

	s := grpc.NewServer()
	proto.RegisterTweetServiceServer(s, &server{})

	log.Println("Servidor gRPC RabbitMQWritter escuchando en :6000")
	if err := s.Serve(lis); err != nil {
		log.Fatalf("Fallo al servir: %v", err)
	}
}

