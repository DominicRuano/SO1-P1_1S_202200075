use std::{fs, process::Command, thread, time::{Duration, Instant}};
use reqwest::blocking::Client;
use serde::{Deserialize, Serialize};
use std::sync::{Arc, atomic::{AtomicBool, Ordering}};

// Estructuras para manejar la memoria y CPU
#[derive(Serialize, Deserialize, Debug, Clone)]
struct MemoryInfo {
    Total: u64,
    Free: u64,
    Used: u64,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct CPUInfo {
    Usage: f64,
}

// Estructura para la información de los procesos
#[derive(Serialize, Deserialize, Debug, Clone)]
struct ProcessInfo {
    PID: u32,
    Name: String,
    Cmdline: String,
    MemoryUsage: u64,
    CPUUsage: f64,
    DiskRead: u64,
    DiskWrite: u64,
    Time: u64,
}

// Estructura principal del JSON
#[derive(Serialize, Deserialize, Debug, Clone)]
struct SysInfo {
    Memory: MemoryInfo,
    CPU: CPUInfo,
    Processes: Vec<ProcessInfo>,
}

// 🛠 Función para levantar el contenedor si no está en ejecución
fn start_container() {
    let output = Command::new("docker")
        .args(["ps", "--filter", "name=api-python-container", "--format", "{{.Names}}"])
        .output()
        .expect("Fallo al verificar contenedores");

    let running = String::from_utf8_lossy(&output.stdout).contains("api-python-container");

    if running {
        println!("✅ Contenedor ya está en ejecución.");
        return;
    }

    println!("🚀 Iniciando contenedor...");

    let output = Command::new("docker")
        .args(["run", "-d", "--rm", "-p", "8000:8000", "-v", "/home/user/proyecto/logs:/logs", "--name", "api-python-container", "api-python"])
        .output()
        .expect("Fallo al iniciar el contenedor");

    if output.status.success() {
        println!("✅ Contenedor iniciado correctamente.");
    } else {
        let error_msg = String::from_utf8_lossy(&output.stderr);
        println!("❌ Error iniciando el contenedor: {}", error_msg);
    }
}

// 🛠 Función para leer y deserializar `/proc/sysinfo_<carnet>`
fn read_sysinfo(carnet: &str) -> Option<SysInfo> {
    let path = format!("/proc/sysinfo_{}", carnet);
    if let Ok(content) = fs::read_to_string(&path) {
        serde_json::from_str(&content).ok()
    } else {
        println!("❌ Error leyendo `{}`", path);
        None
    }
}

// 🛠 Función para enviar logs a la API en Python
fn send_log(log_server: &str, data: &SysInfo) {
    let client = Client::new();
    let response = client.post(log_server).json(data).send();

    match response {
        Ok(resp) => println!("📡 Logs enviados: {:?}", resp.status()),
        Err(e) => println!("❌ Error enviando logs: {}", e),
    }
}

// 🛠 Función para eliminar contenedores innecesarios (Aún no implementada)
// fn clean_containers() {
//     println!("🗑 Eliminando contenedores innecesarios...");
// }

// 🛠 Función para imprimir logs estilizados
fn print_logs(data: &SysInfo) {
    println!("📊 ==== INFORME DEL SISTEMA ====");
    println!("💾 Memoria Total: {} MB", data.Memory.Total);
    println!("🟢 Memoria Libre: {} MB", data.Memory.Free);
    println!("🔴 Memoria Usada: {} MB", data.Memory.Used);
    println!("🖥 CPU Uso: {}%", data.CPU.Usage);
    println!("📝 Procesos:");
    for process in &data.Processes {
        println!(
            "  🔹 PID: {} | {} | CPU: {}% | RAM: {} MB",
            process.PID, process.Name, process.CPUUsage, process.MemoryUsage
        );
    }
    println!("=================================");
}

// 🛠 Función para detener el contenedor al finalizar
fn stop_container() {
    let _ = Command::new("docker")
        .args(["stop", "api-python-container"])
        .output()
        .expect("Fallo al detener el contenedor");

    println!("🛑 Contenedor detenido.");
}

fn main() {
    let carnet = "202200075";  // Cambia por tu carnet real
    let log_server = "http://localhost:8000/logs";  // API en Python

    let running = Arc::new(AtomicBool::new(true));
    let r = running.clone();  // ✅ Clonar `running` para usarlo en `while`

    // Capturar SIGINT (Ctrl+C) para cerrar el contenedor antes de salir
    ctrlc::set_handler(move || {
        println!("🛑 Señal SIGINT recibida, deteniendo contenedor...");
        running.store(false, Ordering::SeqCst);
    }).expect("Error al configurar el manejador de señales");



    println!("🎯 Iniciando servicio en Rust...");
    start_container();  // 🔹 Levantar contenedor antes del loop

    while r.load(Ordering::SeqCst) {
        let start_time = Instant::now(); // ⏳ Marca el inicio del ciclo

        let sysinfo = read_sysinfo(carnet);

        let mut threads = vec![];

        if let Some(data) = sysinfo {
            // 🧵 Thread para enviar logs
            let log_server_clone = log_server.to_string();
            let data_clone = data.clone();
            threads.push(thread::spawn(move || {
                send_log(&log_server_clone, &data_clone);
            }));

            // 🧵 Thread para imprimir logs
            let data_clone = data.clone();
            threads.push(thread::spawn(move || {
                print_logs(&data_clone);
            }));

            // 🧵 Thread para eliminar contenedores (comentado hasta implementarlo)
            // threads.push(thread::spawn(move || {
            //     clean_containers();
            // }));
        }

        // 🔄 Esperar a que todos los threads terminen
        for t in threads {
            t.join().expect("Error en un thread");
        }

        // ⏳ Calcular cuánto tiempo tardó todo
        let elapsed_time = start_time.elapsed();
        let sleep_time = if elapsed_time < Duration::from_secs(30) {
            Duration::from_secs(30) - elapsed_time
        } else {
            Duration::from_secs(0) // Si tomó más de 30s, no esperar
        };

        println!("🕒 Esperando {:?} para el próximo ciclo...", sleep_time);
        thread::sleep(sleep_time);
    }

    // 🔹 Si alguna vez el loop terminara, detenemos el contenedor
    stop_container();
}

