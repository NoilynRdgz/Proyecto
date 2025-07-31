#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

#define MAX_PROCESOS 10
#define QUANTUM 2

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

    printf("\n🧵 Proceso %d (%s) ejecutándose %d s...\n",
           p->id, p->tipo == 0 ? "Productor" : "Consumidor", p->duracion);

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
    printf("\n🔷 Ejecutando FCFS con hilos...\n");
    pthread_t hilos[n];
    for (int i = 0; i < n; i++) {
        pthread_create(&hilos[i], NULL, ejecutar_proceso, &procesos[i]);
        pthread_join(hilos[i], NULL);
    }
    printf("\n📊 Métricas FCFS:\n");
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = procesos[i].tiempo_inicio - procesos[i].tiempo_llegada;
        long respuesta = procesos[i].tiempo_fin - procesos[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf("Proceso %d → Espera: %ld s | Respuesta: %ld s | Resultado: %s\n",
               procesos[i].id, espera, respuesta, procesos[i].resultado);
    }
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf("⏳ Promedio espera: %.2f s\n", prom_esp);
    printf("⏳ Promedio respuesta: %.2f s\n", prom_resp);
    guardar_metricas("FCFS", procesos, n, prom_esp, prom_resp);
}

int comparar_duracion(const void* a, const void* b) {
    Proceso* p1 = (Proceso*)a;
    Proceso* p2 = (Proceso*)b;
    return p1->duracion - p2->duracion;
}

void planificador_sjn(Proceso procesos[], int n) {
    printf("\n🔷 Ejecutando SJN con hilos...\n");
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
    printf("\n📊 Métricas SJN:\n");
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = copia[i].tiempo_inicio - copia[i].tiempo_llegada;
        long respuesta = copia[i].tiempo_fin - copia[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf("Proceso %d → Espera: %ld s | Respuesta: %ld s | Resultado: %s\n",
               copia[i].id, espera, respuesta, copia[i].resultado);
    }
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf("⏳ Promedio espera: %.2f s\n", prom_esp);
    printf("⏳ Promedio respuesta: %.2f s\n", prom_resp);
    guardar_metricas("SJN", copia, n, prom_esp, prom_resp);
}

int comparar_prioridad(const void* a, const void* b) {
    Proceso* p1 = (Proceso*)a;
    Proceso* p2 = (Proceso*)b;
    return p1->prioridad - p2->prioridad;
}

void planificador_prioridad_sin_desalojo(Proceso procesos[], int n) {
    printf("\n🔷 Ejecutando Prioridad (sin desalojo)...\n");
    Proceso copia[n];
    for (int i = 0; i < n; i++) copia[i] = procesos[i];
    qsort(copia, n, sizeof(Proceso), comparar_prioridad);

    pthread_t hilos[n];
    for (int i = 0; i < n; i++) {
        pthread_create(&hilos[i], NULL, ejecutar_proceso, &copia[i]);
        pthread_join(hilos[i], NULL);
    }
    printf("\n📊 Métricas Prioridad (sin desalojo):\n");
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = copia[i].tiempo_inicio - copia[i].tiempo_llegada;
        long respuesta = copia[i].tiempo_fin - copia[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf("Proceso %d → Espera: %ld s | Respuesta: %ld s | Resultado: %s\n",
               copia[i].id, espera, respuesta, copia[i].resultado);
    }
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf("⏳ Promedio espera: %.2f s\n", prom_esp);
    printf("⏳ Promedio respuesta: %.2f s\n", prom_resp);
    guardar_metricas("Prioridad (sin desalojo)", copia, n, prom_esp, prom_resp);
}
void planificador_prioridad_con_desalojo(Proceso procesos[], int n) {
    printf("\n🔷 Ejecutando Prioridad (con desalojo)...\n");
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
            printf("Tiempo %d → Ejecutando P%d (%s) [Prioridad %d]\n",
                   tiempo, procesos[mejor].id,
                   procesos[mejor].tipo == 0 ? "Prod" : "Cons",
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
            }
        } else {
            printf("Tiempo %d → CPU inactivo\n", tiempo);
            sleep(1);
        }
        tiempo++;
    }
    printf("\n📊 Métricas Prioridad (con desalojo):\n");
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = procesos[i].tiempo_inicio - procesos[i].tiempo_llegada;
        long respuesta = procesos[i].tiempo_fin - procesos[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf("Proceso %d → Espera: %ld s | Respuesta: %ld s | Resultado: %s\n",
               procesos[i].id, espera, respuesta, procesos[i].resultado);
    }
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf("⏳ Promedio espera: %.2f s\n", prom_esp);
    printf("⏳ Promedio respuesta: %.2f s\n", prom_resp);
    guardar_metricas("Prioridad (con desalojo)", procesos, n, prom_esp, prom_resp);
}


void planificador_rr(Proceso procesos[], int n) {
    printf("\n🔷 Ejecutando Round Robin...\n");
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
                printf("🧵 Proceso %d (%s) ejecuta %d s (restante: %d)\n",
                       procesos[i].id,
                       procesos[i].tipo == 0 ? "Productor" : "Consumidor",
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
                }
            }
        }
        tiempo++;
    }
    printf("\n📊 Métricas Round Robin:\n");
    long total_espera = 0, total_respuesta = 0;
    for (int i = 0; i < n; i++) {
        long espera = procesos[i].tiempo_inicio - procesos[i].tiempo_llegada;
        long respuesta = procesos[i].tiempo_fin - procesos[i].tiempo_llegada;
        total_espera += espera;
        total_respuesta += respuesta;
        printf("Proceso %d → Espera: %ld s | Respuesta: %ld s | Resultado: %s\n",
               procesos[i].id, espera, respuesta, procesos[i].resultado);
    }
    float prom_esp = total_espera / (float)n;
    float prom_resp = total_respuesta / (float)n;
    printf("⏳ Promedio espera: %.2f s\n", prom_esp);
    printf("⏳ Promedio respuesta: %.2f s\n", prom_resp);
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
    printf("\nSeleccione el algoritmo:\n");
    printf("1. FCFS\n2. SJN\n3. Prioridad (sin desalojo)\n4. Prioridad (con desalojo)\n5. Round Robin\nOpción: ");
    scanf("%d", &opcion);

    switch (opcion) {
        case 1: planificador_fcfs(procesos, cantidad); break;
        case 2: planificador_sjn(procesos, cantidad); break;
        case 3: planificador_prioridad_sin_desalojo(procesos, cantidad); break;
        case 4: planificador_prioridad_con_desalojo(procesos, cantidad); break;
        case 5: planificador_rr(procesos, cantidad); break;
        default: printf("Opción inválida.\n");
    }

    pthread_mutex_destroy(&mutex_archivo);
}

int main() {
    menu();
    return 0;
}
