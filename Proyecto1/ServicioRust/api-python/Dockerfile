# Usa una imagen base de Python
FROM python:3.10

# Establece el directorio de trabajo
WORKDIR /app

# Copia los archivos necesarios
COPY requirements.txt .
COPY main.py .

# Instala las dependencias
RUN pip install --no-cache-dir -r requirements.txt

# Expone el puerto de FastAPI
EXPOSE 8000

# Comando para ejecutar la API
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000"]

