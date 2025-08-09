// Algoritmos.c
// Versión corregida e integrada: archivo compartido en disco + persistencia completa
// Compilar: gcc -o Proyecto Algoritmos.c -lm -pthread

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#define MAX_PROCESOS 50
#define MAX_LINEAS_ARCHIVO 1000
#define MAX_LINEA_LEN 256

// Códigos de colores ANSI
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"

pthread_mutex_t mutex_archivo;

// Variables globales del sistema
int quantum_usuario = 2; // Quantum configurable por el usuario
long total_tiempo_cpu = 0; // Para calcular utilización CPU
long tiempo_inicio_simulacion = 0;

// Archivo compartido en disco y capacidad
char nombre_archivo_compartido[] = "archivo_compartido.txt";
int capacidad_archivo = 0; // número máximo de líneas en el archivo

// Estructura principal de procesos
typedef struct {
    int id;
    int tipo; // 0 = Productor  1 = Consumidor
    int duracion;
    int tiempo_llegada;
    int prioridad;
    int restante; // para RR o desalojo
    long tiempo_inicio;
    long tiempo_fin;
    char resultado[128];
    int completado;
    int iniciado; // para RR
    // Métricas adicionales
    long tiempo_espera_mutex;
    long inicio_espera_mutex;
} Proceso;

// Declaraciones de funciones
void mostrar_metricas_completas(const char* algoritmo, Proceso procesos[], int n, long tiempo_total_simulacion);
void guardar_metricas_completas(const char* algoritmo, Proceso procesos[], int n, 
                                float prom_espera, float prom_respuesta, float prom_mutex, 
                                float utilizacion_cpu, long tiempo_total);
void generar_operacion_matematica(Proceso* proceso);
void escribir_archivo_avanzado(const char* nombre_archivo, Proceso* proceso, const char* modo);

// ---------- utilidades de pantalla ----------
void limpiar_pantalla() {
    printf("\033[2J\033[H");
}

// ---------- funciones matemáticas para productores ----------
long calcular_factorial(int n) {
    if (n <= 1) return 1;
    long result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

void generar_operacion_matematica(Proceso* proceso) {
    int tipo = rand() % 4; // 4 tipos de operaciones
    
    switch (tipo) {
        case 0: { // Suma
            int a = rand() % 100 + 1;
            int b = rand() % 100 + 1;
            int resultado = a + b;
            snprintf(proceso->resultado, sizeof(proceso->resultado), "%d+%d=%d", a, b, resultado);
            break;
        }
        case 1: { // Multiplicación
            int a = rand() % 20 + 1;
            int b = rand() % 20 + 1;
            int resultado = a * b;
            snprintf(proceso->resultado, sizeof(proceso->resultado), "%d*%d=%d", a, b, resultado);
            break;
        }
        case 2: { // Factorial
            int n = rand() % 8 + 1; // Factorial hasta 8
            long resultado = calcular_factorial(n);
            snprintf(proceso->resultado, sizeof(proceso->resultado), "%d!=%ld", n, resultado);
            break;
        }
        case 3: { // Promedio
            int nums[5];
            int suma = 0;
            for (int i = 0; i < 5; i++) {
                nums[i] = rand() % 50 + 1;
                suma += nums[i];
            }
            double promedio = suma / 5.0;
            snprintf(proceso->resultado, sizeof(proceso->resultado), "prom(%d,%d,%d,%d,%d)=%.2f", 
                    nums[0], nums[1], nums[2], nums[3], nums[4], promedio);
            break;
        }
    }
}

double procesar_operacion(const char* operacion) {
    double resultado = 0.0;
    char op[100];
    strcpy(op, operacion);
    
    // Extraer resultado después del '='
    char* igual = strchr(op, '=');
    if (igual) {
        resultado = atof(igual + 1);
    }
    
    return resultado;
}

// ---------- manejo de archivo compartido en disco ----------
void inicializar_archivo() {
    FILE* f = fopen(nombre_archivo_compartido, "w");
    if (f) fclose(f);
}

// Cuenta líneas actuales del archivo
int contar_lineas_archivo() {
    FILE* f = fopen(nombre_archivo_compartido, "r");
    if (!f) return 0;
    int count = 0;
    char buffer[MAX_LINEA_LEN];
    while (fgets(buffer, sizeof(buffer), f)) count++;
    fclose(f);
    return count;
}

// Escribir (append) una línea si hay espacio (productor)
void escribir_archivo_avanzado(const char* nombre_archivo, Proceso* proceso, const char* modo) {
    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    
    pthread_mutex_lock(&mutex_archivo);
    
    clock_gettime(CLOCK_MONOTONIC, &fin);
    long mutex_wait_time = (fin.tv_sec - inicio.tv_sec) * 1000000L + (fin.tv_nsec - inicio.tv_nsec) / 1000L;
    proceso->tiempo_espera_mutex += mutex_wait_time;
    
    FILE* f = fopen(nombre_archivo, modo);
    if (f) {
        fprintf(f, "P%d (%s): %s [Prio: %d, Dur: %ds, Mutex: %ldμs]\n",
                proceso->id,
                proceso->tipo == 0 ? "Productor" : "Consumidor",
                proceso->resultado,
                proceso->prioridad,
                proceso->duracion,
                mutex_wait_time);
        fclose(f);
    }
    
    pthread_mutex_unlock(&mutex_archivo);
}

// Leer y eliminar última línea (consumidor). Si vacío, retorna "Nada que leer".
void leer_y_eliminar_archivo_avanzado(char* buffer_out, Proceso* proceso) {
    struct timeval inicio, fin;
    gettimeofday(&inicio, NULL);
    
    pthread_mutex_lock(&mutex_archivo);
    
    gettimeofday(&fin, NULL);
    long tiempo_espera = (fin.tv_sec - inicio.tv_sec) * 1000000 + (fin.tv_usec - inicio.tv_usec);
    proceso->tiempo_espera_mutex += tiempo_espera;
    
    FILE* f = fopen(nombre_archivo_compartido, "r");
    if (!f) {
        strcpy(buffer_out, "Nada que leer");
        pthread_mutex_unlock(&mutex_archivo);
        return;
    }
    // Leer todas las líneas en memoria (limitado por MAX_LINEAS_ARCHIVO)
    char (*lineas)[MAX_LINEA_LEN] = malloc(MAX_LINEAS_ARCHIVO * MAX_LINEA_LEN);
    if (!lineas) {
        strcpy(buffer_out, "Error memoria");
        fclose(f);
        pthread_mutex_unlock(&mutex_archivo);
        return;
    }
    int total = 0;
    while (total < MAX_LINEAS_ARCHIVO && fgets(lineas[total], MAX_LINEA_LEN, f)) {
        // quitar '\n'
        lineas[total][strcspn(lineas[total], "\n")] = 0;
        total++;
    }
    fclose(f);
    if (total > 0) {
        // tomar la última línea
        strncpy(buffer_out, lineas[total - 1], MAX_LINEA_LEN - 1);
        buffer_out[MAX_LINEA_LEN - 1] = 0;
        // reescribir archivo sin la última línea
        f = fopen(nombre_archivo_compartido, "w");
        if (f) {
            for (int i = 0; i < total - 1; i++) {
                fprintf(f, "%s\n", lineas[i]);
            }
            fclose(f);
        }
    } else {
        strcpy(buffer_out, "Nada que leer");
    }
    free(lineas);
    pthread_mutex_unlock(&mutex_archivo);
}

// Funciones básicas de compatibilidad
void escribir_archivo(const char* texto) {
    FILE* f = fopen("archivo_compartido.txt", "a");
    if (f) {
        fprintf(f, "%s\n", texto);
        fclose(f);
    }
}

void leer_y_eliminar_archivo(char* buffer_out) {
    FILE* f = fopen("archivo_compartido.txt", "r");
    if (f) {
        if (fgets(buffer_out, 128, f)) {
            // Remover salto de línea
            buffer_out[strcspn(buffer_out, "\n")] = '\0';
        }
        fclose(f);
        
        // Simplemente truncar el archivo para "eliminar"
        f = fopen("archivo_compartido.txt", "w");
        if (f) fclose(f);
    }
}

// ---------- configuración dinámica de procesos ----------
int configurar_procesos_dinamicamente(Proceso procesos[]) {
    int n;
    printf(BOLD YELLOW "\n⚙️  CONFIGURACIÓN DINÁMICA DE PROCESOS\n" RESET);
    printf("═══════════════════════════════════════════════════════\n\n");
    
    printf("📊 Ingrese el número de procesos (1-%d): ", MAX_PROCESOS);
    if (scanf("%d", &n) != 1 || n <= 0 || n > MAX_PROCESOS) {
        printf(RED "❌ Número inválido. Usando configuración por defecto.\n" RESET);
        return 0;
    }
    
    for (int i = 0; i < n; i++) {
        printf(CYAN "\n🔧 Configurando Proceso %d:\n" RESET, i + 1);
        
        procesos[i].id = i + 1;
        
        // Tipo de proceso
        printf("  Tipo (0=Productor, 1=Consumidor): ");
        if (scanf("%d", &procesos[i].tipo) != 1 || (procesos[i].tipo != 0 && procesos[i].tipo != 1)) {
            procesos[i].tipo = i % 2; // Alternar por defecto
            printf(YELLOW "  Valor inválido, usando tipo %s\n" RESET, 
                   procesos[i].tipo == 0 ? "Productor" : "Consumidor");
        }
        
        // Duración
        printf("  Duración (segundos): ");
        if (scanf("%d", &procesos[i].duracion) != 1 || procesos[i].duracion <= 0) {
            procesos[i].duracion = rand() % 5 + 1;
            printf(YELLOW "  Valor inválido, usando duración aleatoria: %d\n" RESET, procesos[i].duracion);
        }
        
        // Tiempo de llegada
        printf("  Tiempo de llegada: ");
        if (scanf("%d", &procesos[i].tiempo_llegada) != 1 || procesos[i].tiempo_llegada < 0) {
            procesos[i].tiempo_llegada = i;
            printf(YELLOW "  Valor inválido, usando tiempo: %d\n" RESET, procesos[i].tiempo_llegada);
        }
        
        // Prioridad
        printf("  Prioridad (menor número = mayor prioridad): ");
        if (scanf("%d", &procesos[i].prioridad) != 1 || procesos[i].prioridad < 0) {
            procesos[i].prioridad = rand() % 5;
            printf(YELLOW "  Valor inválido, usando prioridad aleatoria: %d\n" RESET, procesos[i].prioridad);
        }
        
        // Inicializar campos
        procesos[i].restante = procesos[i].duracion;
        procesos[i].tiempo_inicio = 0;
        procesos[i].tiempo_fin = 0;
        procesos[i].resultado[0] = '\0';
        procesos[i].completado = 0;
        procesos[i].iniciado = 0;
        procesos[i].tiempo_espera_mutex = 0;
        
        printf(GREEN "  ✅ Proceso %d configurado: %s, Dur=%ds, Lleg=%d, Prio=%d\n" RESET,
               procesos[i].id, 
               procesos[i].tipo == 0 ? "🏭 Productor" : "🛒 Consumidor",
               procesos[i].duracion, procesos[i].tiempo_llegada, procesos[i].prioridad);
    }
    
    return n;
}
// Guardar estado completo: número procesos, capacidad, procesos (campos numéricos), luego resultados líneas separadas,
// luego marcador "---ARCHIVO---" y después contenido actual del archivo compartido.
void guardar_estado_completo(const char* archivo_estado, Proceso procesos[], int n) {
    FILE* f = fopen(archivo_estado, "w");
    if (!f) {
        printf(RED "❌ No se pudo abrir %s para guardar.\n" RESET, archivo_estado);
        return;
    }
    fprintf(f, "%d %d\n", n, capacidad_archivo);
    for (int i = 0; i < n; i++) {
        // Guardamos campos numéricos en una línea
        fprintf(f, "%d %d %d %d %d %d %ld %ld %d %d\n",
                procesos[i].id, procesos[i].tipo, procesos[i].duracion,
                procesos[i].tiempo_llegada, procesos[i].prioridad, procesos[i].restante,
                procesos[i].tiempo_inicio, procesos[i].tiempo_fin,
                procesos[i].completado, procesos[i].iniciado);
        // Luego la línea resultado (puede tener espacios o estar vacía)
        if (strlen(procesos[i].resultado) > 0) {
            fprintf(f, "%s\n", procesos[i].resultado);
        } else {
            fprintf(f, "Sin resultado\n");
        }
    }
    // Separador
    fprintf(f, "----ARCHIVO-COMPARTIDO----\n");
    // Volcar archivo compartido actual
    FILE* fc = fopen(nombre_archivo_compartido, "r");
    if (fc) {
        char buf[MAX_LINEA_LEN];
        while (fgets(buf, sizeof(buf), fc)) {
            fputs(buf, f); // ya contiene '\n'
        }
        fclose(fc);
    }
    fclose(f);
    printf(GREEN "✅ Estado completo guardado en '%s'\n" RESET, archivo_estado);
}

// Cargar estado completo. Devuelve cantidad de procesos cargados o 0 si error.
int cargar_estado_completo(const char* archivo_estado, Proceso procesos[]) {
    FILE* f = fopen(archivo_estado, "r");
    if (!f) {
        printf(YELLOW "⚠️  No existe '%s' o no se puede abrir.\n" RESET, archivo_estado);
        return 0;
    }
    int n = 0;
    if (fscanf(f, "%d %d\n", &n, &capacidad_archivo) != 2 || n <= 0) {
        fclose(f);
        printf(RED "❌ Formato inválido en %s (primera línea)\n" RESET, archivo_estado);
        return 0;
    }
    
    printf(CYAN "📖 Cargando %d procesos...\n" RESET, n);
    
    for (int i = 0; i < n; i++) {
        // leer la línea con números
        if (fscanf(f, "%d %d %d %d %d %d %ld %ld %d %d",
                   &procesos[i].id, &procesos[i].tipo, &procesos[i].duracion,
                   &procesos[i].tiempo_llegada, &procesos[i].prioridad, &procesos[i].restante,
                   &procesos[i].tiempo_inicio, &procesos[i].tiempo_fin,
                   &procesos[i].completado, &procesos[i].iniciado) != 10) {
            fclose(f);
            printf(RED "❌ Error leyendo proceso %d en %s\n" RESET, i+1, archivo_estado);
            return 0;
        }
        
        // Saltar cualquier espacio en blanco hasta la siguiente línea
        char c;
        while ((c = fgetc(f)) != EOF && (c == ' ' || c == '\t')) {}
        if (c == '\n') {} // consumir el salto de línea
        else if (c != EOF) ungetc(c, f); // devolver el caracter si no es salto de línea
        
        // leer la línea resultado (puede contener espacios o estar vacía)
        if (fgets(procesos[i].resultado, sizeof(procesos[i].resultado), f)) {
            // quitar '\n' al final de forma segura
            size_t len = strlen(procesos[i].resultado);
            if (len > 0 && procesos[i].resultado[len-1] == '\n') {
                procesos[i].resultado[len-1] = '\0';
            }
        } else {
            procesos[i].resultado[0] = '\0';
        }
        
        printf(CYAN "  ✓ P%d: tipo=%d, dur=%d, lleg=%d, prio=%d\n" RESET, 
               procesos[i].id, procesos[i].tipo, procesos[i].duracion, 
               procesos[i].tiempo_llegada, procesos[i].prioridad);
    }
    
    fclose(f);
    printf(GREEN "📂 Estado cargado exitosamente: %d procesos, capacidad archivo: %d\n" RESET, n, capacidad_archivo);
    return n;
}

// ---------- metricas completas ----------
void mostrar_metricas_completas(const char* algoritmo, Proceso procesos[], int n, long tiempo_total_simulacion) {
    printf("\n" BOLD MAGENTA "📊 MÉTRICAS COMPLETAS - %s:" RESET "\n", algoritmo);
    printf(MAGENTA "┌────┬─────────┬───────────┬─────────────┬──────────────────────────────────────────┐" RESET "\n");
    printf(MAGENTA "│ ID │ Espera  │ Respuesta │ Mutex (μs)  │                Resultado                 │" RESET "\n");
    printf(MAGENTA "├────┼─────────┼───────────┼─────────────┼──────────────────────────────────────────┤" RESET "\n");
    
    long total_espera = 0, total_respuesta = 0, total_mutex = 0;
    long total_tiempo_ejecucion = 0;
    
    for (int i = 0; i < n; i++) {
        long espera = procesos[i].tiempo_inicio - procesos[i].tiempo_llegada;
        long respuesta = procesos[i].tiempo_fin - procesos[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        total_mutex += procesos[i].tiempo_espera_mutex;
        total_tiempo_ejecucion += procesos[i].duracion;
        
        printf(MAGENTA "│ %-2d │ %5ld s │ %7ld s │ %9ld │ %-40s │" RESET "\n",
               procesos[i].id, espera, respuesta, procesos[i].tiempo_espera_mutex,
               procesos[i].resultado);
    }
    
    printf(MAGENTA "└────┴─────────┴───────────┴─────────────┴──────────────────────────────────────────┘" RESET "\n");
    
    // Calcular métricas avanzadas
    float prom_espera = total_espera / (float)n;
    float prom_respuesta = total_respuesta / (float)n;
    float prom_mutex = total_mutex / (float)n / 1000.0; // Convertir a ms
    float utilizacion_cpu = (total_tiempo_ejecucion / (float)tiempo_total_simulacion) * 100.0;
    
    printf(BOLD GREEN "\n📈 RESUMEN DE MÉTRICAS:\n" RESET);
    printf(GREEN "   ⏰ Tiempo promedio de espera:     %.2f segundos\n" RESET, prom_espera);
    printf(GREEN "   ⚡ Tiempo promedio de respuesta:  %.2f segundos\n" RESET, prom_respuesta);
    printf(GREEN "   🔒 Tiempo promedio espera mutex:  %.2f milisegundos\n" RESET, prom_mutex);
    printf(GREEN "   💻 Utilización de CPU:            %.2f%%\n" RESET, utilizacion_cpu);
    printf(GREEN "   🕐 Tiempo total de simulación:    %ld segundos\n\n" RESET, tiempo_total_simulacion);
    
    // Guardar métricas en archivo
    guardar_metricas_completas(algoritmo, procesos, n, prom_espera, prom_respuesta, prom_mutex, utilizacion_cpu, tiempo_total_simulacion);
}

void guardar_metricas_completas(const char* algoritmo, Proceso procesos[], int n, 
                                float prom_espera, float prom_respuesta, float prom_mutex, 
                                float utilizacion_cpu, long tiempo_total) {
    FILE* f = fopen("metricas_completas.txt", "a");
    if (!f) return;

    time_t now = time(NULL);
    fprintf(f, "\n==========================================\n");
    fprintf(f, "Algoritmo: %s\n", algoritmo);
    fprintf(f, "Fecha/Hora: %s", ctime(&now));
    fprintf(f, "Número de procesos: %d\n", n);
    fprintf(f, "Tiempo total simulación: %ld segundos\n", tiempo_total);
    fprintf(f, "------------------------------------------\n");
    
    for (int i = 0; i < n; i++) {
        long espera = procesos[i].tiempo_inicio - procesos[i].tiempo_llegada;
        long respuesta = procesos[i].tiempo_fin - procesos[i].tiempo_llegada;
        fprintf(f, "P%d: Espera=%lds, Respuesta=%lds, Mutex=%ldμs, Resultado=%s\n",
                procesos[i].id, espera, respuesta, procesos[i].tiempo_espera_mutex, procesos[i].resultado);
    }
    
    fprintf(f, "------------------------------------------\n");
    fprintf(f, "Promedio espera: %.2f s\n", prom_espera);
    fprintf(f, "Promedio respuesta: %.2f s\n", prom_respuesta);
    fprintf(f, "Promedio espera mutex: %.2f ms\n", prom_mutex);
    fprintf(f, "Utilización CPU: %.2f%%\n", utilizacion_cpu);
    fprintf(f, "==========================================\n");
    
    fclose(f);
}

// ---------- funciones que usan el archivo compartido en disco ----------
// reemplazan las manipulaciones directas del array archivo_memoria

// Nota: las funciones de planificación usarán escribir_archivo() y leer_y_eliminar_archivo()

// ---------- función principal para ejecutar (no usada en planificadores secuenciales, queda por compatibilidad) ----------
void* ejecutar_proceso(void* arg) {
    Proceso* p = (Proceso*)arg;
    sleep(p->tiempo_llegada);
    struct timeval t_inicio, t_fin;
    gettimeofday(&t_inicio, NULL);
    p->tiempo_inicio = t_inicio.tv_sec;

    printf(CYAN "    🧵 Proceso %d" RESET " (" YELLOW "%s" RESET ") " GREEN "ejecutándose %d s..." RESET "\n",
           p->id, p->tipo == 0 ? "🏭 Productor" : "🛒 Consumidor", p->duracion);

    sleep(p->duracion);

    if (p->tipo == 0) {
        snprintf(p->resultado, sizeof(p->resultado), "Proceso %d escribió: 3 + 5 = 8", p->id);
        escribir_archivo(p->resultado);
    } else {
        leer_y_eliminar_archivo(p->resultado);
    }

    gettimeofday(&t_fin, NULL);
    p->tiempo_fin = t_fin.tv_sec;

    pthread_exit(NULL);
}

// ---------- planificadores (usando archivo en disco) ----------
void planificador_fcfs(Proceso procesos[], int n) {
    printf("\n" BOLD BLUE "🔷 EJECUTANDO FCFS (First Come First Served)" RESET "\n");
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    long tiempo_actual = 0;
    
    time_t inicio_simulacion = time(NULL);
    
    for (int i = 0; i < n; i++) {
        if (tiempo_actual < procesos[i].tiempo_llegada) {
            tiempo_actual = procesos[i].tiempo_llegada;
        }
        procesos[i].tiempo_inicio = tiempo_actual;
        printf(CYAN "    🧵 Proceso %d" RESET " (" YELLOW "%s" RESET ") " GREEN "ejecutándose %d s..." RESET "\n",
               procesos[i].id, procesos[i].tipo == 0 ? "🏭 Productor" : "🛒 Consumidor", procesos[i].duracion);
        
        // Generar operación matemática y simular trabajo
        generar_operacion_matematica(&procesos[i]);
        printf(GREEN "    🔢 Calculando: %s" RESET "\n", procesos[i].resultado);
        
        sleep(procesos[i].duracion);
        tiempo_actual += procesos[i].duracion;
        procesos[i].tiempo_fin = tiempo_actual;

        // Usar archivo avanzado con medición de mutex
        escribir_archivo_avanzado("archivo_compartido.txt", &procesos[i], i == 0 ? "w" : "a");
    }
    
    long tiempo_total_simulacion = time(NULL) - inicio_simulacion;
    
    // Mostrar métricas completas
    mostrar_metricas_completas("FCFS", procesos, n, tiempo_total_simulacion);
}

void planificador_sjn(Proceso procesos[], int n) {
    printf("\n" BOLD BLUE "🔷 EJECUTANDO SJN (Shortest Job Next)" RESET "\n");
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    // Usar malloc en lugar de VLA (Variable Length Array)
    Proceso* copia = malloc(n * sizeof(Proceso));
    if (!copia) {
        printf(RED "❌ Error: No se pudo asignar memoria\n" RESET);
        return;
    }
    
    for (int i = 0; i < n; i++) copia[i] = procesos[i];
    // ordenar por duracion simple
    for (int i = 0; i < n-1; i++) {
        for (int j = i+1; j < n; j++) {
            if (copia[j].duracion < copia[i].duracion) {
                Proceso tmp = copia[i]; copia[i] = copia[j]; copia[j] = tmp;
            }
        }
    }
    time_t inicio_simulacion = time(NULL);
    long tiempo_actual = 0;
    
    for (int i = 0; i < n; i++) {
        if (tiempo_actual < copia[i].tiempo_llegada) tiempo_actual = copia[i].tiempo_llegada;
        copia[i].tiempo_inicio = tiempo_actual;
        printf(CYAN "    🧵 Proceso %d" RESET " (" YELLOW "%s" RESET ") " GREEN "ejecutándose %d s..." RESET "\n",
               copia[i].id, copia[i].tipo == 0 ? "🏭 Productor" : "🛒 Consumidor", copia[i].duracion);
        
        // Generar operación matemática
        generar_operacion_matematica(&copia[i]);
        printf(GREEN "    🔢 Calculando: %s" RESET "\n", copia[i].resultado);
        
        sleep(copia[i].duracion);
        tiempo_actual += copia[i].duracion;
        copia[i].tiempo_fin = tiempo_actual;
        
        // Usar archivo avanzado
        escribir_archivo_avanzado("archivo_compartido.txt", &copia[i], i == 0 ? "w" : "a");
    }
    
    long tiempo_total_simulacion = time(NULL) - inicio_simulacion;
    
    // Mostrar métricas completas
    mostrar_metricas_completas("SJN", copia, n, tiempo_total_simulacion);
    
    free(copia);
}

void planificador_prioridad_sin_desalojo(Proceso procesos[], int n) {
    printf("\n" BOLD BLUE "🔷 EJECUTANDO PRIORIDAD (sin desalojo)" RESET "\n");
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    // Usar malloc en lugar de VLA
    Proceso* copia = malloc(n * sizeof(Proceso));
    if (!copia) {
        printf(RED "❌ Error: No se pudo asignar memoria\n" RESET);
        return;
    }
    
    for (int i = 0; i < n; i++) copia[i] = procesos[i];
    // ordenar por prioridad ascendente (menor = más alta prioridad)
    for (int i = 0; i < n-1; i++)
        for (int j = i+1; j < n; j++)
            if (copia[j].prioridad < copia[i].prioridad) {
                Proceso tmp = copia[i]; copia[i] = copia[j]; copia[j] = tmp;
            }
    time_t inicio_simulacion = time(NULL);
    long tiempo_actual = 0;
    
    for (int i = 0; i < n; i++) {
        if (tiempo_actual < copia[i].tiempo_llegada) tiempo_actual = copia[i].tiempo_llegada;
        copia[i].tiempo_inicio = tiempo_actual;
        printf(CYAN "    🧵 Proceso %d" RESET " (" YELLOW "%s" RESET ") " GREEN "ejecutándose %d s... " MAGENTA "[Prio %d]" RESET "\n",
               copia[i].id, copia[i].tipo == 0 ? "🏭 Productor" : "🛒 Consumidor", copia[i].duracion, copia[i].prioridad);
        
        // Generar operación matemática
        generar_operacion_matematica(&copia[i]);
        printf(GREEN "    🔢 Calculando: %s" RESET "\n", copia[i].resultado);
        
        sleep(copia[i].duracion);
        tiempo_actual += copia[i].duracion;
        copia[i].tiempo_fin = tiempo_actual;
        
        // Usar archivo avanzado
        escribir_archivo_avanzado("archivo_compartido.txt", &copia[i], i == 0 ? "w" : "a");
    }
    
    long tiempo_total_simulacion = time(NULL) - inicio_simulacion;
    
    // Mostrar métricas completas
    mostrar_metricas_completas("Prioridad (sin desalojo)", copia, n, tiempo_total_simulacion);
    
    free(copia);
}

void planificador_prioridad_con_desalojo(Proceso procesos[], int n) {
    printf("\n" BOLD BLUE "🔷 EJECUTANDO PRIORIDAD (con desalojo)" RESET "\n");
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    int tiempo = 0, completados = 0;
    // Usar malloc en lugar de VLA
    int* restantes = malloc(n * sizeof(int));
    if (!restantes) {
        printf(RED "❌ Error: No se pudo asignar memoria\n" RESET);
        return;
    }
    
    for (int i = 0; i < n; i++) restantes[i] = procesos[i].duracion;

    while (completados < n) {
        int mejor = -1;
        for (int i = 0; i < n; i++) {
            if (!procesos[i].completado && procesos[i].tiempo_llegada <= tiempo) {
                if (mejor == -1 || procesos[i].prioridad < procesos[mejor].prioridad) {
                    mejor = i;
                }
            }
        }
        if (mejor != -1) {
            if (!procesos[mejor].iniciado) {
                procesos[mejor].tiempo_inicio = tiempo;
                procesos[mejor].iniciado = 1;
            }
            printf(YELLOW "⏰ Tiempo %d" RESET " → " CYAN "Ejecutando P%d" RESET " (" YELLOW "%s" RESET ") " MAGENTA "[Prioridad %d]" RESET "\n",
                   tiempo, procesos[mejor].id,
                   procesos[mejor].tipo == 0 ? "🏭 Prod" : "🛒 Cons",
                   procesos[mejor].prioridad);
            sleep(1);
            restantes[mejor]--;
            if (restantes[mejor] <= 0) {
                procesos[mejor].completado = 1;
                procesos[mejor].tiempo_fin = tiempo + 1;
                
                // Generar operación matemática al completar
                generar_operacion_matematica(&procesos[mejor]);
                printf(GREEN "    🔢 Resultado: %s" RESET "\n", procesos[mejor].resultado);
                
                // Usar archivo avanzado
                escribir_archivo_avanzado("archivo_compartido.txt", &procesos[mejor], completados == 0 ? "w" : "a");
                
                completados++;
                printf(GREEN "    ✅ Proceso P%d completado!" RESET "\n", procesos[mejor].id);
            }
        } else {
            printf(RED "⏰ Tiempo %d" RESET " → " RED "CPU inactivo" RESET "\n", tiempo);
        }
        tiempo++;
    }
    
    long tiempo_total_simulacion = tiempo;
    
    // Mostrar métricas completas
    mostrar_metricas_completas("Prioridad (con desalojo)", procesos, n, tiempo_total_simulacion);
    
    free(restantes);
}

void planificador_rr(Proceso procesos[], int n) {
    // Solicitar quantum al usuario
    printf(BOLD YELLOW "\n⚙️  CONFIGURACIÓN ROUND ROBIN\n" RESET);
    printf("🔁 Ingrese el quantum (en segundos, ej. 2): ");
    if (scanf("%d", &quantum_usuario) != 1 || quantum_usuario <= 0) {
        quantum_usuario = 2;
        printf(YELLOW "Valor inválido, usando quantum por defecto = 2 segundos\n" RESET);
    }
    
    printf("\n" BOLD BLUE "🔷 EJECUTANDO ROUND ROBIN (Quantum = %d s)" RESET "\n", quantum_usuario);
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    int tiempo_actual = 0;
    int terminados = 0;
    
    // Continuar hasta que todos los procesos terminen
    while (terminados < n) {
        int progreso = 0; // Flag para verificar si hubo progreso en esta ronda
        
        for (int i = 0; i < n; i++) {
            // Saltear procesos ya terminados
            if (procesos[i].restante <= 0) continue;
            
            // Saltear procesos que aún no han llegado
            if (procesos[i].tiempo_llegada > tiempo_actual) continue;
            
            // Inicializar tiempo de inicio si es necesario
            if (!procesos[i].iniciado) {
                procesos[i].tiempo_inicio = tiempo_actual;
                procesos[i].iniciado = 1;
            }
            
            // Calcular tiempo a ejecutar
            int tiempo_exec = (procesos[i].restante <= quantum_usuario) ? procesos[i].restante : quantum_usuario;
            
            printf(CYAN "🧵 Proceso P%d" RESET " (" YELLOW "%s" RESET ") → " GREEN "ejecuta %d s" RESET " (restante: " MAGENTA "%d→%d" RESET ")\n",
                   procesos[i].id,
                   procesos[i].tipo == 0 ? "Prod" : "Cons",
                   tiempo_exec, 
                   procesos[i].restante,
                   procesos[i].restante - tiempo_exec);
            
            // Ejecutar el proceso
            sleep(tiempo_exec);
            procesos[i].restante -= tiempo_exec;
            tiempo_actual += tiempo_exec;
            progreso = 1;
            
            // Verificar si el proceso terminó
            if (procesos[i].restante <= 0) {
                procesos[i].tiempo_fin = tiempo_actual;
                
                // Generar operación matemática al terminar
                generar_operacion_matematica(&procesos[i]);
                printf(GREEN "    🔢 Resultado final: %s" RESET "\n", procesos[i].resultado);
                
                // Usar archivo avanzado
                escribir_archivo_avanzado("archivo_compartido.txt", &procesos[i], terminados == 0 ? "w" : "a");
                
                terminados++;
                printf(GREEN "    ✅ Proceso P%d TERMINADO!" RESET "\n", procesos[i].id);
            }
        }
        
        // Si no hubo progreso, incrementar tiempo para procesos que no han llegado
        if (!progreso) {
            tiempo_actual++;
        }
    }
    
    long tiempo_total_simulacion = tiempo_actual;
    
    // Mostrar métricas completas
    mostrar_metricas_completas("Round Robin", procesos, n, tiempo_total_simulacion);
}

// ---------- menú ----------
void menu() {
    pthread_mutex_init(&mutex_archivo, NULL);

    // Configuración inicial del sistema
    printf("⚙️  CONFIGURACIÓN INICIAL DEL SISTEMA\n");
    printf("═══════════════════════════════════════\n\n");
    
    // Pedir capacidad del archivo al inicio
    printf("📦 Ingrese capacidad máxima del archivo compartido (líneas, ej. 10): ");
    if (scanf("%d", &capacidad_archivo) != 1 || capacidad_archivo <= 0) {
        capacidad_archivo = 10;
        printf(YELLOW "Valor inválido, usando capacidad por defecto = 10\n" RESET);
    }
    
    printf(GREEN "✅ Configuración completada:\n");
    printf("   📦 Capacidad archivo: %d líneas\n\n" RESET, capacidad_archivo);
    
    inicializar_archivo();

    // Proceso ejemplo por defecto (puedes cambiar o cargar)
    Proceso procesos[] = {
        {1, 0, 4, 0, 2, 4, 0, 0, "", 0, 0},
        {2, 1, 3, 1, 1, 3, 0, 0, "", 0, 0},
        {3, 0, 2, 2, 3, 2, 0, 0, "", 0, 0},
        {4, 1, 1, 3, 0, 1, 0, 0, "", 0, 0}
    };
    int cantidad = sizeof(procesos)/sizeof(Proceso);

    // Preguntar si carga estado
    printf("¿Desea cargar un estado previo? (1=Sí, 0=No): ");
    int cargar;
    if (scanf("%d", &cargar) == 1 && cargar == 1) {
        int n = cargar_estado_completo("estado_simulacion.txt", procesos);
        if (n > 0) cantidad = n;
    }

    int opcion;
    do {
        // Resetear procesos para cada nueva ejecución (si no fueron cargados)
        for (int i = 0; i < cantidad; i++) {
            procesos[i].restante = procesos[i].duracion;
            procesos[i].tiempo_inicio = 0;
            procesos[i].tiempo_fin = 0;
            procesos[i].resultado[0] = '\0';
            procesos[i].completado = 0;
            procesos[i].iniciado = 0;
        }
        inicializar_archivo(); // limpiar archivo antes de cada corrida (si prefieres no hacerlo, quita esta línea)

        limpiar_pantalla();
        printf("\n╔══════════════════════════════════════════════════════════════╗\n");
        printf("║              🧵 SIMULADOR DE PLANIFICACIÓN DE PROCESOS       ║\n");
        printf("╚══════════════════════════════════════════════════════════════╝\n");
        printf("\n");

        printf("\n🎯 SELECCIONE EL ALGORITMO DE PLANIFICACIÓN:\n");
        printf("┌─────┬────────────────────────────────────────────┐\n");
        printf("│  1  │ 📝 FCFS (First Come First Served)         │\n");
        printf("│  2  │ ⚡ SJN (Shortest Job Next)                │\n");
        printf("│  3  │ 🏆 Prioridad (sin desalojo)               │\n");
        printf("│  4  │ 🔄 Prioridad (con desalojo)               │\n");
        printf("│  5  │ 🔁 Round Robin                            │\n");
        printf("│  6  │ 💾 Guardar estado actual                  │\n");
        printf("│  7  │ 📂 Cargar estado desde archivo            │\n");
        printf("│  8  │ ⚙️  Configuración dinámica de procesos    │\n");
        printf("│  0  │ 🚪 Salir del programa                     │\n");
        printf("└─────┴────────────────────────────────────────────┘\n");
        printf("💭 Ingrese su opción (0-8): ");
        if (scanf("%d", &opcion) != 1) { opcion = -1; while(getchar()!='\n'); }

        switch (opcion) {
            case 1:
                planificador_fcfs(procesos, cantidad);
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar(); getchar();
                break;
            case 2:
                planificador_sjn(procesos, cantidad);
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar(); getchar();
                break;
            case 3:
                planificador_prioridad_sin_desalojo(procesos, cantidad);
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar(); getchar();
                break;
            case 4:
                planificador_prioridad_con_desalojo(procesos, cantidad);
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar(); getchar();
                break;
            case 5:
                planificador_rr(procesos, cantidad);
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar(); getchar();
                break;
            case 6:
                guardar_estado_completo("estado_simulacion.txt", procesos, cantidad);
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar(); getchar();
                break;
            case 7: {
                int n = cargar_estado_completo("estado_simulacion.txt", procesos);
                if (n > 0) cantidad = n;
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar(); getchar();
                break;
            }
            case 8: {
                cantidad = configurar_procesos_dinamicamente(procesos);
                if (cantidad > 0) {
                    printf(BOLD GREEN "\n✅ Configuración completada. %d procesos configurados.\n" RESET, cantidad);
                } else {
                    printf(BOLD RED "\n❌ Error en configuración. Manteniendo procesos actuales.\n" RESET);
                }
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar(); getchar();
                break;
            }
            case 0:
                printf(BOLD GREEN "\n👋 Saliendo..." RESET "\n");
                break;
            default:
                printf(BOLD RED "❌ Opción inválida. Por favor seleccione una opción del 0 al 8." RESET "\n");
                printf(BOLD CYAN "🔄 Presione ENTER para continuar..." RESET);
                getchar(); getchar();
        }

    } while (opcion != 0);

    pthread_mutex_destroy(&mutex_archivo);
}

// ---------- main ----------
int main() {
    menu();
    return 0;
}
