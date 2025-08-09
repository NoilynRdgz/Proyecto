#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

#define MAX_PROCESOS 10
#define QUANTUM 2

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
    char resultado[64];
    int completado;
    int iniciado; // para RR
} Proceso;

// Simulación del archivo compartido
char archivo_memoria[MAX_PROCESOS][64];
int tope = 0;

// Función para limpiar pantalla
void limpiar_pantalla() {
    printf("\033[2J\033[H");
}

// Guardar estado actual de procesos y archivo compartido
void guardar_estado(const char* nombre_archivo, Proceso procesos[], int n) {
    FILE* f = fopen(nombre_archivo, "w");
    if (!f) return;
    fprintf(f, "%d\n", n);
    for (int i = 0; i < n; i++) {
        fprintf(f, "%d %d %d %d %d %d %ld %ld %d %d %s\n",
                procesos[i].id, procesos[i].tipo, procesos[i].duracion,
                procesos[i].tiempo_llegada, procesos[i].prioridad, procesos[i].restante,
                procesos[i].tiempo_inicio, procesos[i].tiempo_fin,
                procesos[i].completado, procesos[i].iniciado,
                procesos[i].resultado);
    }
    fprintf(f, "%d\n", tope);
    for (int i = 0; i < tope; i++) {
        fprintf(f, "%s\n", archivo_memoria[i]);
    }
    fclose(f);
    printf("✅ Estado guardado en '%s'\n", nombre_archivo);
}

// Cargar estado desde archivo
int cargar_estado(const char* nombre_archivo, Proceso procesos[]) {
    FILE* f = fopen(nombre_archivo, "r");
    if (!f) {
        printf("⚠️  No se pudo abrir el archivo de estado.\n");
        return 0;
    }
    int n;
    fscanf(f, "%d", &n);
    for (int i = 0; i < n; i++) {
        fscanf(f, "%d %d %d %d %d %d %ld %ld %d %d %s \n",
               &procesos[i].id, &procesos[i].tipo, &procesos[i].duracion,
               &procesos[i].tiempo_llegada, &procesos[i].prioridad, &procesos[i].restante,
               &procesos[i].tiempo_inicio, &procesos[i].tiempo_fin,
               &procesos[i].completado, &procesos[i].iniciado,
               procesos[i].resultado);
    }
    fscanf(f, "%d", &tope);
    for (int i = 0; i < tope; i++) {
        fscanf(f, " %s\n", archivo_memoria[i]);
    }
    fclose(f);
    printf("📂 Estado cargado desde '%s' con %d procesos.\n", nombre_archivo, n);
    return n;
}

void guardar_metricas(const char* algoritmo, Proceso procesos[], int n, float prom_espera, float prom_respuesta) {
    FILE* f = fopen("metricas.txt", "a");
    if (!f) return;

    fprintf(f, "\n==============================\n");
    fprintf(f, "Algoritmo: %s\n", algoritmo);
    for (int i = 0; i < n; i++) {
        long espera = procesos[i].tiempo_inicio - procesos[i].tiempo_llegada;
        long respuesta = procesos[i].tiempo_fin - procesos[i].tiempo_llegada;
        fprintf(f, "Proceso %d → Espera: %ld s | Respuesta: %ld s | Resultado: %s\n",
                procesos[i].id, espera, respuesta, procesos[i].resultado);
    }
    fprintf(f, "Promedio espera: %.2f s\n", prom_espera);
    fprintf(f, "Promedio respuesta: %.2f s\n", prom_respuesta);
    fclose(f);
}

// Función general para procesos (usado por FCFS, SJN, PRIO sin desalojo)
void* ejecutar_proceso(void* arg) {
    Proceso* p = (Proceso*)arg;

    sleep(p->tiempo_llegada);
    struct timeval t_inicio, t_fin;
    gettimeofday(&t_inicio, NULL);
    p->tiempo_inicio = t_inicio.tv_sec;

    printf(CYAN "    🧵 Proceso %d" RESET " (" YELLOW "%s" RESET ") " GREEN "ejecutándose %d s..." RESET "\n",
           p->id, p->tipo == 0 ? "🏭 Productor" : "🛒 Consumidor", p->duracion);

    sleep(p->duracion);

    pthread_mutex_lock(&mutex_archivo);
    if (p->tipo == 0) {
        sprintf(p->resultado, "Proceso %d escribió: 3 + 5 = 8", p->id);
        strcpy(archivo_memoria[tope++], p->resultado);
    } else {
        if (tope > 0) strcpy(p->resultado, archivo_memoria[--tope]);
        else strcpy(p->resultado, "Nada que leer");
    }
    pthread_mutex_unlock(&mutex_archivo);

    gettimeofday(&t_fin, NULL);
    p->tiempo_fin = t_fin.tv_sec;

    pthread_exit(NULL);
}

void planificador_fcfs(Proceso procesos[], int n) {
    printf("\n" BOLD BLUE "🔷 EJECUTANDO FCFS (First Come First Served)" RESET "\n");
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    pthread_t hilos[n];
    for (int i = 0; i < n; i++) {
        pthread_create(&hilos[i], NULL, ejecutar_proceso, &procesos[i]);
        pthread_join(hilos[i], NULL);
    }
    
    printf("\n" BOLD MAGENTA "📊 MÉTRICAS FCFS:" RESET "\n");
    printf(MAGENTA "┌────┬─────────┬───────────┬──────────────────────────────────────────┐" RESET "\n");
    printf(MAGENTA "│ ID │ Espera  │ Respuesta │                Resultado                 │" RESET "\n");
    printf(MAGENTA "├────┼─────────┼───────────┼──────────────────────────────────────────┤" RESET "\n");
    
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = procesos[i].tiempo_inicio - procesos[i].tiempo_llegada;
        long respuesta = procesos[i].tiempo_fin - procesos[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf(MAGENTA "│" RESET "%3d " MAGENTA "│" RESET "%6ld s " MAGENTA "│" RESET "%8ld s " MAGENTA "│" RESET " %-39s " MAGENTA "│" RESET "\n",
               procesos[i].id, espera, respuesta, procesos[i].resultado);
    }
    
    printf(MAGENTA "└────┴─────────┴───────────┴──────────────────────────────────────────┘" RESET "\n");
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf(BOLD GREEN "⏳ Promedio espera: %.2f s" RESET " | " BOLD GREEN "⏳ Promedio respuesta: %.2f s" RESET "\n\n", prom_esp, prom_resp);
    guardar_metricas("FCFS", procesos, n, prom_esp, prom_resp);
}

int comparar_duracion(const void* a, const void* b) {
    Proceso* p1 = (Proceso*)a;
    Proceso* p2 = (Proceso*)b;
    return p1->duracion - p2->duracion;
}

void planificador_sjn(Proceso procesos[], int n) {
    printf("\n" BOLD BLUE "🔷 EJECUTANDO SJN (Shortest Job Next)" RESET "\n");
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    Proceso copia[n];
    for (int i = 0; i < n; i++) copia[i] = procesos[i];
    
    // Ordenar por duración
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (copia[j].duracion < copia[i].duracion) {
                Proceso temp = copia[i];
                copia[i] = copia[j];
                copia[j] = temp;
            }
        }
    }

    pthread_t hilos[n];
    for (int i = 0; i < n; i++) {
        pthread_create(&hilos[i], NULL, ejecutar_proceso, &copia[i]);
        pthread_join(hilos[i], NULL);
    }
    
    printf("\n" BOLD MAGENTA "📊 MÉTRICAS SJN:" RESET "\n");
    printf(MAGENTA "┌────┬─────────┬───────────┬──────────────────────────────────────────┐" RESET "\n");
    printf(MAGENTA "│ ID │ Espera  │ Respuesta │                Resultado                 │" RESET "\n");
    printf(MAGENTA "├────┼─────────┼───────────┼──────────────────────────────────────────┤" RESET "\n");
    
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = copia[i].tiempo_inicio - copia[i].tiempo_llegada;
        long respuesta = copia[i].tiempo_fin - copia[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf(MAGENTA "│" RESET "%3d " MAGENTA "│" RESET "%6ld s " MAGENTA "│" RESET "%8ld s " MAGENTA "│" RESET " %-39s " MAGENTA "│" RESET "\n",
               copia[i].id, espera, respuesta, copia[i].resultado);
    }
    
    printf(MAGENTA "└────┴─────────┴───────────┴──────────────────────────────────────────┘" RESET "\n");
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf(BOLD GREEN "⏳ Promedio espera: %.2f s" RESET " | " BOLD GREEN "⏳ Promedio respuesta: %.2f s" RESET "\n\n", prom_esp, prom_resp);
    guardar_metricas("SJN", copia, n, prom_esp, prom_resp);
}

int comparar_prioridad(const void* a, const void* b) {
    Proceso* p1 = (Proceso*)a;
    Proceso* p2 = (Proceso*)b;
    return p1->prioridad - p2->prioridad;
}

void planificador_prioridad_sin_desalojo(Proceso procesos[], int n) {
    printf("\n" BOLD BLUE "🔷 EJECUTANDO PRIORIDAD (sin desalojo)" RESET "\n");
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    Proceso copia[n];
    for (int i = 0; i < n; i++) copia[i] = procesos[i];
    qsort(copia, n, sizeof(Proceso), comparar_prioridad);

    pthread_t hilos[n];
    for (int i = 0; i < n; i++) {
        pthread_create(&hilos[i], NULL, ejecutar_proceso, &copia[i]);
        pthread_join(hilos[i], NULL);
    }
    
    printf("\n" BOLD MAGENTA "📊 MÉTRICAS PRIORIDAD (sin desalojo):" RESET "\n");
    printf(MAGENTA "┌────┬─────────┬───────────┬──────────────────────────────────────────┐" RESET "\n");
    printf(MAGENTA "│ ID │ Espera  │ Respuesta │                Resultado                 │" RESET "\n");
    printf(MAGENTA "├────┼─────────┼───────────┼──────────────────────────────────────────┤" RESET "\n");
    
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = copia[i].tiempo_inicio - copia[i].tiempo_llegada;
        long respuesta = copia[i].tiempo_fin - copia[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf(MAGENTA "│" RESET "%3d " MAGENTA "│" RESET "%6ld s " MAGENTA "│" RESET "%8ld s " MAGENTA "│" RESET " %-39s " MAGENTA "│" RESET "\n",
               copia[i].id, espera, respuesta, copia[i].resultado);
    }
    
    printf(MAGENTA "└────┴─────────┴───────────┴──────────────────────────────────────────┘" RESET "\n");
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf(BOLD GREEN "⏳ Promedio espera: %.2f s" RESET " | " BOLD GREEN "⏳ Promedio respuesta: %.2f s" RESET "\n\n", prom_esp, prom_resp);
    guardar_metricas("Prioridad (sin desalojo)", copia, n, prom_esp, prom_resp);
}
void planificador_prioridad_con_desalojo(Proceso procesos[], int n) {
    printf("\n" BOLD BLUE "🔷 EJECUTANDO PRIORIDAD (con desalojo)" RESET "\n");
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    int tiempo = 0, completados = 0;
    int restantes[n];
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
                struct timeval t;
                gettimeofday(&t, NULL);
                procesos[mejor].tiempo_inicio = t.tv_sec;
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
                struct timeval tf;
                gettimeofday(&tf, NULL);
                procesos[mejor].tiempo_fin = tf.tv_sec;
                pthread_mutex_lock(&mutex_archivo);
                if (procesos[mejor].tipo == 0) {
                    sprintf(procesos[mejor].resultado, "P%d escribió: 2*3=6", procesos[mejor].id);
                    strcpy(archivo_memoria[tope++], procesos[mejor].resultado);
                } else {
                    if (tope > 0) strcpy(procesos[mejor].resultado, archivo_memoria[--tope]);
                    else strcpy(procesos[mejor].resultado, "Nada que leer");
                }
                pthread_mutex_unlock(&mutex_archivo);
                completados++;
                printf(GREEN "    ✅ Proceso P%d completado!" RESET "\n", procesos[mejor].id);
            }
        } else {
            printf(RED "⏰ Tiempo %d" RESET " → " RED "CPU inactivo" RESET "\n", tiempo);
            sleep(1);
        }
        tiempo++;
    }
    
    printf("\n" BOLD MAGENTA "📊 MÉTRICAS PRIORIDAD (con desalojo):" RESET "\n");
    printf(MAGENTA "┌────┬─────────┬───────────┬──────────────────────────────────────────┐" RESET "\n");
    printf(MAGENTA "│ ID │ Espera  │ Respuesta │                Resultado                 │" RESET "\n");
    printf(MAGENTA "├────┼─────────┼───────────┼──────────────────────────────────────────┤" RESET "\n");
    
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = procesos[i].tiempo_inicio - procesos[i].tiempo_llegada;
        long respuesta = procesos[i].tiempo_fin - procesos[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf(MAGENTA "│" RESET "%3d " MAGENTA "│" RESET "%6ld s " MAGENTA "│" RESET "%8ld s " MAGENTA "│" RESET " %-39s " MAGENTA "│" RESET "\n",
               procesos[i].id, espera, respuesta, procesos[i].resultado);
    }
    
    printf(MAGENTA "└────┴─────────┴───────────┴──────────────────────────────────────────┘" RESET "\n");
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf(BOLD GREEN "⏳ Promedio espera: %.2f s" RESET " | " BOLD GREEN "⏳ Promedio respuesta: %.2f s" RESET "\n\n", prom_esp, prom_resp);
    guardar_metricas("Prioridad (con desalojo)", procesos, n, prom_esp, prom_resp);
}


void planificador_rr(Proceso procesos[], int n) {
    printf("\n" BOLD BLUE "🔷 EJECUTANDO ROUND ROBIN (Quantum = %d s)" RESET "\n", QUANTUM);
    printf(CYAN "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" RESET "\n\n");
    
    int procesos_restantes = n;
    int tiempo = 0;

    while (procesos_restantes > 0) {
        for (int i = 0; i < n; i++) {
            if (procesos[i].tiempo_llegada <= tiempo && procesos[i].restante > 0) {
                if (!procesos[i].iniciado) {
                    struct timeval t;
                    gettimeofday(&t, NULL);
                    procesos[i].tiempo_inicio = t.tv_sec;
                    procesos[i].iniciado = 1;
                }
                int ejecutar = procesos[i].restante < QUANTUM ? procesos[i].restante : QUANTUM;
                printf(CYAN "🧵 Proceso %d" RESET " (" YELLOW "%s" RESET ") " GREEN "ejecuta %d s" RESET " (restante: " MAGENTA "%d" RESET ")\n",
                       procesos[i].id,
                       procesos[i].tipo == 0 ? "🏭 Productor" : "🛒 Consumidor",
                       ejecutar, procesos[i].restante);
                sleep(ejecutar);
                procesos[i].restante -= ejecutar;
                if (procesos[i].restante <= 0) {
                    struct timeval tf;
                    gettimeofday(&tf, NULL);
                    procesos[i].tiempo_fin = tf.tv_sec;
                    pthread_mutex_lock(&mutex_archivo);
                    if (procesos[i].tipo == 0) {
                        sprintf(procesos[i].resultado, "Proceso %d escribió: 3 * 5 = 15", procesos[i].id);
                        strcpy(archivo_memoria[tope++], procesos[i].resultado);
                    } else {
                        if (tope > 0) strcpy(procesos[i].resultado, archivo_memoria[--tope]);
                        else strcpy(procesos[i].resultado, "Nada que leer");
                    }
                    pthread_mutex_unlock(&mutex_archivo);
                    procesos_restantes--;
                    printf(GREEN "    ✅ Proceso %d completado!" RESET "\n", procesos[i].id);
                }
            }
        }
        tiempo++;
    }
    
    printf("\n" BOLD MAGENTA "📊 MÉTRICAS ROUND ROBIN:" RESET "\n");
    printf(MAGENTA "┌────┬─────────┬───────────┬──────────────────────────────────────────┐" RESET "\n");
    printf(MAGENTA "│ ID │ Espera  │ Respuesta │                Resultado                 │" RESET "\n");
    printf(MAGENTA "├────┼─────────┼───────────┼──────────────────────────────────────────┤" RESET "\n");
    
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = procesos[i].tiempo_inicio - procesos[i].tiempo_llegada;
        long respuesta = procesos[i].tiempo_fin - procesos[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf(MAGENTA "│" RESET "%3d " MAGENTA "│" RESET "%6ld s " MAGENTA "│" RESET "%8ld s " MAGENTA "│" RESET " %-39s " MAGENTA "│" RESET "\n",
               procesos[i].id, espera, respuesta, procesos[i].resultado);
    }
    
    printf(MAGENTA "└────┴─────────┴───────────┴──────────────────────────────────────────┘" RESET "\n");
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf(BOLD GREEN "⏳ Promedio espera: %.2f s" RESET " | " BOLD GREEN "⏳ Promedio respuesta: %.2f s" RESET "\n\n", prom_esp, prom_resp);
    guardar_metricas("Round Robin", procesos, n, prom_esp, prom_resp);
}

void menu() {
    pthread_mutex_init(&mutex_archivo, NULL);

    Proceso procesos[] = {
        {1, 0, 4, 0, 2, 4, 0, 0, "", 0, 0},
        {2, 1, 3, 1, 1, 3, 0, 0, "", 0, 0},
        {3, 0, 2, 2, 3, 2, 0, 0, "", 0, 0},
        {4, 1, 1, 3, 0, 1, 0, 0, "", 0, 0}
    };
    int cantidad = sizeof(procesos)/sizeof(Proceso);

    int opcion;
    
    do {
        // Limpiar pantalla y mostrar título
        limpiar_pantalla();
        printf("\n");
        printf("╔══════════════════════════════════════════════════════════════╗\n");
        printf("║              🧵 SIMULADOR DE PLANIFICACIÓN DE PROCESOS       ║\n");
        printf("╚══════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        
        // Resetear procesos para cada nueva ejecución
        for (int i = 0; i < cantidad; i++) {
            procesos[i].restante = procesos[i].duracion;
            procesos[i].tiempo_inicio = 0;
            procesos[i].tiempo_fin = 0;
            strcpy(procesos[i].resultado, "");
            procesos[i].completado = 0;
            procesos[i].iniciado = 0;
        }
        tope = 0; // Resetear archivo compartido
        
        printf("\n🎯 SELECCIONE EL ALGORITMO DE PLANIFICACIÓN:\n");
        printf("┌─────┬────────────────────────────────────────────┐\n");
        printf("│  1  │ 📝 FCFS (First Come First Served)         │\n");
        printf("│  2  │ ⚡ SJN (Shortest Job Next)                │\n");
        printf("│  3  │ 🏆 Prioridad (sin desalojo)               │\n");
        printf("│  4  │ 🔄 Prioridad (con desalojo)               │\n");
        printf("│  5  │ 🔁 Round Robin                            │\n");
        printf("│  0  │ 🚪 Salir del programa                     │\n");
        printf("└─────┴────────────────────────────────────────────┘\n");
        printf("💭 Ingrese su opción (0-5): ");
        scanf("%d", &opcion);

        switch (opcion) {
            case 1: 
                planificador_fcfs(procesos, cantidad); 
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar(); // Para consumir el \n pendiente
                getchar(); // Para esperar el ENTER del usuario
                break;
            case 2: 
                planificador_sjn(procesos, cantidad); 
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar();
                getchar();
                break;
            case 3: 
                planificador_prioridad_sin_desalojo(procesos, cantidad); 
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar();
                getchar();
                break;
            case 4: 
                planificador_prioridad_con_desalojo(procesos, cantidad); 
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar();
                getchar();
                break;
            case 5: 
                planificador_rr(procesos, cantidad); 
                printf(BOLD CYAN "\n🔄 Presione ENTER para volver al menú..." RESET);
                getchar();
                getchar();
                break;
            case 0:
                printf(BOLD GREEN "\n👋 Saliendo..." RESET "\n");
                break;
            default: 
                printf(BOLD RED "❌ Opción inválida. Por favor seleccione una opción del 0 al 5." RESET "\n");
                printf(BOLD CYAN "🔄 Presione ENTER para continuar..." RESET);
                getchar();
                getchar();
        }
        
    } while (opcion != 0);

    pthread_mutex_destroy(&mutex_archivo);
}

int main() {
    menu();
    return 0;
}
