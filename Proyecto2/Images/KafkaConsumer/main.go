package main

import (
	"context"
	"log"
	"strings"
	"fmt"

	"github.com/segmentio/kafka-go"
	"github.com/redis/go-redis/v9"
)

var ctx = context.Background()

func main() {
	rdb := redis.NewClient(&redis.Options{
		Addr: "redis.proyecto2.svc.cluster.local:6379",
	})

	reader := kafka.NewReader(kafka.ReaderConfig{
		Brokers: []string{"kafka-cluster-kafka-bootstrap.proyecto2.svc.cluster.local:9092"},
		Topic:   "tweets-clima",
		GroupID: "kafka-consumer-group",
	})

	log.Println("📥 Kafka consumer iniciado")

	for {
		msg, err := reader.ReadMessage(ctx)
		if err != nil {
			log.Printf("❌ Error leyendo mensaje: %v", err)
			continue
		}
	
		parts := strings.Split(string(msg.Value), "|")
		if len(parts) != 3 {
			log.Println("❗ Mensaje inválido:", string(msg.Value))
			continue
		}
	
		descripcion := parts[0]
		pais := parts[1]
		clima := parts[2]
	
		// Guardar mensaje como JSON individual
		jsonStr := fmt.Sprintf(`{"description":"%s", "country":"%s", "weather":"%s"}`, descripcion, pais, clima)
		err = rdb.LPush(ctx, "tweets:json", jsonStr).Err()
		if err != nil {
			log.Println("❌ Error guardando JSON:", err)
		}
	
		// Contador por país
		err = rdb.HIncrBy(ctx, "contador:paises", pais, 1).Err()
		if err != nil {
			log.Println("❌ Error contador país:", err)
		}
	
		// Contador total de mensajes
		err = rdb.Incr(ctx, "contador:total").Err()
		if err != nil {
			log.Println("❌ Error contador total:", err)
		}
	
		log.Println("✅ Guardado:", descripcion, pais, clima)
	}
}

