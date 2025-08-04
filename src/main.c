/**
 * @file main.c
 * @brief Entry point of the system - Sistema completo de monitoreo de métricas del sistema
 */

#include "expose_metrics.h"
#include <pthread.h> // Required for thread usage
#include <stdbool.h>

/**
 * @brief Intervalo de actualización de métricas en segundos.
 */
#define SLEEP_TIME 1

/**
 * @brief Constante para representar el valor cero.
 *
 * Se utiliza para mejorar la legibilidad en comparaciones.
 */
#define ZERO 0

/**
 * @brief Función principal del sistema de monitoreo.
 *
 * Inicializa los recursos, crea el hilo HTTP para exponer métricas
 * y entra en un bucle de actualización periódica.
 *
 * @param argc Cantidad de argumentos de línea de comandos (no utilizado).
 * @param argv Lista de argumentos de línea de comandos (no utilizado).
 * @return EXIT_SUCCESS si finaliza correctamente, EXIT_FAILURE en caso de error.
 */
int main(int argc, char* argv[])
{
    // Suprimir warnings de parámetros no utilizados
    (void)argc;
    (void)argv;

    printf("=== Sistema de Monitoreo de Métricas del Sistema ===\n");
    printf("Iniciando monitoreo de:\n");
    printf("- CPU usage\n");
    printf("- Memory usage (total, used, available)\n");
    printf("- Disk I/O metrics (read/write rates, utilization)\n");
    printf("- Network metrics (bandwidth, packet rates, errors)\n");
    printf("- Process statistics (total, running, sleeping, stopped, zombie)\n");
    printf("- System performance (context switches, interrupts, process creation)\n");
    printf("Métricas expuestas en: http://localhost:8000/metrics\n");
    printf("================================================\n\n");

    // Initialize metrics
    init_metrics();

    // Create a thread to expose metrics via HTTP
    pthread_t tid;
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != ZERO)
    {
        fprintf(stderr, "Error creating HTTP server thread\n");
        return EXIT_FAILURE;
    }

    printf("HTTP server started on port 8000\n");
    printf("Starting metrics collection loop...\n\n");

    // Main loop to update metrics every second
    while (true)
    {
        printf("--- Updating metrics at %ld ---\n", time(NULL));

        // Actualizar métricas básicas
        update_cpu_gauge();
        update_memory_gauges();

        // Actualizar métricas de I/O y red
        update_disk_metrics();
        update_network_metrics();

        // Actualizar métricas de procesos y rendimiento del sistema
        update_process_metrics();
        update_context_metrics();

        printf("--- Metrics update completed ---\n\n");

        sleep(SLEEP_TIME);
    }

    return EXIT_SUCCESS;
}