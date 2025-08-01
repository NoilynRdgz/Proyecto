# 🧵 Simulador de Algoritmos de Planificación de Procesos

Este proyecto implementa un simulador de diferentes algoritmos de planificación de procesos del sistema operativo, incluyendo productores y consumidores que trabajan con un archivo compartido.

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

```bash
# Compilar el programa
gcc -o simulador Algoritmos.c -lpthread

# Ejecutar
./simulador
```

## 📊 Estructura del Proyecto

- `Algoritmos.c` - Código principal con implementación de todos los algoritmos
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
