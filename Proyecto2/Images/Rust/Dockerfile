# Etapa 1: Build
FROM rust:1.81-slim as builder

# Instalar librerías necesarias para compilar dependencias como reqwest
RUN apt-get update && apt-get install -y \
    pkg-config \
    libssl-dev \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app

# Copiar archivos necesarios
COPY Cargo.toml .
COPY src ./src

# Compilar en modo release
RUN cargo build --release

# Etapa 2: Imagen final
FROM debian:bookworm-slim

# Solo instalar certificados (no necesitas OpenSSL runtime)
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/local/bin

# Copiar el binario desde la etapa anterior
COPY --from=builder /usr/src/app/target/release/rust_api /usr/local/bin/rust_api

EXPOSE 8080

CMD ["./rust_api"]
