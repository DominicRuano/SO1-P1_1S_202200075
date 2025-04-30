# Comandos utilizados para la configuración del proyecto

```bash
#  Crear el namespace para el proyecto 2
kubectl create namespace proyecto2

# Habilitamos ingress (en minikube)
minikube addons enable ingress

# Iniciar sesssion en harbor
docker login 34.133.12.34.nip.io

# Aplicar un yml
kubectl apply -f rust.yml

# Eliminar un yml
kubectl delete -f rust.yml

# Habilitar las metricas del servidor (ESPERAR 1 minuto)
minikube addons enable metrics-server

# Ver los pods en el namespace proyecto2 o todo el namespace
kubectl get pods -n proyecto2
kubectl get all -n proyecto2

# Ver los que tiene asignado
kubectl top pod -n proyecto2

# Ver los que tiene asignado (en tiempo real)
watch -n 1 kubectl top pod -n proyecto2
watch -n 1 kubectl get hpa -n proyecto2

# Instalar proto
go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

# Ejecutar proto
export PATH="$PATH:$(go env GOPATH)/bin"
protoc --go_out=. --go-grpc_out=. tweet.proto


```
