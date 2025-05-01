package main

import (
	"log"
	"strings"
	"fmt"

	"github.com/redis/go-redis/v9"
	"github.com/streadway/amqp"
	"golang.org/x/net/context"
)

var ctx = context.Background()

func main() {
	vdb := redis.NewClient(&redis.Options{
		Addr: "valkey.proyecto2.svc.cluster.local:6379",
	})

	conn, err := amqp.Dial("amqp://guest:guest@rabbitmq.proyecto2.svc.cluster.local:5672/")
	if err != nil {
		log.Fatalf("❌ Fallo conectando a RabbitMQ: %v", err)
	}
	defer conn.Close()

	ch, err := conn.Channel()
	if err != nil {
		log.Fatalf("❌ Fallo abriendo canal: %v", err)
	}
	defer ch.Close()

	msgs, err := ch.Consume("tweets-clima", "", true, false, false, false, nil)
	if err != nil {
		log.Fatalf("❌ Error en consumo de cola: %v", err)
	}

	log.Println("📥 RabbitMQ consumer iniciado")

	for msg := range msgs {
		parts := strings.Split(string(msg.Body), "|")
		if len(parts) != 3 {
			log.Println("❗ Mensaje inválido:", string(msg.Body))
			continue
		}
		
		descripcion := parts[0]
		pais := parts[1]
		clima := parts[2]
	
		// Guardar mensaje como JSON individual
		jsonStr := fmt.Sprintf(`{"description":"%s", "country":"%s", "weather":"%s"}`, descripcion, pais, clima)
		err = vdb.LPush(ctx, "tweets:json", jsonStr).Err()
		if err != nil {
			log.Println("❌ Error guardando JSON:", err)
		}
	
		// Contador por país
		err = vdb.HIncrBy(ctx, "contador:paises", pais, 1).Err()
		if err != nil {
			log.Println("❌ Error contador país:", err)
		}
	
		// Contador total de mensajes
		err = vdb.Incr(ctx, "contador:total").Err()
		if err != nil {
			log.Println("❌ Error contador total:", err)
		}
	
		log.Println("✅ Guardado:", descripcion, pais, clima)
	}
}

