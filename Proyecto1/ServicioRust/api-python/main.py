from fastapi import FastAPI, HTTPException
import json
import os

app = FastAPI()

# Ruta del archivo JSON persistente
LOG_FILE = "/logs/logs.json"

# Asegurar que el archivo existe al inicio
if not os.path.exists(LOG_FILE):
    with open(LOG_FILE, "w") as f:
        json.dump([], f)

@app.post("/logs")
async def save_logs(logs: dict):
    """
    Endpoint para recibir logs desde Rust y guardarlos en un archivo JSON.
    """
    try:
        # Leer logs actuales
        with open(LOG_FILE, "r") as f:
            data = json.load(f)

        # Agregar el nuevo log
        data.append(logs)

        # Guardar logs actualizados
        with open(LOG_FILE, "w") as f:
            json.dump(data, f, indent=4)

        return {"message": "Logs almacenados exitosamente"}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.get("/logs")
async def get_logs():
    """
    Endpoint para devolver todos los logs almacenados.
    """
    try:
        with open(LOG_FILE, "r") as f:
            data = json.load(f)
        return data

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.get("/send-to-grafana")
async def send_logs_to_grafana():
    """
    Endpoint para enviar los logs almacenados a Grafana.
    """
    # Función vacía por ahora
    return {"message": "Función de envío a Grafana pendiente de implementación"}


@app.get("/generate-graphs")
async def generate_graphs():
    """
    Endpoint para generar gráficas basadas en los logs almacenados.
    """
    # Función vacía por ahora
    return {"message": "Generación de gráficas pendiente de implementación"}

