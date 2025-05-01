from locust import HttpUser, task, between
import json
import random

# Leer los tweets del archivo JSON (una sola vez al inicio)
with open("tweets.json", "r", encoding="utf-8") as file:
    tweets_data = json.load(file)

class TweetLoadTest(HttpUser):
    wait_time = between(0.1, 0.5)

    @task
    def send_dynamic_tweet(self):
        tweet = random.choice(tweets_data)
        self.client.post("/input", json=tweet)

