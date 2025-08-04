# Makefile for compiling the metrics program with network monitoring

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Iinclude -Wall -Wextra -std=c99 -g

# Libraries
LIBS = -lprom -pthread -lpromhttp -lmicrohttpd

# Source files
SOURCES = src/main.c src/expose_metrics.c src/metrics.c 

# Executable name
TARGET = metrics

# Default rule
all: $(TARGET)

# Rule to compile the program
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

# Rule to clean compiled files
clean:
	rm -f $(TARGET)

# Rule to rebuild everything
rebuild: clean all

# Instalar dependencias (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libprom-dev libpromhttp-dev libmicrohttpd-dev

# Ejecutar el programa
run: $(TARGET)
	./$(TARGET)

# Verificar métricas
test-metrics:
	curl http://localhost:8000/metrics

# Mostrar ayuda
help:
	@echo "Comandos disponibles:"
	@echo "  make all          - Compilar el proyecto"
	@echo "  make clean        - Limpiar archivos generados"
	@echo "  make rebuild      - Limpiar y recompilar"
	@echo "  make install-deps - Instalar dependencias"
	@echo "  make run          - Compilar y ejecutar"
	@echo "  make test-metrics - Probar endpoint de métricas"
	@echo "  make help         - Mostrar esta ayuda"

.PHONY: all clean rebuild install-deps run test-metrics help