#Documentación del Proyecto: Gestor de Contenedores

## Universidad de San Carlos de Guatemala

**Facultad de Ingeniería**\
**Escuela de Ciencias y Sistemas**\
**Sistemas Operativos 1 - Sección “P”**

### Docente y Auxiliares

- **Ing. Sergio Arnaldo Méndez**
- **Aux. Álvaro Norberto García**
- **Aux. Sergio Alfonso Ferrer García**

---

## 1. Introducción

El presente proyecto consiste en la implementación de un gestor de contenedores en Linux, utilizando diversos lenguajes y herramientas. Este sistema permitirá comprender a fondo el funcionamiento del kernel de Linux, la gestión eficiente de recursos en un entorno virtualizado y la automatización de procesos a través de contenedores.

Este proyecto integra los conocimientos adquiridos en la primera unidad del curso de Sistemas Operativos 1. En él, se pondrán en práctica técnicas avanzadas de administración de sistemas mediante el uso de Bash, módulos del kernel en C, desarrollo en Rust y la utilización de Docker para la gestión de contenedores. El objetivo final es construir un sistema capaz de manejar contenedores de manera eficiente, minimizando el uso innecesario de recursos y asegurando un control preciso sobre su ejecución y eliminación.

## 2. Objetivos

- Explorar el funcionamiento interno del Kernel de Linux mediante la creación de módulos en C, lo que permitirá obtener información precisa del sistema.
- Utilizar el lenguaje Rust para la gestión del sistema, asegurando una administración segura y eficiente de los contenedores.
- Comprender el funcionamiento de los contenedores con Docker, permitiendo su implementación efectiva en entornos de producción.
- Automatizar procesos mediante scripts en Bash, facilitando la creación y gestión de contenedores sin intervención manual constante.
- Implementar un sistema de monitoreo en tiempo real de los recursos consumidos por cada contenedor.
- Generar reportes gráficos y logs estructurados para analizar el comportamiento de los contenedores en el tiempo.

## 3. Arquitectura del Proyecto

El sistema consta de los siguientes componentes fundamentales:

1. **Script creador de contenedores**: Responsable de generar y administrar contenedores de Docker de manera aleatoria mediante un cronjob.
2. **Módulo de Kernel en C**: Encargado de capturar métricas del sistema, incluyendo información detallada sobre la gestión de memoria y el uso de recursos computacionales.
3. **Servicio en Rust**: Gestiona la ejecución, análisis y eliminación de contenedores, garantizando que solo los procesos esenciales permanezcan activos.
4. **Contenedor administrador de logs**: Almacena registros de actividad y genera gráficas basadas en los datos recolectados, proporcionando una visión clara del uso de los recursos.

## 4. Implementación

### 4.1 Script Creador de Contenedores

- Genera 10 contenedores aleatorios cada 30 segundos.
- Los contenedores se crean a partir de la imagen `containerstack/alpine-stress`, especializada en la simulación de estrés en sistemas.
- Cada contenedor puede consumir distintos recursos: RAM, CPU, I/O y Disco.
- Se utilizan métodos aleatorios para asignar nombres únicos a los contenedores, como `/dev/urandom` o la fecha del sistema.
- El cronjob automatiza este proceso, asegurando una ejecución continua del script sin intervención manual.

### 4.2 Setup

Comandos utilizados:
```bash
// Comando para hacer pull a la imagen
sudo docker pull  containerstack/alpine-stress

```


### 4.2 Módulo de Kernel

- Captura información detallada sobre la memoria RAM y el uso del CPU en KB o MB.
- Registra procesos relacionados con los contenedores en ejecución, almacenando los siguientes datos:
  - PID del proceso
  - Nombre del proceso
  - Comando de ejecución o ID del contenedor
  - Uso de memoria en porcentaje
  - Consumo de CPU
  - Consumo de disco
  - Actividad de I/O
- Los datos se guardan en `/proc/sysinfo_202200075` en formato JSON, facilitando su acceso y análisis.

### 4.3 Servicio en Rust

- Se encarga de la gestión central del sistema y la administración eficiente de los contenedores.
- Crea un contenedor de administración de logs que recibirá todas las peticiones HTTP generadas durante la ejecución.
- Ejecuta un análisis de los contenedores activos cada 10 segundos para decidir cuáles deben ser eliminados o mantenidos.
- Funcionalidades principales:
  - Lectura y deserialización del archivo `/proc/sysinfo_202200075`.
  - Análisis de los datos del sistema y toma de decisiones sobre la administración de contenedores.
  - Registro y generación de logs para facilitar el monitoreo del sistema.
  - Comunicación con el contenedor administrador de logs mediante peticiones HTTP.
- Reglas de operación:
  - Siempre debe haber al menos un contenedor de cada tipo (CPU, RAM, I/O y Disco).
  - No se debe eliminar el contenedor encargado de administrar los logs.

### 4.4 Contenedor Administrador de Logs

- Implementado en Python con FastAPI para recibir y almacenar logs en formato JSON.
- Utiliza volúmenes de Docker para el almacenamiento persistente de registros.
- Genera dos gráficas representativas con matplotlib basadas en los datos de consumo de los contenedores.
- Ofrece la posibilidad de mostrar datos en tiempo real para un análisis inmediato del sistema.

## 5. Instalación y Uso

### 5.1 Requisitos Previos

- Sistema operativo **Linux** (Físico o Virtualizado).
- Instalación de Docker.
- Instalación de Rust y Cargo.
- Bash y permisos de superusuario.
- Dependencias de Python para FastAPI y matplotlib.

### 5.2 Instalación

1. Clonar el repositorio del proyecto desde GitHub.
2. Compilar e instalar el módulo del kernel en el sistema.
3. Configurar y ejecutar el script creador de contenedores.
4. Iniciar el servicio en Rust.
5. Levantar el contenedor administrador de logs.

### 5.3 Ejemplo de Uso

```bash
sudo insmod gestor_containers.ko
cargo run --release
```

## 6. Evaluación y Restricciones

- Evaluación en modalidad presencial.
- Se verificará la última versión del código en GitHub.
- Penalización del 25% por cada 12 horas de retraso en la entrega, hasta un máximo de 48 horas.
- Solo se permite el uso de Linux como entorno de desarrollo.

## 7. Conclusión

Este proyecto integra diversos conocimientos en sistemas operativos, programación en C, Rust y Bash, y administración de contenedores con Docker. Permite optimizar la gestión de recursos en entornos virtualizados, asegurando un monitoreo eficiente y un control automatizado de los contenedores en ejecución. La implementación de logs estructurados y gráficos representa un avance en la visualización y gestión de los datos generados por el sistema.

