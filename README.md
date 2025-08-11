# Simulador de Planificación de Procesos y Gestión de Archivos Compartidos

## 1. Descripción del Proyecto

Este proyecto es un simulador de planificación de procesos implementado en el lenguaje C. El simulador permite a los usuarios:
- Comparar el rendimiento de varios algoritmos de planificación.
- Configurar dinámicamente los procesos (productores y consumidores).
- Gestionar un archivo compartido en el disco (`archivo_compartido.txt`) utilizando mecanismos de sincronización (mutex).
- Guardar y cargar el estado completo de la simulación para su análisis posterior.

### Algoritmos Implementados
- **FCFS** (First-Come, First-Served)
- **SJN** (Shortest Job Next)
- **Prioridad** (con y sin desalojo)
- **Round Robin**

## 2. Requisitos del Sistema

- **Compilador:** GCC (GNU Compiler Collection) o similar.
- **Sistema Operativo:** Compatible con POSIX (Linux, macOS). La biblioteca `pthread` es fundamental para la concurrencia.

## 3. Instrucciones de Compilación

Abre una terminal en el directorio donde se encuentran los archivos del proyecto y ejecuta el siguiente comando:

```bash
gcc -o Proyecto Algoritmos.c -lm -pthread

## 4. Para ejecutar:
./Proyecto
