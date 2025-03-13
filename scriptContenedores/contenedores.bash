# rectorio donde se almacenarán los logs de los contenedores
LOG_DIR="./"
mkdir -p "$LOG_DIR"

# Cantidad de contenedores a crear
NUM_CONTENEDORES=2

# Imagen base
DOCKER_IMAGE="containerstack/alpine-stress"

# Función para generar un nombre único
function generar_nombre() {
    echo "contenedor-$(date +%s%N | sha256sum | head -c 10)"
}

# Función para seleccionar aleatoriamente un tipo de carga
function tipo_carga() {
    local tipos=("cpu" "ram" "io" "disk")
    echo "${tipos[$RANDOM % ${#tipos[@]}]}"
}

# Crear contenedores aleatoriamente
for i in $(seq 1 $NUM_CONTENEDORES); do
    NOMBRE=$(generar_nombre)
    CARGA=$(tipo_carga)

    case "$CARGA" in
        "cpu")
            timeout 10 sudo docker run --rm -d --cpus="0.1" --name "$NOMBRE" "$DOCKER_IMAGE" stress --cpu 1
            ;;
        "ram")
            timeout 10 sudo docker run --rm -d --memory="64m" --memory-swap="64m" --name "$NOMBRE" "$DOCKER_IMAGE" stress --vm 1 --vm-bytes 32M
            ;;
        "io")
            timeout 10 sudo docker run --rm -d --io-max-rbps="1048576" --io-max-wbps="1048576" --name "$NOMBRE" "$DOCKER_IMAGE" stress --io 1
            ;;
        "disk")
            timeout 10 sudo docker run --rm -d --memory="32m" --memory-swap="32m" --name "$NOMBRE" "$DOCKER_IMAGE" stress --hdd 1 --hdd-bytes 64M
            ;;
    esac
    
    echo "[$(date)] Contenedor creado: $NOMBRE ($CARGA)" | tee -a "$LOG_DIR/AUTODELETE.log"
done
