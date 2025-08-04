# Análisis de Sistemas de Monitoreo: /proc, Prometheus y Grafana

## 1. Funcionalidad del sistema de archivos /proc en Linux y su uso para métricas

El directorio /proc en sistemas Linux constituye un sistema de archivos pseudo o virtual que actúa como interfaz directa hacia la información interna del kernel. Este mecanismo permite acceder en tiempo real al estado de procesos y recursos del sistema sin necesidad de herramientas externas complejas.

Las aplicaciones prácticas para la recopilación de métricas incluyen:

- Consulta de /proc/stat para obtener estadísticas detalladas de procesamiento
- Revisión de /proc/meminfo para monitorear el estado de la memoria del sistema  
- Exploración de directorios /proc/[pid]/ para analizar procesos individuales

Un caso de uso típico sería extraer datos de utilización del procesador mediante la lectura y procesamiento del archivo /proc/stat. Otros archivos relevantes incluyen /proc/loadavg para la carga del sistema y /proc/uptime para el tiempo de actividad.

## 2. Análisis de campos de CPU en /proc/stat y cálculo de utilización

El archivo /proc/stat contiene los siguientes campos relacionados con el tiempo de CPU:

1. user: tiempo ejecutando en modo usuario
2. nice: tiempo en modo usuario con procesos de baja prioridad  
3. system: tiempo ejecutando en modo kernel
4. idle: tiempo sin actividad
5. iowait: tiempo en espera de operaciones I/O
6. irq: tiempo procesando interrupciones hardware
7. softirq: tiempo procesando interrupciones software

El método para determinar el porcentaje de utilización implica:

1. Sumar la totalidad de valores para determinar el tiempo total transcurrido
2. Determinar el tiempo activo: tiempo total menos tiempo idle
3. Realizar dos mediciones consecutivas para calcular los deltas correspondientes
4. Aplicar la fórmula: (delta tiempo activo / delta tiempo total) × 100

*Nota: Es importante realizar las mediciones con intervalos regulares (ej: cada segundo) para obtener datos precisos y evitar fluctuaciones bruscas en los cálculos.*

## 3. Arquitectura de recopilación y visualización: Prometheus y Grafana

**Prometheus** opera como sistema de monitoreo que:
- Extrae métricas mediante peticiones HTTP programadas a endpoints específicos
- Mantiene un almacén de datos de series temporales optimizado
- Proporciona capacidades de consulta através de su lenguaje PromQL

Sus elementos fundamentales incluyen:
- Motor de recopilación y base de datos integrada
- Librerías cliente para instrumentación de aplicaciones
- Gateway para trabajos batch o de ejecución breve
- Exportadores especializados para sistemas como HAProxy, StatsD y Graphite

**Grafana** funciona como plataforma de visualización que:
- Presenta los datos almacenados en Prometheus de forma gráfica
- Facilita la construcción de tableros interactivos con múltiples visualizaciones

Sus componentes principales son:
- Interface de administración centralizada
- Elementos de visualización diversos (gráficos, tablas, medidores)
- Conectores hacia múltiples fuentes de datos
- Motor de alertas configurable

## 4. Clasificación de métricas en Prometheus

**Gauge**: Métrica que representa valores que fluctúan hacia arriba y abajo.
Aplicación: Monitoreo del uso actual de memoria RAM.

**Counter**: Métrica acumulativa que únicamente puede incrementarse.
Aplicación: Contabilización del total de peticiones HTTP atendidas.

**Histogram**: Métrica que categoriza observaciones en intervalos predefinidos.
Aplicación: Análisis de latencias de respuesta de APIs distribuidas por rangos de tiempo.

*Adicionalmente, existe el tipo **Summary** que es similar al histogram pero calcula cuantiles directamente en el cliente.*

## 5. Importancia de los mutex en contextos multi-threading

La implementación de mutex resulta fundamental al manejar métricas en aplicaciones multi-hilo para prevenir condiciones de carrera críticas. Las consecuencias de omitir esta sincronización incluyen:

- Actualizaciones concurrentes que generan lecturas y escrituras erróneas
- Inconsistencias en la información cuando un hilo lee durante una escritura de otro
- Posible corrupción total de los datos almacenados

Como ejemplo ilustrativo: si dos hilos ejecutan incrementos simultáneos sobre un contador sin protección mutex, ambos podrían leer el valor inicial idéntico, procesarlo independientemente y sobrescribir el resultado, causando que solo se registre un incremento en lugar de los dos esperados.

*Para implementar esto correctamente, se pueden usar primitivas como pthread_mutex_lock() en C/C++ o synchronized en Java.*