/**
 * @file metrics.h
 * @brief Funciones para obtener el uso de CPU, memoria, disco, red, procesos y cambios de contexto desde el sistema de
 * archivos /proc.
 */

#ifndef METRICS_H
#define METRICS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Tamaño del búfer usado para leer líneas de archivos del sistema.
 */
#define BUFFER_SIZE 256

/**
 * @brief Número máximo de interfaces de red que se monitorearán.
 */
#define MAX_INTERFACES 10

/**
 * @brief Información de uso de memoria del sistema.
 */
typedef struct
{
    unsigned long long total_mem;     /**< Memoria total del sistema en KB. */
    unsigned long long used_mem;      /**< Memoria usada en KB. */
    unsigned long long available_mem; /**< Memoria disponible en KB. */
} memory_info_t;

/**
 * @brief Estadísticas de operaciones de disco.
 */
typedef struct
{
    char device_name[32];           /**< Nombre del dispositivo. */
    unsigned long reads_completed;  /**< Lecturas completadas. */
    unsigned long reads_merged;     /**< Lecturas fusionadas. */
    unsigned long sectors_read;     /**< Sectores leídos. */
    unsigned long time_reading;     /**< Tiempo leyendo (ms). */
    unsigned long writes_completed; /**< Escrituras completadas. */
    unsigned long writes_merged;    /**< Escrituras fusionadas. */
    unsigned long sectors_written;  /**< Sectores escritos. */
    unsigned long time_writing;     /**< Tiempo escribiendo (ms). */
    unsigned long ios_in_progress;  /**< I/Os en curso. */
    unsigned long time_io;          /**< Tiempo total de I/O (ms). */
    unsigned long weighted_time_io; /**< Tiempo ponderado de I/O (ms). */
} disk_stats_t;

/**
 * @brief Estadísticas de una interfaz de red.
 */
typedef struct
{
    char interface_name[32];       /**< Nombre de la interfaz. */
    unsigned long long rx_bytes;   /**< Bytes recibidos. */
    unsigned long long rx_packets; /**< Paquetes recibidos. */
    unsigned long long rx_errors;  /**< Errores de recepción. */
    unsigned long long rx_dropped; /**< Paquetes descartados en recepción. */
    unsigned long long tx_bytes;   /**< Bytes transmitidos. */
    unsigned long long tx_packets; /**< Paquetes transmitidos. */
    unsigned long long tx_errors;  /**< Errores de transmisión. */
    unsigned long long tx_dropped; /**< Paquetes descartados en transmisión. */
} network_interface_stats_t;

/**
 * @brief Estadísticas generales de procesos del sistema.
 */
typedef struct
{
    unsigned long total_processes;    /**< Total de procesos. */
    unsigned long running_processes;  /**< Procesos en ejecución. */
    unsigned long sleeping_processes; /**< Procesos en espera. */
    unsigned long stopped_processes;  /**< Procesos detenidos. */
    unsigned long zombie_processes;   /**< Procesos zombie. */
} process_stats_t;

/**
 * @brief Estadísticas del sistema relacionadas con el contexto.
 */
typedef struct
{
    unsigned long long context_switches;  /**< Cambios de contexto totales. */
    unsigned long long processes_created; /**< Procesos creados. */
    unsigned long long interrupts;        /**< Interrupciones. */
    unsigned long long soft_interrupts;   /**< Interrupciones suaves. */
} context_stats_t;

/**
 * @brief Métricas de salud del disco para monitoreo preventivo.
 */
typedef struct
{
    double read_rate;      /**< Lecturas por segundo. */
    double write_rate;     /**< Escrituras por segundo. */
    double io_utilization; /**< Porcentaje de utilización del disco. */
    double avg_wait_time;  /**< Tiempo promedio de espera (ms). */
    double queue_depth;    /**< Profundidad de cola de operaciones I/O. */
} disk_health_metrics_t;

/**
 * @brief Métricas derivadas de una interfaz de red.
 */
typedef struct
{
    double rx_rate_bps;           /**< Tasa de recepción (bytes/seg). */
    double tx_rate_bps;           /**< Tasa de transmisión (bytes/seg). */
    double rx_packet_rate;        /**< Paquetes recibidos por segundo. */
    double tx_packet_rate;        /**< Paquetes transmitidos por segundo. */
    double rx_error_rate;         /**< Tasa de errores de recepción. */
    double tx_error_rate;         /**< Tasa de errores de transmisión. */
    double total_bandwidth_usage; /**< Uso total de ancho de banda. */
} network_metrics_t;

/**
 * @brief Métricas agregadas de rendimiento del sistema.
 */
typedef struct
{
    double context_switch_rate;   /**< Cambios de contexto por segundo. */
    double process_creation_rate; /**< Procesos creados por segundo. */
    double interrupt_rate;        /**< Interrupciones por segundo. */
    double process_load_ratio;    /**< Ratio de procesos activos sobre total. */
} system_performance_metrics_t;

/**
 * @brief Obtiene el porcentaje de uso de memoria desde /proc/meminfo.
 *
 * Lee los valores de memoria total y disponible desde /proc/meminfo y calcula
 * el porcentaje de uso de memoria.
 *
 * @return Uso de memoria como porcentaje (0.0 a 100.0), o -1.0 en caso de error.
 */
double get_memory_usage(void);

/**
 * @brief Obtiene el porcentaje de uso de CPU desde /proc/stat.
 *
 * Lee los tiempos de CPU desde /proc/stat y calcula el porcentaje de uso de CPU
 * en un intervalo de tiempo.
 *
 * @return Uso de CPU como porcentaje (0.0 a 100.0), o -1.0 en caso de error.
 */
double get_cpu_usage(void);

/**
 * @brief Obtiene el uso de memoria desde /proc/meminfo.
 *
 * Lee los valores de memoria total, disponible y usada desde /proc/meminfo
 *
 * @param mem_info Puntero a estructura donde se almacenará la información de memoria
 * @return 0 si es exitoso, -1 en caso de error
 */
int get_memory_info(memory_info_t* mem_info);

/**
 * @brief Obtiene las estadísticas de un disco específico desde /proc/diskstats.
 *
 * Lee los valores estadísticos de I/O de un dispositivo de disco específico
 * desde /proc/diskstats, incluyendo lecturas, escrituras, tiempo de I/O y
 * operaciones en progreso.
 *
 * @param device Nombre del dispositivo de disco (ej: "sda", "nvme0n1")
 * @param stats Puntero a estructura donde se almacenarán las estadísticas
 * @return 0 si es exitoso, -1 si hay error o el dispositivo no se encuentra
 */
int get_disk_stats(const char* device, disk_stats_t* stats);

/**
 * @brief Calcula métricas de salud del disco para prevención de fallos.
 *
 * Compara estadísticas actuales con las anteriores para calcular tasas de
 * operaciones I/O, utilización del disco, tiempo de espera promedio y
 * profundidad de cola. Estas métricas son clave para detectar problemas
 * de rendimiento y prevenir fallos del almacenamiento.
 *
 * @param current Estadísticas actuales del disco
 * @param previous Estadísticas anteriores del disco
 * @param time_delta Tiempo transcurrido entre mediciones en segundos
 * @param health Estructura donde se almacenarán las métricas calculadas
 */
void calculate_disk_health(disk_stats_t* current, disk_stats_t* previous, double time_delta,
                           disk_health_metrics_t* health);

/**
 * @brief Detecta automáticamente el disco principal del sistema.
 *
 * Analiza /proc/diskstats para identificar el primer disco con actividad
 * (lecturas/escrituras > 0). Soporta diferentes tipos de discos: NVMe
 * (nvme0n1), SATA/SCSI (sda), IDE (hda) y eMMC (mmcblk0). Excluye
 * particiones para monitorear solo discos completos.
 *
 * @return Puntero a string con nombre del disco detectado, o "sda" como fallback
 */
char* detect_primary_disk(void);

/**
 * @brief Obtiene estadísticas de una interfaz de red específica desde /proc/net/dev.
 *
 * Lee las estadísticas de tráfico de red de una interfaz específica,
 * incluyendo bytes, paquetes, errores y paquetes descartados tanto
 * para recepción como transmisión.
 *
 * @param interface Nombre de la interfaz de red (ej: "eth0", "wlan0", "enp0s3")
 * @param stats Puntero a estructura donde se almacenarán las estadísticas
 * @return 0 si es exitoso, -1 si hay error o la interfaz no se encuentra
 */
int get_network_stats(const char* interface, network_interface_stats_t* stats);

/**
 * @brief Calcula métricas de red para monitoreo de comunicación.
 *
 * Compara estadísticas actuales con las anteriores para calcular tasas de
 * transferencia, tasas de paquetes y tasas de errores. Estas métricas son
 * esenciales para asegurar la comunicación entre puestos y detectar
 * problemas de conectividad.
 *
 * @param current Estadísticas actuales de la interfaz
 * @param previous Estadísticas anteriores de la interfaz
 * @param time_delta Tiempo transcurrido entre mediciones en segundos
 * @param metrics Estructura donde se almacenarán las métricas calculadas
 */
void calculate_network_metrics(network_interface_stats_t* current, network_interface_stats_t* previous,
                               double time_delta, network_metrics_t* metrics);

/**
 * @brief Detecta automáticamente la interfaz de red principal activa.
 *
 * Analiza /proc/net/dev para identificar la primera interfaz con tráfico
 * significativo (excluyendo loopback). Busca interfaces ethernet (eth, enp),
 * wireless (wlan, wlp) y otras interfaces activas.
 *
 * @return Puntero a string con nombre de la interfaz detectada, o "eth0" como fallback
 */
char* detect_primary_network_interface(void);

/**
 * @brief Obtiene estadísticas de procesos del sistema desde /proc.
 *
 * Cuenta los procesos por estado leyendo /proc/stat para el total de procesos
 * y recorriendo /proc/[pid]/stat para clasificar por estado (running,
 * sleeping, stopped, zombie). Esta información es clave para detectar
 * sobrecargas del sistema.
 *
 * @param stats Puntero a estructura donde se almacenarán las estadísticas
 * @return 0 si es exitoso, -1 si hay error
 */
int get_process_stats(process_stats_t* stats);

/**
 * @brief Obtiene estadísticas de cambios de contexto desde /proc/stat.
 *
 * Lee desde /proc/stat los valores de cambios de contexto (ctxt), procesos
 * creados (processes), interrupciones (intr) y soft interrupts (softirq).
 * Estas métricas son fundamentales para analizar el rendimiento del sistema.
 *
 * @param stats Puntero a estructura donde se almacenarán las estadísticas
 * @return 0 si es exitoso, -1 si hay error
 */
int get_context_stats(context_stats_t* stats);

/**
 * @brief Calcula métricas de rendimiento del sistema.
 *
 * Compara estadísticas actuales con las anteriores para calcular tasas de
 * cambios de contexto, creación de procesos e interrupciones. También
 * calcula el ratio de carga de procesos para detectar sobrecargas.
 *
 * @param current_context Estadísticas actuales de contexto
 * @param previous_context Estadísticas anteriores de contexto
 * @param current_process Estadísticas actuales de procesos
 * @param time_delta Tiempo transcurrido entre mediciones en segundos
 * @param metrics Estructura donde se almacenarán las métricas calculadas
 */
void calculate_system_performance_metrics(context_stats_t* current_context, context_stats_t* previous_context,
                                          process_stats_t* current_process, double time_delta,
                                          system_performance_metrics_t* metrics);

#endif // METRICS_H