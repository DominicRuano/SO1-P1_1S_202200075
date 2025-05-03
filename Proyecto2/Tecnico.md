# Documentación del Proyecto 2 – Tweets del Clima
---

**Universidad de San Carlos de Guatemala**
Facultad de Ingeniería, Escuela de Ciencias y Sistemas

**Curso:** Sistemas Operativos 1 – 1S2025
**Proyecto 2:** Tweets del Clima

**Estudiante:** Dominic Juan Pablo Ruano
**Carné:** 202200075
**Fecha:** 02 de mayo de 2025
-----------------------------

## 1. Despliegues

### 1.1 Ingress

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rust-api-ingress
  namespace: proyecto2
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: 
spec:
  rules:
  - host: ""
    http:
      paths:
      - path: /input
        pathType: Prefix
        backend:
          service:
            name: rust-api-service
            port:
              number: 80
```

**Explicación**: El Ingress dirige el tráfico HTTP/HTTPS entrante a los servicios REST y gRPC. Utiliza NGINX como controlador de Ingress, permitiendo la reescritura de URL y balanceo de carga entre réplicas.

### 1.2 API Rust (Lectura de `/input` y envío de logs)

**Dockerfile**:

```dockerfile
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

```

**Deployment**:

```yaml
# Deployment de Rust
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rust-api
  namespace: proyecto2
spec:
  replicas: 1
  selector:
    matchLabels:
      app: rust-api
  template:
    metadata:
      labels:
        app: rust-api
    spec:
      containers:
      - name: rust-api
        image: 34.139.16.44.nip.io/sopes/rust-api:latest
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: "20m"
            memory: "5Mi"
          limits:
            cpu: "50m"
            memory: "100Mi"
---

# Service para Rust
apiVersion: v1
kind: Service
metadata:
  name: rust-api-service
  namespace: proyecto2
spec:
  selector:
    app: rust-api
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: ClusterIP
```

**Explicación**: Levanta un contenedor que lee periódicamente `/input` y envía los registros via REST API al deploy de go.

### 1.3 API Go REST y GRPC

**Dockerfile**:

```dockerfile
# Etapa 1: Build
FROM golang:1.23 as builder

WORKDIR /app

COPY go.mod .
COPY go.sum .
RUN go mod download

COPY . .

RUN go build -o go-api

# Etapa 2: Imagen final
FROM debian:bookworm-slim

WORKDIR /usr/local/bin

COPY --from=builder /app/go-api .

EXPOSE 8081

CMD ["./go-api"]

```

**Deployment + Service**:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: go-api
  namespace: proyecto2
spec:
  replicas: 1
  selector:
    matchLabels:
      app: go-api
  template:
    metadata:
      labels:
        app: go-api
    spec:
      containers:
      - name: go-api
        image: 34.139.16.44.nip.io/sopes/go-api-rest:latest
        imagePullPolicy: Always
        ports:
        - containerPort: 8081

      - name: go-grpc-server
        image: 34.139.16.44.nip.io/sopes/go-api-grpc:latest
        imagePullPolicy: Always
        ports:
        - containerPort: 50051

---

# Service interno para Go API
apiVersion: v1
kind: Service
metadata:
  name: go-api-service
  namespace: proyecto2
spec:
  selector:
    app: go-api
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8081
  type: ClusterIP

```

**Explicación**: Servicio REST expuesto internamente por `ClusterIP`, envia internamente del api rest a el primer GRPC client.

### 1.4 RabbtMQ/Kafka Writter

**Deployment + Service**:

```yaml
# RabbitMQWritter
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rabbitwritter
  namespace: proyecto2
spec:
  replicas: 1
  selector:
    matchLabels:
      app: rabbitwritter
  template:
    metadata:
      labels:
        app: rabbitwritter
    spec:
      containers:
      - name: rabbitwritter
        image: 34.139.16.44.nip.io/sopes/rabbitwritter:latest
        imagePullPolicy: Always
        ports:
        - containerPort: 6000

---
# KafkaWritter
apiVersion: apps/v1
kind: Deployment
metadata:
  name: kafkawritter
  namespace: proyecto2
spec:
  replicas: 1
  selector:
    matchLabels:
      app: kafkawritter
  template:
    metadata:
      labels:
        app: kafkawritter
    spec:
      containers:
      - name: kafkawritter
        image: 34.139.16.44.nip.io/sopes/kafkawritter:latest
        imagePullPolicy: Always
        ports:
        - containerPort: 6000

---
# Service para RabbitMQWritter
apiVersion: v1
kind: Service
metadata:
  name: rabbitwritter
  namespace: proyecto2
spec:
  selector:
    app: rabbitwritter
  ports:
    - protocol: TCP
      port: 6000
      targetPort: 6000
  type: ClusterIP

---
# Service para KafkaWritter
apiVersion: v1
kind: Service
metadata:
  name: kafkawritter
  namespace: proyecto2
spec:
  selector:
    app: kafkawritter
  ports:
    - protocol: TCP
      port: 6000
      targetPort: 6000
  type: ClusterIP
```

**Explicación**: Servicio GRPC expuesto internamente por `ClusterIP`, recibe por GRPC los mensajes de la API REST y los envía a RabbitMQ o Kafka. El servicio de RabbitMQ se expone por `ClusterIP` y el de kafak tambien.

### 1.5 RabbitMQ/Kafka deploys

```yaml
# RabbitMQ Deploy/pod
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rabbitmq
  namespace: proyecto2
spec:
  replicas: 1
  selector:
    matchLabels:
      app: rabbitmq
  template:
    metadata:
      labels:
        app: rabbitmq
    spec:
      containers:
      - name: rabbitmq
        image: rabbitmq:3-management
        ports:
        - containerPort: 5672  # AMQP
        - containerPort: 15672 # Web UI
        env:
        - name: RABBITMQ_DEFAULT_USER
          value: guest
        - name: RABBITMQ_DEFAULT_PASS
          value: guest

---
# RabbitMQ Service
apiVersion: v1
kind: Service
metadata:
  name: rabbitmq
  namespace: proyecto2
spec:
  selector:
    app: rabbitmq
  ports:
  - name: amqp
    port: 5672
    targetPort: 5672
  - name: management
    port: 15672
    targetPort: 15672
  type: ClusterIP

---
# Strimzi Kafka Cluster (1 nodo, sin TLS ni autenticación)
apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata:
  name: kafka-cluster
  namespace: proyecto2
spec:
  kafka:
    version: 3.8.1
    replicas: 1
    listeners:
      - name: plain
        port: 9092
        type: internal
        tls: false
    config:
      offsets.topic.replication.factor: 1
      transaction.state.log.replication.factor: 1
      transaction.state.log.min.isr: 1
      log.message.format.version: "3.6"
      auto.create.topics.enable: true
    storage:
      type: ephemeral
  zookeeper:
    replicas: 1
    storage:
      type: ephemeral
  entityOperator:
    topicOperator: {}
    userOperator: {}

---
# KafkaTopic tweets-clima
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaTopic
metadata:
  name: tweets-clima
  namespace: proyecto2
  labels:
    strimzi.io/cluster: kafka-cluster
spec:
  partitions: 1
  replicas: 1
  topicName: tweets-clima

```

**Explicación**: Kafka y rabbit distribuye los mensajes en topics y particiones.

### 1.7 RabbitMQ/Kafka Consumers

```yaml
# RabbitMQConsumer
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rabbitconsumer
  namespace: proyecto2
spec:
  replicas: 1
  selector:
    matchLabels:
      app: rabbitconsumer
  template:
    metadata:
      labels:
        app: rabbitconsumer
    spec:
      containers:
      - name: rabbitconsumer
        image: 34.139.16.44.nip.io/sopes/rabbitconsumer:latest
        imagePullPolicy: Always

---
# KafkaConsumer
apiVersion: apps/v1
kind: Deployment
metadata:
  name: kafkaconsumer
  namespace: proyecto2
spec:
  replicas: 1
  selector:
    matchLabels:
      app: kafkaconsumer
  template:
    metadata:
      labels:
        app: kafkaconsumer
    spec:
      containers:
      - name: kafkaconsumer
        image: 34.139.16.44.nip.io/sopes/kafkaconsumer:latest
        imagePullPolicy: Always
```

**Explicación**: Lee mensajes de Kafka (o RabbitMQ) y almacena datos intermedios en Redis o Valkey.

### 1.8 Redis/ Valkey deployment

```yaml
# Redis Persistent Volume Claim
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: redis-pvc
  namespace: proyecto2
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 2Gi
  storageClassName: standard

---
# Valkey Persistent Volume Claim
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: valkey-pvc
  namespace: proyecto2
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 2Gi
  storageClassName: standard

---
# Redis Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: redis
  namespace: proyecto2
  labels:
    app: redis
spec:
  replicas: 1
  selector:
    matchLabels:
      app: redis
  template:
    metadata:
      labels:
        app: redis
    spec:
      containers:
        - name: redis
          image: redis:7.2
          ports:
            - containerPort: 6379
          volumeMounts:
            - name: redis-data
              mountPath: /data
          args: ["--appendonly", "yes"]
          resources:
            limits:
              memory: "256Mi"
              cpu: "250m"
      volumes:
        - name: redis-data
          persistentVolumeClaim:
            claimName: redis-pvc

---
# Valkey Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: valkey
  namespace: proyecto2
  labels:
    app: valkey
spec:
  replicas: 1
  selector:
    matchLabels:
      app: valkey
  template:
    metadata:
      labels:
        app: valkey
    spec:
      containers:
        - name: valkey
          image: valkey/valkey:7.2
          ports:
            - containerPort: 6379
          volumeMounts:
            - name: valkey-data
              mountPath: /data
          args: ["--appendonly", "yes"]
          resources:
            limits:
              memory: "256Mi"
              cpu: "250m"
      volumes:
        - name: valkey-data
          persistentVolumeClaim:
            claimName: valkey-pvc


---
# Redis Service
apiVersion: v1
kind: Service
metadata:
  name: redis
  namespace: proyecto2
spec:
  selector:
    app: redis
  ports:
    - protocol: TCP
      port: 6379
      targetPort: 6379
  type: ClusterIP

---
# Valkey Service
apiVersion: v1
kind: Service
metadata:
  name: valkey
  namespace: proyecto2
spec:
  selector:
    app: valkey
  ports:
    - protocol: TCP
      port: 6379
      targetPort: 6379
  type: ClusterIP
```

**Explicación**: Almacén en memoria para métricas de conteo rápido y caches de respuestas.

### 1.10 Grafana

```yaml
# Grafana deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: grafana
  namespace: proyecto2
  labels:
    app: grafana
spec:
  replicas: 1
  selector:
    matchLabels:
      app: grafana
  template:
    metadata:
      labels:
        app: grafana
    spec:
      containers:
      - name: grafana
        image: grafana/grafana:10.2.3
        ports:
        - containerPort: 3000

---
# Grafana service
apiVersion: v1
kind: Service
metadata:
  name: grafana
  namespace: proyecto2
spec:
  type: NodePort
  selector:
    app: grafana
  ports:
    - port: 3000
      targetPort: 3000
      nodePort: 32000

```

**Explicación**: Dashboard para visualizar métricas recopiladas de Redis y Valkey.

### 1.11 Harbor

**Instalación**: desplegado en una VPC aparte o `namespace` dedicado:

```shell
helm repo add harbor https://helm.goharbor.io
helm install harbor harbor/harbor \
  --set expose.type=nodePort \
  --set externalURL=https://harbor.example.com
```

**Explicación**: Registro privado para almacenar imágenes Docker usadas en los despliegues.

## 2. Respuestas a preguntas conceptuales

**1. ¿Cómo funciona Kafka?**
Kafka es un sistema de mensajería basado en un registro distribuido (log). Los productores publican mensajes en *topics*, que se dividen en particiones replicadas entre *brokers*. Los consumidores se suscriben a topics y leen mensajes de forma secuencial, manteniendo un *offset* propio. Esto permite alta escalabilidad, tolerancia a fallos y procesamiento en tiempo real.

**2. ¿Cómo difiere Valkey de Redis?**
Valkey es un almacén clave-valor con cifrado en reposo y persistencia transaccional más robusta, orientado a casos de uso donde la durabilidad y seguridad son críticas. Redis es un almacén en memoria más rápido, ideal para caches y contadores en tiempo real, pero con opciones de persistencia menos granulares.

**3. ¿Es mejor gRPC que HTTP?**
gRPC usa HTTP/2 y Protobuf, ofreciendo comunicación binaria eficiente, soporte nativo para streaming y contratos fuertemente tipados. HTTP/REST basada en JSON es más simple y universal, pero menos eficiente en tamaño de mensaje y sin streaming nativo. La elección depende de requisitos de rendimiento versus compatibilidad.

**4. ¿Hubo una mejora al utilizar dos réplicas en los deployments de API REST y gRPC?**
Sí: se logró alta disponibilidad y balanceo de carga interno. En pruebas de carga con Locust, duplicar réplicas redujo el tiempo de respuesta promedio.

**5. Para los consumidores, ¿qué opción(es) utilizó y por qué?**
Se utilizo una funcion normal de go, ya que no se vio la necesidad de usar los goroutines, ya que el flujo de mensajes no es tan alto. Se utilizó un `for` para leer los mensajes de Kafka y RabbitMQ, y se almacenaron en Redis o Valkey.

### 3.2 Flujo de mensajes

1. **Locust** genera solicitudes HTTPS ➔  2. **Ingress** ➔ 3. **API REST/GRPC** ➔ 4. Publica en **Kafka** (o **RabbitMQ**) ➔ 5. **Consumidor** lee y almacena en **Redis/Valkey** ➔ 6. **Grafana** consulta métricas de Redis/Valkey.
