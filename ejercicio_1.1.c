#include <stdio.h>           // Biblioteca para entrada/salida estándar
#include <stdlib.h>          // Biblioteca para funciones estándar (malloc, free, exit, etc.)
#include <string.h>          // Biblioteca para manipulación de cadenas
#include <unistd.h>          // Biblioteca para llamadas al sistema (read, write, close, etc.)
#include <sys/wait.h>        // Biblioteca para manejo de procesos (wait, waitpid)
#include <fcntl.h>           // Biblioteca para operaciones con ficheros (open, O_RDONLY, etc.)
#include <signal.h>          // Biblioteca para manejo de señales
#include <errno.h>           // Biblioteca para manejo de errores (errno)

/* Constantes definidas */
const int max_line = 1024;       // Número máximo de caracteres por línea
const int max_commands = 10;     // Número máximo de comandos encadenados con pipe
#define max_redirections 3       // Número máximo de redirecciones: stdin, stdout y stderr
#define max_args 15              // Número máximo de argumentos por comando

/* Variables globales a utilizar en el programa */
char * argvv[max_args];          // Arreglo para almacenar el comando y sus argumentos
char * filev[max_redirections];   // Arreglo para almacenar las redirecciones (0: entrada, 1: salida, 2: error)
int background = 0;              // Indicador para ejecución en background (1) o en foreground (0)

/**
 * Función para tokenizar una línea en tokens utilizando un delimitador dado.
 * @param linea: Cadena a tokenizar.
 * @param delim: Delimitador utilizado para separar tokens.
 * @param tokens: Arreglo donde se almacenarán los tokens.
 * @param max_tokens: Número máximo de tokens a extraer.
 * @return: Número de tokens encontrados.
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
    int i = 0;  // Contador para los tokens
    char *token = strtok(linea, delim); // Extrae el primer token usando strtok
    while (token != NULL && i < max_tokens - 1) {  // Mientras se encuentren tokens y no se exceda el límite
        tokens[i++] = token;      // Almacena el token en el arreglo
        token = strtok(NULL, delim); // Extrae el siguiente token
    }
    tokens[i] = NULL;  // Coloca NULL al final del arreglo de tokens
    return i;  // Retorna el número de tokens encontrados
}

/**
 * Función para procesar las redirecciones en la línea de comandos.
 * Busca los símbolos de redirección "<", ">" y "!>" y almacena en filev el nombre del fichero.
 */
void procesar_redirecciones(char *args[]) {
    // Inicializa todas las posiciones de filev a NULL (sin redirección)
    filev[0] = NULL;
    filev[1] = NULL;
    filev[2] = NULL;
    // Recorre el arreglo de argumentos
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {  // Si se detecta redirección de entrada
            filev[0] = args[i+1];  // Guarda el nombre del fichero para entrada
            args[i] = NULL;        // Anula el token del símbolo de redirección
            args[i + 1] = NULL;    // Anula el token con el nombre del fichero
        } else if (strcmp(args[i], ">") == 0) {  // Si se detecta redirección de salida
            filev[1] = args[i+1];  // Guarda el nombre del fichero para salida
            args[i] = NULL;        // Anula el token del símbolo de redirección
            args[i + 1] = NULL;    // Anula el token con el nombre del fichero
        } else if (strcmp(args[i], "!>") == 0) {  // Si se detecta redirección de error
            filev[2] = args[i+1];  // Guarda el nombre del fichero para error
            args[i] = NULL;        // Anula el token del símbolo de redirección
            args[i + 1] = NULL;    // Anula el token con el nombre del fichero
        }
    }
}

/**
 * Función para procesar una línea de comandos.
 * Separa la línea en comandos (por pipes), tokeniza cada uno, detecta redirecciones
 * y verifica si el comando debe ejecutarse en background.
 * @param linea: Cadena que contiene la línea de comandos.
 * @return: Número de comandos (segmentos) encontrados en la línea.
 */
int procesar_linea(char *linea) {
    char *comandos[max_commands];  // Arreglo para almacenar los comandos separados por "|"
    int num_comandos = tokenizar_linea(linea, "|", comandos, max_commands); // Tokeniza la línea usando "|" como delimitador

    /* Se verifica si el último comando contiene '&' para indicar ejecución en background */
    if (strchr(comandos[num_comandos - 1], '&')) {
        background = 1;  // Se establece que la ejecución es en background
        char *pos = strchr(comandos[num_comandos - 1], '&'); 
        *pos = '\0'; // Se elimina el carácter '&' de la cadena
    } else {
        background = 0; // Si no hay '&', la ejecución es en foreground
    }

    // Procesa cada uno de los comandos encontrados
    for (int i = 0; i < num_comandos; i++) {
        // Tokeniza el comando usando espacios, tabuladores y saltos de línea
        int args_count = tokenizar_linea(comandos[i], " \t\n", argvv, max_args);
        // Procesa las redirecciones dentro del comando
        procesar_redirecciones(argvv);

        /********* BLOQUE DE DEPURACIÓN: Imprime el comando, argumentos, redirecciones y estado de background **********/
        printf("Comando = %s\n", argvv[0] ? argvv[0] : "NULL");  // Imprime el nombre del comando
        for (int arg = 1; arg < max_args; arg++) {  // Recorre los posibles argumentos
            if (argvv[arg] != NULL)
                printf("Args = %s\n", argvv[arg]);  // Imprime cada argumento no nulo
        }
        printf("Background = %d\n", background);  // Imprime si la ejecución es en background (1) o en foreground (0)
        if (filev[0] != NULL)
            printf("Redir [IN] = %s\n", filev[0]);  // Imprime la redirección de entrada si existe
        if (filev[1] != NULL)
            printf("Redir [OUT] = %s\n", filev[1]); // Imprime la redirección de salida si existe
        if (filev[2] != NULL)
            printf("Redir [ERR] = %s\n", filev[2]); // Imprime la redirección de error si existe
        /********************************************************************************************************************/
    }

    return num_comandos; // Retorna el número de comandos procesados
}

/**
 * Función principal del programa.
 * Realiza las siguientes tareas:
 * - Verifica que se pase el fichero de comandos como argumento.
 * - Abre el fichero de comandos usando la llamada al sistema open.
 * - Lee el fichero carácter a carácter para construir cada línea.
 * - Verifica que la primera línea sea "## Script de SSOO" y que no existan líneas vacías.
 * - Procesa cada línea de comandos mediante la función procesar_linea.
 */
int main(int argc, char *argv[]) {
    int fd, bytes_read;      // Variables para el descriptor del fichero y cantidad de bytes leídos
    char buffer[max_line];   // Buffer para almacenar cada línea del fichero
    int index = 0;           // Índice para recorrer el buffer
    char c;                  // Variable para almacenar cada carácter leído
    int line_number = 0;     // Contador de líneas leídas

    /* Se comprueba que se haya pasado exactamente un argumento (el fichero de comandos) */
    if (argc != 2) {
        perror("Usage: ./scripter <script_file>");  // Muestra error si el número de argumentos es incorrecto
        return -1; // Termina el programa con error
    }

    /* Se abre el fichero de comandos en modo lectura utilizando open */
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {  // Si ocurre un error al abrir el fichero
        perror("Error opening file");  // Se muestra el error correspondiente
        return -1;  // Termina el programa con error
    }

    /* Se lee el fichero carácter a carácter */
    while ((bytes_read = read(fd, &c, 1)) > 0) {  // Mientras se puedan leer caracteres
        if (c == '\n') {  // Si se encuentra un salto de línea
            /* Si se detecta una línea vacía (sin caracteres) se notifica error */
            if (index == 0) {
                perror("Empty line encountered");  // Imprime mensaje de error por línea vacía
                close(fd);  // Cierra el fichero
                return -1;  // Termina el programa con error
            }
            buffer[index] = '\0'; // Se inserta el carácter nulo para terminar la cadena
            line_number++;      // Se incrementa el contador de líneas

            /* Para la primera línea se verifica que sea exactamente "## Script de SSOO" */
            if (line_number == 1) {
                if (strcmp(buffer, "## Script de SSOO") != 0) {
                    perror("Script file does not start with '## Script de SSOO'"); // Error si la primera línea no coincide
                    close(fd);  // Cierra el fichero
                    return -1;  // Termina el programa con error
                }
            } else {
                /* Para las siguientes líneas, se procesa el comando mediante la función procesar_linea */
                procesar_linea(buffer);
            }
            index = 0; // Reinicia el índice para comenzar a leer la siguiente línea
        } else {
            /* Se almacena el carácter leído en el buffer, verificando que no se exceda la longitud máxima */
            if (index < max_line - 1) {
                buffer[index++] = c;  // Añade el carácter al buffer y aumenta el índice
            } else {
                perror("Line exceeds maximum allowed length"); // Error si la línea es demasiado larga
                close(fd);  // Cierra el fichero
                return -1;  // Termina el programa con error
            }
        }
    }

    /* Si el fichero no termina con un salto de línea, se procesa la última línea leída */
    if (index > 0) {
        buffer[index] = '\0';  // Termina la cadena con el carácter nulo
        line_number++;         // Incrementa el contador de líneas
        if (line_number == 1) {  // Si es la primera línea
            if (strcmp(buffer, "## Script de SSOO") != 0) {
                perror("Script file does not start with '## Script de SSOO'"); // Verifica que sea la línea correcta
                close(fd);  // Cierra el fichero
                return -1;  // Termina el programa con error
            }
        } else {
            procesar_linea(buffer); // Procesa la última línea
        }
    }

    close(fd);  // Cierra el fichero de comandos
    return 0;   // Termina el programa con éxito
}
