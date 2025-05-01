package main

import (
	"context"
	// "log"
	"time"

	"github.com/tuusuario/Go1-rest/proto"
	"google.golang.org/grpc"
)

func SendTweetToGRPCServer(tweet *proto.Tweet) error {
	// Conexión al servidor gRPC en el mismo Pod (localhost)
	conn, err := grpc.Dial("localhost:50051", grpc.WithInsecure())
	if err != nil {
		return err
	}
	defer conn.Close()

	client := proto.NewTweetServiceClient(conn)

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*2)
	defer cancel()

	// Llamada al método remoto
	_, err = client.SendTweet(ctx, tweet)
	if err != nil {
		return err
	}

	// log.Println("Respuesta del servidor gRPC:", res.Message)
	return nil
}

