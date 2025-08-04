#include "metrics.h"
#include <ctype.h>
#include <dirent.h>

// Definicions de variables/constantes
#define KILOBYTES_TO_BYTES 1024
#define PERCENTAGE_MULTIPLIER 100.0
#define MILLISECONDS_TO_SECONDS 1000.0
#define CPU_STAT_FIELDS_REQUIRED 8
#define DISK_STAT_FIELDS_REQUIRED 14
#define NETWORK_STAT_FIELDS_REQUIRED 8
#define LINE_BUFFER_SIZE 512
#define DEVICE_NAME_SIZE 32
#define INTERFACE_NAME_SIZE 32
#define COMMAND_NAME_SIZE 256
#define PATH_BUFFER_SIZE 512
#define NVME_PREFIX_LENGTH 4
#define SD_DEVICE_LENGTH 3
#define SD_PREFIX_LENGTH 2
#define MMCBLK_PREFIX_LENGTH 6
#define ETH_PREFIX_LENGTH 3
#define ENP_PREFIX_LENGTH 3
#define ENS_PREFIX_LENGTH 3
#define WLAN_PREFIX_LENGTH 4
#define WLP_PREFIX_LENGTH 3
#define WLO_PREFIX_LENGTH 3
#define WLS_PREFIX_LENGTH 3
#define INTR_PREFIX_LENGTH 5
#define SOFTIRQ_PREFIX_LENGTH 8
#define PROC_STAT_EXTENDED_BUFFER_SIZE (BUFFER_SIZE * 4)
#define MIN_REQUIRED_CONTEXT_FIELDS 2
#define MAX_EXPECTED_CONTEXT_FIELDS 4
#define EXPECTED_FSCANF_FIELDS 3
#define MIN_SSCANF_FIELDS 2
#define BASE_10 10
#define PRINTF_DECIMAL_PRECISION 1
#define PRINTF_RATIO_PRECISION 3
#define FALLBACK_DISK_NAME "sda"
#define FALLBACK_NETWORK_INTERFACE "eth0"
#define LOOPBACK_INTERFACE "lo"

// Valores booleanos y de retorno
#define SUCCESS 0
#define ERROR -1
#define ERROR_VALUE_DOUBLE -1.0
#define ZERO_VALUE_DOUBLE 0.0
#define BOOL_TRUE 1
#define BOOL_FALSE 0
#define SINGLE_MATCH 1
#define NO_BYTES 0
#define NO_PACKETS 0
#define NO_PROCESSES 0
#define FIRST_CHAR_INDEX 0
#define STRING_TERMINATOR '\0'
#define ARRAY_OFFSET_ONE 1
#define BUFFER_NULL_TERMINATOR_SPACE 1

// Caracteres y estados de proceso
#define SPACE_CHAR ' '
#define TAB_CHAR '\t'
#define COLON_CHAR ':'
#define PARTITION_INDICATOR_CHAR 'p'
#define PROCESS_STATE_RUNNING 'R'
#define PROCESS_STATE_SLEEPING 'S'
#define PROCESS_STATE_UNINTERRUPTIBLE_SLEEP 'D'
#define PROCESS_STATE_IDLE 'I'
#define PROCESS_STATE_STOPPED 'T'
#define PROCESS_STATE_STOPPED_DEBUGGER 't'
#define PROCESS_STATE_ZOMBIE 'Z'

// Índices de archivos proc
#define PROC_NET_DEV_HEADER_LINES 2
#define FIRST_HEADER_LINE 1
#define SECOND_HEADER_LINE 2

int get_memory_info(memory_info_t* mem_info)
{
    FILE* fp;
    char buffer[BUFFER_SIZE];
    unsigned long long total = NO_BYTES, available = NO_BYTES;

    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/meminfo");
        return ERROR;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "MemTotal: %llu kB", &total) == SINGLE_MATCH)
        {
            continue;
        }
        if (sscanf(buffer, "MemAvailable: %llu kB", &available) == SINGLE_MATCH)
        {
            break;
        }
    }

    fclose(fp);

    if (total == NO_BYTES || available == NO_BYTES)
    {
        fprintf(stderr, "Error reading memory information from /proc/meminfo\n");
        return ERROR;
    }

    // Convertir de kB a bytes
    mem_info->total_mem = total * KILOBYTES_TO_BYTES;
    mem_info->available_mem = available * KILOBYTES_TO_BYTES;
    mem_info->used_mem = mem_info->total_mem - mem_info->available_mem;

    return SUCCESS;
}

double get_memory_usage()
{
    FILE* fp;
    char buffer[BUFFER_SIZE];
    unsigned long long total_mem = NO_BYTES, free_mem = NO_BYTES;

    // Open /proc/meminfo file
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/meminfo");
        return ERROR_VALUE_DOUBLE;
    }

    // Read total and available memory values
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "MemTotal: %llu kB", &total_mem) == SINGLE_MATCH)
        {
            continue; // MemTotal found
        }
        if (sscanf(buffer, "MemAvailable: %llu kB", &free_mem) == SINGLE_MATCH)
        {
            break; // MemAvailable found, we can stop reading
        }
    }

    fclose(fp);

    // Check if both values were found
    if (total_mem == NO_BYTES || free_mem == NO_BYTES)
    {
        fprintf(stderr, "Error reading memory information from /proc/meminfo\n");
        return ERROR_VALUE_DOUBLE;
    }

    // Calculate memory usage percentage
    double used_mem = total_mem - free_mem;
    double mem_usage_percent = (used_mem / total_mem) * PERCENTAGE_MULTIPLIER;

    return mem_usage_percent;
}

double get_cpu_usage()
{
    static unsigned long long prev_user = NO_BYTES, prev_nice = NO_BYTES, prev_system = NO_BYTES, prev_idle = NO_BYTES,
                              prev_iowait = NO_BYTES, prev_irq = NO_BYTES, prev_softirq = NO_BYTES,
                              prev_steal = NO_BYTES;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    unsigned long long totald, idled;
    double cpu_usage_percent;

    // Open /proc/stat file
    FILE* fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/stat");
        return ERROR_VALUE_DOUBLE;
    }

    char buffer[PROC_STAT_EXTENDED_BUFFER_SIZE];
    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        perror("Error reading /proc/stat");
        fclose(fp);
        return ERROR_VALUE_DOUBLE;
    }
    fclose(fp);

    // Parse CPU time values
    int ret = sscanf(buffer, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait,
                     &irq, &softirq, &steal);
    if (ret < CPU_STAT_FIELDS_REQUIRED)
    {
        fprintf(stderr, "Error parsing /proc/stat\n");
        return ERROR_VALUE_DOUBLE;
    }

    // Calculate differences between current and previous readings
    unsigned long long prev_idle_total = prev_idle + prev_iowait;
    unsigned long long idle_total = idle + iowait;

    unsigned long long prev_non_idle = prev_user + prev_nice + prev_system + prev_irq + prev_softirq + prev_steal;
    unsigned long long non_idle = user + nice + system + irq + softirq + steal;

    unsigned long long prev_total = prev_idle_total + prev_non_idle;
    unsigned long long total = idle_total + non_idle;

    totald = total - prev_total;
    idled = idle_total - prev_idle_total;

    if (totald == NO_BYTES)
    {
        fprintf(stderr, "Totald is zero, cannot calculate CPU usage!\n");
        return ERROR_VALUE_DOUBLE;
    }

    // Calculate CPU usage percentage
    cpu_usage_percent = ((double)(totald - idled) / totald) * PERCENTAGE_MULTIPLIER;

    // Update previous values for next reading
    prev_user = user;
    prev_nice = nice;
    prev_system = system;
    prev_idle = idle;
    prev_iowait = iowait;
    prev_irq = irq;
    prev_softirq = softirq;
    prev_steal = steal;

    return cpu_usage_percent;
}

// Función simplificada para detectar solo el disco principal
char* detect_primary_disk()
{
    FILE* fp;
    char line[LINE_BUFFER_SIZE];
    char dev_name[DEVICE_NAME_SIZE];
    int major, minor;
    static char primary_disk[DEVICE_NAME_SIZE] = {FIRST_CHAR_INDEX};

    fp = fopen("/proc/diskstats", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/diskstats");
        return NULL;
    }

    while (fgets(line, sizeof(line), fp))
    {
        unsigned long reads, dummy1, dummy2, dummy3, writes;

        // Leer campos explícitamente sin usar assignment suppression
        int fields = sscanf(line, "%d %d %s %lu %lu %lu %lu %lu", &major, &minor, dev_name, &reads, &dummy1, &dummy2,
                            &dummy3, &writes);

        if (fields >= CPU_STAT_FIELDS_REQUIRED && (reads > NO_BYTES || writes > NO_BYTES))
        {
            // Buscar primer disco activo (no partición)
            if ((strncmp(dev_name, "nvme", NVME_PREFIX_LENGTH) == SUCCESS && strstr(dev_name, "p") == NULL) ||
                ((strncmp(dev_name, "sd", SD_PREFIX_LENGTH) == SUCCESS) && strlen(dev_name) == SD_DEVICE_LENGTH) ||
                (strncmp(dev_name, "mmcblk", MMCBLK_PREFIX_LENGTH) == SUCCESS && strstr(dev_name, "p") == NULL))
            {

                strncpy(primary_disk, dev_name, sizeof(primary_disk) - BUFFER_NULL_TERMINATOR_SPACE);
                fclose(fp);
                printf("Primary disk detected: %s\n", primary_disk);
                return primary_disk;
            }
        }
    }

    fclose(fp);

    // Fallback: usar sda si no encontramos nada
    strcpy(primary_disk, FALLBACK_DISK_NAME);
    printf("Using fallback disk: %s\n", primary_disk);
    return primary_disk;
}

// Función para "rastrear" = leer y procesar estadísticas
int get_disk_stats(const char* device, disk_stats_t* stats)
{
    FILE* fp;
    char line[LINE_BUFFER_SIZE];
    char dev_name[DEVICE_NAME_SIZE];
    int major, minor;

    fp = fopen("/proc/diskstats", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/diskstats");
        return ERROR;
    }

    while (fgets(line, sizeof(line), fp))
    {
        // Parsear línea: major minor name [11 campos de estadísticas]
        int fields = sscanf(line, "%d %d %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &major, &minor, dev_name,
                            &stats->reads_completed, &stats->reads_merged, &stats->sectors_read, &stats->time_reading,
                            &stats->writes_completed, &stats->writes_merged, &stats->sectors_written,
                            &stats->time_writing, &stats->ios_in_progress, &stats->time_io, &stats->weighted_time_io);

        // Si encontramos el dispositivo que buscamos
        if (fields >= DISK_STAT_FIELDS_REQUIRED && strcmp(dev_name, device) == SUCCESS)
        {
            strncpy(stats->device_name, dev_name, sizeof(stats->device_name) - BUFFER_NULL_TERMINATOR_SPACE);
            fclose(fp);
            return SUCCESS;
        }
    }

    fclose(fp);
    fprintf(stderr, "Device %s not found in /proc/diskstats\n", device);
    return ERROR;
}

// Calcular métricas de salud (para prevención de fallos)
void calculate_disk_health(disk_stats_t* current, disk_stats_t* previous, double time_delta,
                           disk_health_metrics_t* health)
{
    // Tasa de operaciones I/O
    health->read_rate = (current->reads_completed - previous->reads_completed) / time_delta;
    health->write_rate = (current->writes_completed - previous->writes_completed) / time_delta;

    // Utilización del disco (% tiempo ocupado)
    unsigned long io_time_delta = current->time_io - previous->time_io;
    health->io_utilization = (io_time_delta / (time_delta * MILLISECONDS_TO_SECONDS)) * PERCENTAGE_MULTIPLIER;

    // Tiempo promedio de espera
    unsigned long total_ios = (current->reads_completed + current->writes_completed) -
                              (previous->reads_completed + previous->writes_completed);
    if (total_ios > NO_BYTES)
    {
        health->avg_wait_time = (double)io_time_delta / total_ios;
    }

    // Cola de I/O actual
    health->queue_depth = current->ios_in_progress;
}

// Detectar interfaz de red principal
char* detect_primary_network_interface()
{
    FILE* fp;
    char line[LINE_BUFFER_SIZE];
    char interface_name[INTERFACE_NAME_SIZE];
    static char primary_interface[INTERFACE_NAME_SIZE] = {FIRST_CHAR_INDEX};
    unsigned long long rx_bytes, tx_bytes;

    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/net/dev");
        return NULL;
    }

    // Saltar las dos primeras líneas de encabezado
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    printf("DEBUG: Scanning network interfaces...\n");

    while (fgets(line, sizeof(line), fp))
    {
        // Buscar los dos puntos que separan interfaz de estadísticas
        char* colon = strchr(line, COLON_CHAR);
        if (colon == NULL)
            continue;

        *colon = STRING_TERMINATOR; // Dividir línea en nombre de interfaz y estadísticas

        // Extraer nombre de interfaz (quitar espacios)
        char* interface_start = line;
        while (*interface_start == SPACE_CHAR || *interface_start == TAB_CHAR)
        {
            interface_start++;
        }
        strncpy(interface_name, interface_start, sizeof(interface_name) - BUFFER_NULL_TERMINATOR_SPACE);
        interface_name[sizeof(interface_name) - BUFFER_NULL_TERMINATOR_SPACE] = STRING_TERMINATOR;

        // Parsear estadísticas básicas
        char* stats = colon + ARRAY_OFFSET_ONE;
        if (sscanf(stats, "%llu %*u %*u %*u %*u %*u %*u %*u %llu", &rx_bytes, &tx_bytes) >= MIN_SSCANF_FIELDS)
        {
            printf("DEBUG: Interface %s - RX: %llu bytes, TX: %llu bytes\n", interface_name, rx_bytes, tx_bytes);

            // Ignorar loopback
            if (strcmp(interface_name, LOOPBACK_INTERFACE) == SUCCESS)
            {
                continue;
            }

            // Buscar cualquier interfaz con tráfico significativo
            if (rx_bytes > NO_BYTES || tx_bytes > NO_BYTES)
            {
                // Priorizar interfaces conocidas primero
                if (strncmp(interface_name, "eth", ETH_PREFIX_LENGTH) == SUCCESS ||
                    strncmp(interface_name, "enp", ENP_PREFIX_LENGTH) == SUCCESS ||
                    strncmp(interface_name, "ens", ENS_PREFIX_LENGTH) == SUCCESS ||
                    strncmp(interface_name, "wlan", WLAN_PREFIX_LENGTH) == SUCCESS ||
                    strncmp(interface_name, "wlp", WLP_PREFIX_LENGTH) == SUCCESS ||
                    strncmp(interface_name, "wlo", WLO_PREFIX_LENGTH) == SUCCESS ||
                    strncmp(interface_name, "wls", WLS_PREFIX_LENGTH) == SUCCESS)
                {

                    strncpy(primary_interface, interface_name,
                            sizeof(primary_interface) - BUFFER_NULL_TERMINATOR_SPACE);
                    primary_interface[sizeof(primary_interface) - BUFFER_NULL_TERMINATOR_SPACE] = STRING_TERMINATOR;
                    fclose(fp);
                    printf("Primary network interface detected: %s\n", primary_interface);
                    return primary_interface;
                }

                // Si no es una interfaz conocida pero tiene tráfico, guardarla como opción
                if (primary_interface[FIRST_CHAR_INDEX] == STRING_TERMINATOR)
                {
                    strncpy(primary_interface, interface_name,
                            sizeof(primary_interface) - BUFFER_NULL_TERMINATOR_SPACE);
                    primary_interface[sizeof(primary_interface) - BUFFER_NULL_TERMINATOR_SPACE] = STRING_TERMINATOR;
                }
            }
        }
    }

    fclose(fp);

    // Si encontramos alguna interfaz con tráfico, usarla
    if (primary_interface[FIRST_CHAR_INDEX] != STRING_TERMINATOR)
    {
        printf("Primary network interface detected: %s\n", primary_interface);
        return primary_interface;
    }

    // Último recurso: buscar la primera interfaz que no sea loopback
    fp = fopen("/proc/net/dev", "r");
    if (fp != NULL)
    {
        fgets(line, sizeof(line), fp);
        fgets(line, sizeof(line), fp);

        while (fgets(line, sizeof(line), fp))
        {
            char* colon = strchr(line, COLON_CHAR);
            if (colon == NULL)
                continue;

            *colon = STRING_TERMINATOR;
            char* interface_start = line;
            while (*interface_start == SPACE_CHAR || *interface_start == TAB_CHAR)
            {
                interface_start++;
            }

            if (strcmp(interface_start, LOOPBACK_INTERFACE) != SUCCESS)
            {
                strncpy(primary_interface, interface_start, sizeof(primary_interface) - BUFFER_NULL_TERMINATOR_SPACE);
                primary_interface[sizeof(primary_interface) - BUFFER_NULL_TERMINATOR_SPACE] = STRING_TERMINATOR;
                fclose(fp);
                printf("Using first available interface: %s\n", primary_interface);
                return primary_interface;
            }
        }
        fclose(fp);
    }

    // Fallback final
    strcpy(primary_interface, FALLBACK_NETWORK_INTERFACE);
    printf("Using fallback network interface: %s\n", primary_interface);
    return primary_interface;
}

// Obtener estadísticas de red
int get_network_stats(const char* interface, network_interface_stats_t* stats)
{
    FILE* fp;
    char line[LINE_BUFFER_SIZE];
    char interface_name[INTERFACE_NAME_SIZE];

    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/net/dev");
        return ERROR;
    }

    // Saltar las dos primeras líneas de encabezado
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp))
    {
        // Buscar los dos puntos que separan interfaz de estadísticas
        char* colon = strchr(line, COLON_CHAR);
        if (colon == NULL)
            continue;

        *colon = STRING_TERMINATOR; // Dividir línea

        // Extraer nombre de interfaz limpio
        char* interface_start = line;
        while (*interface_start == SPACE_CHAR || *interface_start == TAB_CHAR)
        {
            interface_start++;
        }
        strncpy(interface_name, interface_start, sizeof(interface_name) - BUFFER_NULL_TERMINATOR_SPACE);

        // Si es la interfaz que buscamos
        if (strcmp(interface_name, interface) == SUCCESS)
        {
            char* stats_line = colon + ARRAY_OFFSET_ONE;

            // Parsear estadísticas según formato de /proc/net/dev
            // RX: bytes packets errs drop fifo frame compressed multicast
            // TX: bytes packets errs drop fifo colls carrier compressed
            int fields = sscanf(stats_line, "%llu %llu %llu %llu %*u %*u %*u %*u %llu %llu %llu %llu %*u %*u %*u %*u",
                                &stats->rx_bytes, &stats->rx_packets, &stats->rx_errors, &stats->rx_dropped,
                                &stats->tx_bytes, &stats->tx_packets, &stats->tx_errors, &stats->tx_dropped);

            if (fields >= NETWORK_STAT_FIELDS_REQUIRED)
            {
                strncpy(stats->interface_name, interface_name,
                        sizeof(stats->interface_name) - BUFFER_NULL_TERMINATOR_SPACE);
                fclose(fp);
                return SUCCESS;
            }
        }
    }

    fclose(fp);
    fprintf(stderr, "Network interface %s not found in /proc/net/dev\n", interface);
    return ERROR;
}

// Calcular métricas de red
void calculate_network_metrics(network_interface_stats_t* current, network_interface_stats_t* previous,
                               double time_delta, network_metrics_t* metrics)
{
    // Tasas de transferencia en bytes por segundo
    metrics->rx_rate_bps = (double)(current->rx_bytes - previous->rx_bytes) / time_delta;
    metrics->tx_rate_bps = (double)(current->tx_bytes - previous->tx_bytes) / time_delta;

    // Tasas de paquetes por segundo
    metrics->rx_packet_rate = (double)(current->rx_packets - previous->rx_packets) / time_delta;
    metrics->tx_packet_rate = (double)(current->tx_packets - previous->tx_packets) / time_delta;

    // Tasas de error
    unsigned long long rx_error_delta = current->rx_errors - previous->rx_errors;
    unsigned long long tx_error_delta = current->tx_errors - previous->tx_errors;
    unsigned long long rx_packet_delta = current->rx_packets - previous->rx_packets;
    unsigned long long tx_packet_delta = current->tx_packets - previous->tx_packets;

    // Calcular porcentaje de errores
    metrics->rx_error_rate = (rx_packet_delta > NO_PACKETS)
                                 ? ((double)rx_error_delta / rx_packet_delta) * PERCENTAGE_MULTIPLIER
                                 : ZERO_VALUE_DOUBLE;
    metrics->tx_error_rate = (tx_packet_delta > NO_PACKETS)
                                 ? ((double)tx_error_delta / tx_packet_delta) * PERCENTAGE_MULTIPLIER
                                 : ZERO_VALUE_DOUBLE;

    // Uso total de ancho de banda
    metrics->total_bandwidth_usage = metrics->rx_rate_bps + metrics->tx_rate_bps;
}

int get_process_stats(process_stats_t* stats)
{
    DIR* proc_dir;
    struct dirent* entry;
    FILE* fp;
    char path[PATH_BUFFER_SIZE]; // Aumentar tamaño del buffer
    char state;

    // Inicializar contadores
    stats->total_processes = NO_PROCESSES;
    stats->running_processes = NO_PROCESSES;
    stats->sleeping_processes = NO_PROCESSES;
    stats->stopped_processes = NO_PROCESSES;
    stats->zombie_processes = NO_PROCESSES;

    // Abrir directorio /proc
    proc_dir = opendir("/proc");
    if (proc_dir == NULL)
    {
        perror("Error opening /proc directory");
        return ERROR;
    }

    // Recorrer todos los directorios numéricos (PIDs)
    while ((entry = readdir(proc_dir)) != NULL)
    {
        // Verificar si el nombre del directorio es un número (PID)
        if (!isdigit(entry->d_name[FIRST_CHAR_INDEX]))
        {
            continue;
        }

        // Construir ruta al archivo stat del proceso - usar buffer más grande
        int ret = snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
        if (ret >= (int)sizeof(path))
        {
            // Ruta demasiado larga, saltar este proceso
            continue;
        }

        fp = fopen(path, "r");
        if (fp == NULL)
        {
            // El proceso puede haber terminado, continuar
            continue;
        }

        // Leer solo los primeros campos necesarios
        // Formato: pid (comm) state ...
        char comm[COMMAND_NAME_SIZE];
        int pid;
        if (fscanf(fp, "%d %s %c", &pid, comm, &state) == EXPECTED_FSCANF_FIELDS)
        {
            stats->total_processes++;

            // Clasificar por estado según /proc/[pid]/stat
            switch (state)
            {
            case PROCESS_STATE_RUNNING: // Running
                stats->running_processes++;
                break;
            case PROCESS_STATE_SLEEPING:              // Sleeping (interruptible)
            case PROCESS_STATE_UNINTERRUPTIBLE_SLEEP: // Sleeping (uninterruptible)
            case PROCESS_STATE_IDLE:                  // Idle
                stats->sleeping_processes++;
                break;
            case PROCESS_STATE_STOPPED:          // Stopped (by job control signal)
            case PROCESS_STATE_STOPPED_DEBUGGER: // Stopped (by debugger)
                stats->stopped_processes++;
                break;
            case PROCESS_STATE_ZOMBIE: // Zombie
                stats->zombie_processes++;
                break;
            default:
                // Estados menos comunes, contar como sleeping
                stats->sleeping_processes++;
                break;
            }
        }

        fclose(fp);
    }

    closedir(proc_dir);

    printf("Process Stats - Total: %lu, Running: %lu, Sleeping: %lu, Stopped: %lu, Zombie: %lu\n",
           stats->total_processes, stats->running_processes, stats->sleeping_processes, stats->stopped_processes,
           stats->zombie_processes);

    return SUCCESS;
}

int get_context_stats(context_stats_t* stats)
{
    FILE* fp;
    char line[LINE_BUFFER_SIZE];
    int found_fields = NO_PROCESSES;

    // Inicializar valores
    stats->context_switches = NO_BYTES;
    stats->processes_created = NO_PROCESSES;
    stats->interrupts = NO_BYTES;
    stats->soft_interrupts = NO_BYTES;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("Error opening /proc/stat");
        return ERROR;
    }

    // Leer línea por línea buscando las métricas específicas
    while (fgets(line, sizeof(line), fp) && found_fields < MAX_EXPECTED_CONTEXT_FIELDS)
    {
        // Buscar cambios de contexto
        if (sscanf(line, "ctxt %llu", &stats->context_switches) == SINGLE_MATCH)
        {
            found_fields++;
            continue;
        }

        // Buscar procesos creados
        if (sscanf(line, "processes %llu", &stats->processes_created) == SINGLE_MATCH)
        {
            found_fields++;
            continue;
        }

        // Buscar interrupciones (primera línea que empiece con "intr")
        if (strncmp(line, "intr ", INTR_PREFIX_LENGTH) == SUCCESS)
        {
            // La línea intr tiene formato: "intr total [individual counts...]"
            char* space = strchr(line + INTR_PREFIX_LENGTH, SPACE_CHAR);
            if (space)
            {
                *space = STRING_TERMINATOR; // Terminar string en el primer espacio
                stats->interrupts = strtoull(line + INTR_PREFIX_LENGTH, NULL, BASE_10);
                found_fields++;
            }
            continue;
        }

        // Buscar soft interrupts
        if (strncmp(line, "softirq ", SOFTIRQ_PREFIX_LENGTH) == SUCCESS)
        {
            // Similar a intr: "softirq total [individual counts...]"
            char* space = strchr(line + SOFTIRQ_PREFIX_LENGTH, SPACE_CHAR);
            if (space)
            {
                *space = STRING_TERMINATOR;
                stats->soft_interrupts = strtoull(line + SOFTIRQ_PREFIX_LENGTH, NULL, BASE_10);
                found_fields++;
            }
            continue;
        }
    }

    fclose(fp);

    if (found_fields < MIN_REQUIRED_CONTEXT_FIELDS)
    { // Al menos ctxt y processes son críticos
        fprintf(stderr, "Could not find required context switch statistics in /proc/stat\n");
        return ERROR;
    }

    printf("Context Stats - Switches: %llu, Processes: %llu, Interrupts: %llu, SoftIRQ: %llu\n",
           stats->context_switches, stats->processes_created, stats->interrupts, stats->soft_interrupts);

    return SUCCESS;
}

void calculate_system_performance_metrics(context_stats_t* current_context, context_stats_t* previous_context,
                                          process_stats_t* current_process, double time_delta,
                                          system_performance_metrics_t* metrics)
{
    // Calcular tasas por segundo
    metrics->context_switch_rate =
        (double)(current_context->context_switches - previous_context->context_switches) / time_delta;

    metrics->process_creation_rate =
        (double)(current_context->processes_created - previous_context->processes_created) / time_delta;

    metrics->interrupt_rate = (double)(current_context->interrupts - previous_context->interrupts) / time_delta;

    // Calcular ratio de carga de procesos (indicador de sobrecarga)
    if (current_process->total_processes > NO_PROCESSES)
    {
        metrics->process_load_ratio = (double)current_process->running_processes / current_process->total_processes;
    }
    else
    {
        metrics->process_load_ratio = ZERO_VALUE_DOUBLE;
    }

    printf("System Performance - Context switches/s: %.*f, Process creation/s: %.*f, Load ratio: %.*f\n",
           PRINTF_DECIMAL_PRECISION, metrics->context_switch_rate, PRINTF_DECIMAL_PRECISION,
           metrics->process_creation_rate, PRINTF_RATIO_PRECISION, metrics->process_load_ratio);
}