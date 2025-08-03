# Ejemplos de ejecución del Simulador de Planificación de Procesos

## Ejemplo 1: Configuración básica
- Capacidad del archivo: 3
- Algoritmo: FCFS
- Procesos: 4 (2 productores, 2 consumidores)

### Resultado esperado:
```
🗂️  Configuración del archivo compartido:
Ingrese la capacidad del archivo (1-10): 3

📋 Seleccione el algoritmo:
1. FCFS
2. SJN
3. Prioridad (sin desalojo)
4. Prioridad (con desalojo)
5. Round Robin
6. Cargar estado desde archivo
7. Guardar estado actual
Opción: 1

🔷 Ejecutando FCFS con hilos...
```

## Ejemplo 2: Prueba con Round Robin
- Capacidad del archivo: 5
- Quantum: 2 segundos
- Algoritmo: Round Robin

## Ejemplo 3: Persistencia de datos
1. Ejecutar simulación
2. Seleccionar opción 7 para guardar estado
3. Cerrar programa
4. Abrir programa nuevamente
5. Seleccionar opción 6 para cargar estado

## Archivos generados:
- `metricas.txt` - Contiene las métricas de todas las ejecuciones
- `estado_ejemplo.txt` - Archivo de estado guardado (ejemplo)
