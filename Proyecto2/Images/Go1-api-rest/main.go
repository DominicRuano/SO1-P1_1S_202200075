package main

import (
	"github.com/gin-gonic/gin"
	"net/http"
	"github.com/tuusuario/Go1-rest/proto"
)

type Tweet struct {
	Description string `json:"description"`
	Country     string `json:"country"`
	Weather     string `json:"weather"`
}

func main() {
	router := gin.Default()

	router.POST("/input", func(c *gin.Context) {
		var tweet Tweet
		if err := c.ShouldBindJSON(&tweet); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
			return
		}

		// Llamada al cliente gRPC
		ptweet := &proto.Tweet{
			Description: tweet.Description,
			Country:     tweet.Country,
			Weather:     tweet.Weather,
		}
		if err := SendTweetToGRPCServer(ptweet); err != nil {		
			c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
			return
		}

		c.JSON(http.StatusOK, gin.H{"message": "Tweet recibido en Go API "})
	})

	router.Run(":8081")
}

