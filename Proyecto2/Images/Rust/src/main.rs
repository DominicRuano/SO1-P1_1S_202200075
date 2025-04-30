use actix_web::{post, web, App, HttpResponse, HttpServer, Responder};
use serde::{Deserialize, Serialize};
use reqwest::StatusCode;

#[derive(Serialize, Deserialize)]
struct Tweet {
    description: String,
    country: String,
    weather: String,
}

#[post("/input")]
async fn receive_tweet(tweet: web::Json<Tweet>) -> impl Responder {
    // println!("Recibido Tweet: description={}, country={}, weather={}",
    //     tweet.description, tweet.country, tweet.weather);

    let client = reqwest::Client::new();
    let go_api_url = "http://go-api-service.proyecto2.svc.cluster.local/input";

    // println!("Enviando POST a {}", go_api_url);

    let res = client
        .post(go_api_url)
        .json(&*tweet)
        .send()
        .await;

    match res {
        Ok(r) => {
            // println!("Respuesta de Go API: status={}", r.status());
            if r.status().is_success() {
                HttpResponse::Ok().body("Reenvío exitoso a Go API")
            } else {
                eprintln!("Go API respondió error: {:?}", r.status());
                HttpResponse::BadGateway().body(format!("Error en Go API: {}", r.status()))
            }
        }
        Err(e) => {
            eprintln!("Error conectando a Go API: {:?}", e);
            HttpResponse::BadGateway().body("No se pudo conectar al servicio Go API")
        }
    }
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    println!("Servidor Rust iniciado en http://0.0.0.0:8080");

    HttpServer::new(|| {
        App::new()
            .service(receive_tweet)
    })
    .bind(("0.0.0.0", 8080))?
    .run()
    .await
}
