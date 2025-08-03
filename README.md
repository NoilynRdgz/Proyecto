# 🧵 Simulador de Algoritmos de Planificación de Procesos

**Universidad Latina de Costa Rica**  
**Ingeniería en Sistemas - BIS11 Sistemas Operativos**  
**Segundo Cuatrimestre 2025**  
**Profesor:** Christopher Seas Solano

Este proyecto implementa un simulador completo de diferentes algoritmos de planificación de procesos del sistema operativo, incluyendo productores y consumidores que trabajan con un archivo compartido de capacidad variable.

## 📋 Características

- **Algoritmos implementados:**
  - FCFS (First Come First Served)
  - SJN (Shortest Job Next)
  - Prioridad sin desalojo
  - Prioridad con desalojo
  - Round Robin

- **Funcionalidades:**
  - Simulación de procesos productores y consumidores
  - Manejo de archivo compartido con mutex
  - Cálculo de métricas (tiempo de espera y respuesta)
  - Guardado de métricas en archivo
  - Guardado y carga de estado del sistema

## 🛠️ Compilación y Ejecución

### Opción 1: Usando Makefile (Recomendado)
```bash
# Compilar el programa
make

# Compilar y ejecutar directamente
make run

# Limpiar archivos compilados
make clean
```

### Opción 2: Compilación manual
```bash
# Compilar el programa
gcc -Wall -Wextra -std=c99 -pthread -o simulador Algoritmos.c

# Ejecutar
./simulador
```

## 📊 Estructura del Proyecto

- `Algoritmos.c` - Código principal con implementación de todos los algoritmos
- `Makefile` - Archivo para facilitar la compilación
- `metricas.txt` - Archivo donde se guardan las métricas de ejecución
- `README.md` - Este archivo

## 🧩 Estructura de Procesos

Cada proceso tiene las siguientes propiedades:
- ID único
- Tipo (Productor = 0, Consumidor = 1)
- Duración de ejecución
- Tiempo de llegada
- Prioridad
- Estado de ejecución

## 🗂️ Archivo Compartido

- **Capacidad variable**: Configurada por el usuario al inicio
- **Productores**: Escriben cálculos matemáticos al archivo
- **Consumidores**: Leen y eliminan información del archivo
- **Sincronización**: Uso de mutex para evitar condiciones de carrera

## 💾 Persistencia de Datos

El sistema permite:
- Guardar el estado completo de la simulación
- Cargar estados previos para continuar simulaciones
- Persistir métricas y resultados en archivos

## 🤝 Colaboración

Este proyecto está configurado para colaboración. Los colaboradores pueden:
1. Clonar el repositorio
2. Hacer modificaciones
3. Crear pull requests
4. Revisar y mejorar los algoritmos

## 📈 Métricas

El programa calcula y muestra:
- Tiempo de espera promedio
- Tiempo de respuesta promedio
- Resultados individuales por proceso

## 🔧 Requisitos

- Compilador GCC
- Biblioteca pthread
- Sistema operativo Unix/Linux
