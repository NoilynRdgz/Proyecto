# Makefile para el Simulador de Planificación de Procesos
# Universidad Latina de Costa Rica - BIS11 Sistemas Operativos

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
TARGET = simulador
SOURCE = Algoritmos.c

# Regla principal
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Regla para limpiar archivos compilados
clean:
	rm -f $(TARGET) *.o

# Regla para ejecutar el programa
run: $(TARGET)
	./$(TARGET)

# Regla para mostrar ayuda
help:
	@echo "Comandos disponibles:"
	@echo "  make          - Compila el programa"
	@echo "  make run      - Compila y ejecuta el programa"
	@echo "  make clean    - Elimina archivos compilados"
	@echo "  make help     - Muestra esta ayuda"

.PHONY: clean run help
