#include "expose_metrics.h"

// Definiciones variables/constantes
#define SUCCESS 0
#define ZERO 0
#define ERROR -1
#define BOOL_TRUE 1
#define BOOL_FALSE 0
#define FIRST_RUN_FLAG 1
#define NOT_FIRST_RUN 0
#define NO_LABELS 0
#define ERROR_VALUE_DOUBLE -1.0
#define ZERO_VALUE_DOUBLE 0.0
#define HTTP_SERVER_PORT 8000
#define SLEEP_INTERVAL_SECONDS 1
#define SINGLE_MATCH 1
#define PRINTF_DECIMAL_PRECISION 1
#define PRINTF_RATIO_PRECISION 3
#define PRINTF_ERROR_PRECISION 2
#define PERCENTAGE 100.0

/** Mutex for thread synchronization */
pthread_mutex_t lock;

/** Prometheus metric for CPU usage */
static prom_gauge_t* cpu_usage_metric;

/** Prometheus metric for memory usage */
static prom_gauge_t* memory_usage_metric;

// Global variables

// Memoria
prom_gauge_t* memory_total_metric;
prom_gauge_t* memory_used_metric;
prom_gauge_t* memory_available_metric;

// Disco
prom_gauge_t* disk_read_rate_metric;
prom_gauge_t* disk_write_rate_metric;
prom_gauge_t* disk_utilization_metric;
prom_gauge_t* disk_avg_wait_time_metric;
prom_gauge_t* disk_queue_depth_metric;

// Red
prom_gauge_t* network_rx_rate_metric;
prom_gauge_t* network_tx_rate_metric;
prom_gauge_t* network_rx_packet_rate_metric;
prom_gauge_t* network_tx_packet_rate_metric;
prom_gauge_t* network_rx_error_rate_metric;
prom_gauge_t* network_tx_error_rate_metric;
prom_gauge_t* network_bandwidth_usage_metric;

// Procesos
prom_gauge_t* processes_total_metric;
prom_gauge_t* processes_running_metric;
prom_gauge_t* processes_sleeping_metric;
prom_gauge_t* processes_stopped_metric;
prom_gauge_t* processes_zombie_metric;

// Cambios de contexto y rendimiento del sistema
prom_gauge_t* context_switches_rate_metric;
prom_gauge_t* process_creation_rate_metric;
prom_gauge_t* interrupt_rate_metric;
prom_gauge_t* process_load_ratio_metric;

void update_cpu_gauge()
{
    double usage = get_cpu_usage();
    if (usage >= ZERO_VALUE_DOUBLE)
    {
        pthread_mutex_lock(&lock);
        prom_gauge_set(cpu_usage_metric, usage, NULL);
        pthread_mutex_unlock(&lock);
    }
    else
    {
        fprintf(stderr, "Error getting CPU usage\n");
    }
}

void update_memory_gauges()
{
    memory_info_t mem_info;

    if (get_memory_info(&mem_info) == SUCCESS)
    {
        pthread_mutex_lock(&lock);

        // Actualizar las tres métricas
        prom_gauge_set(memory_total_metric, (double)mem_info.total_mem, NULL);
        prom_gauge_set(memory_used_metric, (double)mem_info.used_mem, NULL);
        prom_gauge_set(memory_available_metric, (double)mem_info.available_mem, NULL);

        if (mem_info.total_mem > ZERO)
        {
            double usage_percentage = ((double)mem_info.used_mem / (double)mem_info.total_mem) * PERCENTAGE;
            prom_gauge_set(memory_usage_metric, usage_percentage, NULL);
        }

        pthread_mutex_unlock(&lock);

        printf("Memory - Total: %llu bytes, Used: %llu bytes, Available: %llu bytes\n", mem_info.total_mem,
               mem_info.used_mem, mem_info.available_mem);
    }
    else
    {
        fprintf(stderr, "Error getting memory information\n");
    }
}

void update_disk_metrics()
{
    static disk_stats_t prev_stats = {SUCCESS};
    static time_t prev_time = SUCCESS;
    static char* primary_disk = NULL;
    static int first_run = FIRST_RUN_FLAG;

    // Detectar disco solo una vez al inicio
    if (first_run)
    {
        primary_disk = detect_primary_disk();
        first_run = NOT_FIRST_RUN;
        if (!primary_disk)
        {
            fprintf(stderr, "No disk found for monitoring\n");
            return;
        }
    }

    disk_stats_t current_stats;
    time_t current_time = time(NULL);

    if (get_disk_stats(primary_disk, &current_stats) == SUCCESS)
    {
        // Solo calcular métricas después de la primera lectura
        if (prev_time > SUCCESS)
        {
            double time_delta = current_time - prev_time;
            if (time_delta > ZERO_VALUE_DOUBLE)
            {
                disk_health_metrics_t health;
                calculate_disk_health(&current_stats, &prev_stats, time_delta, &health);

                pthread_mutex_lock(&lock);

                // Exponer solo las métricas esenciales
                prom_gauge_set(disk_read_rate_metric, health.read_rate, NULL);
                prom_gauge_set(disk_write_rate_metric, health.write_rate, NULL);
                prom_gauge_set(disk_utilization_metric, health.io_utilization, NULL);
                prom_gauge_set(disk_queue_depth_metric, health.queue_depth, NULL);

                pthread_mutex_unlock(&lock);

                printf("Disk I/O - Reads/s: %.*f, Writes/s: %.*f, Util: %.*f%%\n", PRINTF_DECIMAL_PRECISION,
                       health.read_rate, PRINTF_DECIMAL_PRECISION, health.write_rate, PRINTF_DECIMAL_PRECISION,
                       health.io_utilization);
            }
        }

        prev_stats = current_stats;
        prev_time = current_time;
    }
}

void update_network_metrics()
{
    static network_interface_stats_t prev_stats = {SUCCESS};
    static time_t prev_time = SUCCESS;
    static char* primary_interface = NULL;
    static int first_run = FIRST_RUN_FLAG;

    // Detectar interfaz de red solo una vez al inicio
    if (first_run)
    {
        primary_interface = detect_primary_network_interface();
        first_run = NOT_FIRST_RUN;
        if (!primary_interface)
        {
            fprintf(stderr, "No network interface found for monitoring\n");
            return;
        }
    }

    network_interface_stats_t current_stats;
    time_t current_time = time(NULL);

    if (get_network_stats(primary_interface, &current_stats) == SUCCESS)
    {
        // Solo calcular métricas después de la primera lectura
        if (prev_time > SUCCESS)
        {
            double time_delta = current_time - prev_time;
            if (time_delta > ZERO_VALUE_DOUBLE)
            {
                network_metrics_t metrics;
                calculate_network_metrics(&current_stats, &prev_stats, time_delta, &metrics);

                pthread_mutex_lock(&lock);

                // Exponer métricas de red
                prom_gauge_set(network_rx_rate_metric, metrics.rx_rate_bps, NULL);
                prom_gauge_set(network_tx_rate_metric, metrics.tx_rate_bps, NULL);
                prom_gauge_set(network_rx_packet_rate_metric, metrics.rx_packet_rate, NULL);
                prom_gauge_set(network_tx_packet_rate_metric, metrics.tx_packet_rate, NULL);
                prom_gauge_set(network_rx_error_rate_metric, metrics.rx_error_rate, NULL);
                prom_gauge_set(network_tx_error_rate_metric, metrics.tx_error_rate, NULL);
                prom_gauge_set(network_bandwidth_usage_metric, metrics.total_bandwidth_usage, NULL);

                pthread_mutex_unlock(&lock);

                printf("Network - RX: %.*f B/s, TX: %.*f B/s, Bandwidth: %.*f B/s, RX Errors: %.*f%%\n",
                       PRINTF_DECIMAL_PRECISION, metrics.rx_rate_bps, PRINTF_DECIMAL_PRECISION, metrics.tx_rate_bps,
                       PRINTF_DECIMAL_PRECISION, metrics.total_bandwidth_usage, PRINTF_ERROR_PRECISION,
                       metrics.rx_error_rate);
            }
        }

        prev_stats = current_stats;
        prev_time = current_time;
    }
}

void update_process_metrics()
{
    process_stats_t process_stats;

    if (get_process_stats(&process_stats) == SUCCESS)
    {
        pthread_mutex_lock(&lock);

        // Actualizar métricas de procesos
        prom_gauge_set(processes_total_metric, (double)process_stats.total_processes, NULL);
        prom_gauge_set(processes_running_metric, (double)process_stats.running_processes, NULL);
        prom_gauge_set(processes_sleeping_metric, (double)process_stats.sleeping_processes, NULL);
        prom_gauge_set(processes_stopped_metric, (double)process_stats.stopped_processes, NULL);
        prom_gauge_set(processes_zombie_metric, (double)process_stats.zombie_processes, NULL);

        pthread_mutex_unlock(&lock);

        printf("Processes - Total: %lu, Running: %lu, Sleeping: %lu, Stopped: %lu, Zombie: %lu\n",
               process_stats.total_processes, process_stats.running_processes, process_stats.sleeping_processes,
               process_stats.stopped_processes, process_stats.zombie_processes);
    }
    else
    {
        fprintf(stderr, "Error getting process statistics\n");
    }
}

void update_context_metrics()
{
    static context_stats_t prev_context_stats = {SUCCESS};
    static process_stats_t current_process_stats = {SUCCESS};
    static time_t prev_time = SUCCESS;
    static int first_run = FIRST_RUN_FLAG;

    context_stats_t current_context_stats;
    time_t current_time = time(NULL);

    // Obtener estadísticas actuales de contexto
    if (get_context_stats(&current_context_stats) == SUCCESS)
    {
        // Obtener estadísticas actuales de procesos para el ratio de carga
        if (get_process_stats(&current_process_stats) != SUCCESS)
        {
            fprintf(stderr, "Error getting process stats for context metrics\n");
            return;
        }

        // Solo calcular métricas después de la primera lectura
        if (!first_run && prev_time > SUCCESS)
        {
            double time_delta = current_time - prev_time;
            if (time_delta > ZERO_VALUE_DOUBLE)
            {
                system_performance_metrics_t perf_metrics;
                calculate_system_performance_metrics(&current_context_stats, &prev_context_stats,
                                                     &current_process_stats, time_delta, &perf_metrics);

                pthread_mutex_lock(&lock);

                // Exponer métricas de rendimiento del sistema
                prom_gauge_set(context_switches_rate_metric, perf_metrics.context_switch_rate, NULL);
                prom_gauge_set(process_creation_rate_metric, perf_metrics.process_creation_rate, NULL);
                prom_gauge_set(interrupt_rate_metric, perf_metrics.interrupt_rate, NULL);
                prom_gauge_set(process_load_ratio_metric, perf_metrics.process_load_ratio, NULL);

                pthread_mutex_unlock(&lock);

                printf("System Performance - Context switches/s: %.*f, Process creation/s: %.*f, "
                       "Interrupts/s: %.*f, Load ratio: %.*f\n",
                       PRINTF_DECIMAL_PRECISION, perf_metrics.context_switch_rate, PRINTF_DECIMAL_PRECISION,
                       perf_metrics.process_creation_rate, PRINTF_DECIMAL_PRECISION, perf_metrics.interrupt_rate,
                       PRINTF_RATIO_PRECISION, perf_metrics.process_load_ratio);
            }
        }

        prev_context_stats = current_context_stats;
        prev_time = current_time;
        first_run = NOT_FIRST_RUN;
    }
    else
    {
        fprintf(stderr, "Error getting context switch statistics\n");
    }
}

void* expose_metrics(void* arg)
{
    (void)arg; // Unused argument

    // Ensure HTTP handler is attached to the default registry
    promhttp_set_active_collector_registry(NULL);

    // Start HTTP server on port 8000
    struct MHD_Daemon* daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, HTTP_SERVER_PORT, NULL, NULL);
    if (daemon == NULL)
    {
        fprintf(stderr, "Error starting HTTP server\n");
        return NULL;
    }

    // Keep server running
    while (BOOL_TRUE)
    {
        sleep(SLEEP_INTERVAL_SECONDS);
    }

    // Should never reach here
    MHD_stop_daemon(daemon);
    return NULL;
}

void init_memory_metrics()
{
    // Crear las tres métricas
    if (!memory_total_metric)
    {
        memory_total_metric = prom_gauge_new("memory_total_bytes", "Total system memory in bytes", NO_LABELS, NULL);
    }
    if (!memory_used_metric)
    {
        memory_used_metric = prom_gauge_new("memory_used_bytes", "Used system memory in bytes", NO_LABELS, NULL);
    }
    if (!memory_available_metric)
    {
        memory_available_metric =
            prom_gauge_new("memory_available_bytes", "Available system memory in bytes", NO_LABELS, NULL);
    }

    // Registrar las métricas solo si se crearon correctamente
    if (memory_total_metric)
    {
        if (prom_collector_registry_must_register_metric(memory_total_metric) != SUCCESS)
        {
            fprintf(stderr, "Warning: Could not register memory_total_metric\n");
        }
    }
    if (memory_used_metric)
    {
        if (prom_collector_registry_must_register_metric(memory_used_metric) != SUCCESS)
        {
            fprintf(stderr, "Warning: Could not register memory_used_metric\n");
        }
    }
    if (memory_available_metric)
    {
        if (prom_collector_registry_must_register_metric(memory_available_metric) != SUCCESS)
        {
            fprintf(stderr, "Warning: Could not register memory_available_metric\n");
        }
    }
}

void init_disk_metrics(void)
{
    // Crear las métricas de disco
    disk_read_rate_metric = prom_gauge_new("disk_read_rate", "Disk read operations per second", NO_LABELS, NULL);

    disk_write_rate_metric = prom_gauge_new("disk_write_rate", "Disk write operations per second", NO_LABELS, NULL);

    disk_utilization_metric =
        prom_gauge_new("disk_utilization_percent", "Disk utilization percentage", NO_LABELS, NULL);

    disk_avg_wait_time_metric =
        prom_gauge_new("disk_avg_wait_time_ms", "Average disk I/O wait time in milliseconds", NO_LABELS, NULL);

    disk_queue_depth_metric = prom_gauge_new("disk_queue_depth", "Current disk I/O queue depth", NO_LABELS, NULL);

    // Registrar las métricas
    if (disk_read_rate_metric)
    {
        prom_collector_registry_must_register_metric(disk_read_rate_metric);
    }
    if (disk_write_rate_metric)
    {
        prom_collector_registry_must_register_metric(disk_write_rate_metric);
    }
    if (disk_utilization_metric)
    {
        prom_collector_registry_must_register_metric(disk_utilization_metric);
    }
    if (disk_avg_wait_time_metric)
    {
        prom_collector_registry_must_register_metric(disk_avg_wait_time_metric);
    }
    if (disk_queue_depth_metric)
    {
        prom_collector_registry_must_register_metric(disk_queue_depth_metric);
    }
}

void init_network_metrics(void)
{
    // Crear las métricas de red
    network_rx_rate_metric =
        prom_gauge_new("network_rx_rate_bps", "Network receive rate in bytes per second", NO_LABELS, NULL);

    network_tx_rate_metric =
        prom_gauge_new("network_tx_rate_bps", "Network transmit rate in bytes per second", NO_LABELS, NULL);

    network_rx_packet_rate_metric =
        prom_gauge_new("network_rx_packet_rate", "Network receive packet rate per second", NO_LABELS, NULL);

    network_tx_packet_rate_metric =
        prom_gauge_new("network_tx_packet_rate", "Network transmit packet rate per second", NO_LABELS, NULL);

    network_rx_error_rate_metric =
        prom_gauge_new("network_rx_error_rate_percent", "Network receive error rate percentage", NO_LABELS, NULL);

    network_tx_error_rate_metric =
        prom_gauge_new("network_tx_error_rate_percent", "Network transmit error rate percentage", NO_LABELS, NULL);

    network_bandwidth_usage_metric = prom_gauge_new(
        "network_bandwidth_usage_bps", "Total network bandwidth usage in bytes per second", NO_LABELS, NULL);

    // Registrar las métricas
    if (network_rx_rate_metric)
    {
        prom_collector_registry_must_register_metric(network_rx_rate_metric);
    }
    if (network_tx_rate_metric)
    {
        prom_collector_registry_must_register_metric(network_tx_rate_metric);
    }
    if (network_rx_packet_rate_metric)
    {
        prom_collector_registry_must_register_metric(network_rx_packet_rate_metric);
    }
    if (network_tx_packet_rate_metric)
    {
        prom_collector_registry_must_register_metric(network_tx_packet_rate_metric);
    }
    if (network_rx_error_rate_metric)
    {
        prom_collector_registry_must_register_metric(network_rx_error_rate_metric);
    }
    if (network_tx_error_rate_metric)
    {
        prom_collector_registry_must_register_metric(network_tx_error_rate_metric);
    }
    if (network_bandwidth_usage_metric)
    {
        prom_collector_registry_must_register_metric(network_bandwidth_usage_metric);
    }
}

void init_process_metrics(void)
{
    printf("DEBUG: Inicializando métricas de procesos...\n");

    // Crear las métricas de procesos
    processes_total_metric =
        prom_gauge_new("processes_total", "Total number of processes in the system", NO_LABELS, NULL);

    processes_running_metric =
        prom_gauge_new("processes_running", "Number of processes in running state", NO_LABELS, NULL);

    processes_sleeping_metric =
        prom_gauge_new("processes_sleeping", "Number of processes in sleeping state", NO_LABELS, NULL);

    processes_stopped_metric =
        prom_gauge_new("processes_stopped", "Number of processes in stopped state", NO_LABELS, NULL);

    processes_zombie_metric = prom_gauge_new("processes_zombie", "Number of zombie processes", NO_LABELS, NULL);

    // Registrar las métricas
    if (processes_total_metric)
    {
        prom_collector_registry_must_register_metric(processes_total_metric);
    }
    if (processes_running_metric)
    {
        prom_collector_registry_must_register_metric(processes_running_metric);
    }
    if (processes_sleeping_metric)
    {
        prom_collector_registry_must_register_metric(processes_sleeping_metric);
    }
    if (processes_stopped_metric)
    {
        prom_collector_registry_must_register_metric(processes_stopped_metric);
    }
    if (processes_zombie_metric)
    {
        prom_collector_registry_must_register_metric(processes_zombie_metric);
    }

    printf("DEBUG: Métricas de procesos inicializadas\n");
}

void init_context_metrics(void)
{
    // Crear las métricas de cambios de contexto y rendimiento del sistema
    context_switches_rate_metric =
        prom_gauge_new("context_switches_rate", "Context switches per second", NO_LABELS, NULL);

    process_creation_rate_metric =
        prom_gauge_new("process_creation_rate", "Processes created per second", NO_LABELS, NULL);

    interrupt_rate_metric = prom_gauge_new("interrupt_rate", "Interrupts per second", NO_LABELS, NULL);

    process_load_ratio_metric = prom_gauge_new(
        "process_load_ratio", "Ratio of running processes to total processes (0.0-1.0)", NO_LABELS, NULL);

    // Registrar las métricas
    if (context_switches_rate_metric)
    {
        prom_collector_registry_must_register_metric(context_switches_rate_metric);
    }
    if (process_creation_rate_metric)
    {
        prom_collector_registry_must_register_metric(process_creation_rate_metric);
    }
    if (interrupt_rate_metric)
    {
        prom_collector_registry_must_register_metric(interrupt_rate_metric);
    }
    if (process_load_ratio_metric)
    {
        prom_collector_registry_must_register_metric(process_load_ratio_metric);
    }
}

void init_metrics()
{
    // Initialize mutex
    if (pthread_mutex_init(&lock, NULL) != SUCCESS)
    {
        fprintf(stderr, "Error initializing mutex\n");
    }

    // Initialize Prometheus collector registry
    if (prom_collector_registry_default_init() != SUCCESS)
    {
        fprintf(stderr, "Error initializing Prometheus registry\n");
    }

    // Create CPU usage metric
    cpu_usage_metric = prom_gauge_new("cpu_usage_percentage", "CPU usage percentage", NO_LABELS, NULL);
    if (cpu_usage_metric == NULL)
    {
        fprintf(stderr, "Error creating CPU usage metric\n");
    }

    // Create memory usage metric
    memory_usage_metric = prom_gauge_new("memory_usage_percentage", "Memory usage percentage", NO_LABELS, NULL);
    if (memory_usage_metric == NULL)
    {
        fprintf(stderr, "Error creating memory usage metric\n");
    }

    // Initialize specialized metrics
    init_memory_metrics();
    init_disk_metrics();
    init_network_metrics();
    init_process_metrics();
    init_context_metrics();

    // Register basic metrics in the default registry
    if (cpu_usage_metric != NULL)
    {
        if (prom_collector_registry_must_register_metric(cpu_usage_metric) != SUCCESS)
        {
            fprintf(stderr, "Warning: Could not register CPU metric\n");
        }
    }

    if (memory_usage_metric != NULL)
    {
        if (prom_collector_registry_must_register_metric(memory_usage_metric) != SUCCESS)
        {
            fprintf(stderr, "Warning: Could not register memory percentage metric\n");
        }
    }
}

void destroy_mutex()
{
    pthread_mutex_destroy(&lock);
}