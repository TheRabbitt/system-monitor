/**
 * @file expose_metrics.h
 * @brief Programa para leer el uso de CPU, memoria, disco, red, procesos y cambios de contexto y exponerlos como
 * métricas de Prometheus.
 */

#ifndef EXPOSE_METRICS_H
#define EXPOSE_METRICS_H

#include "metrics.h"
#include <errno.h>
#include <prom.h>
#include <promhttp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // Para sleep

/**
 * @brief Tamaño del búfer utilizado para leer líneas de archivos del sistema.
 */
#define BUFFER_SIZE 256

/**
 * @brief Actualiza la métrica de uso de CPU.
 */
void update_cpu_gauge(void);

/**
 * @brief Inicializa las métricas de memoria.
 */
void init_memory_metrics(void);

/**
 * @brief Actualiza la métrica de uso de memoria.
 */
void update_memory_gauges(void);

/**
 * @brief Inicializa las métricas de disco.
 */
void init_disk_metrics(void);

/**
 * @brief Actualiza metricas de disco.
 */
void update_disk_metrics(void);

/**
 * @brief Inicializa las métricas de red.
 */
void init_network_metrics(void);

/**
 * @brief Actualiza las métricas de red.
 */
void update_network_metrics(void);

/**
 * @brief Inicializa las métricas de procesos del sistema.
 */
void init_process_metrics(void);

/**
 * @brief Actualiza las métricas de conteo de procesos.
 */
void update_process_metrics(void);

/**
 * @brief Inicializa las métricas de cambios de contexto.
 */
void init_context_metrics(void);

/**
 * @brief Actualiza las métricas de cambios de contexto y rendimiento del sistema.
 */
void update_context_metrics(void);

/**
 * @brief Función del hilo para exponer las métricas vía HTTP en el puerto 8000.
 * @param arg Argumento no utilizado.
 * @return NULL
 */
void* expose_metrics(void* arg);

/**
 * @brief Inicializar mutex y métricas.
 */
void init_metrics(void);

/**
 * @brief Destructor de mutex
 */
void destroy_mutex(void);

#endif // EXPOSE_METRICS_H