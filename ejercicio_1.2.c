#include <stdio.h>       // Biblioteca para entrada/salida estándar.
#include <string.h>      // Biblioteca para funciones de manipulación de cadenas.
#include <stdlib.h>      // Biblioteca para funciones estándar (por ejemplo, exit).

/* Definición de constantes */
#define MAX_COMMANDS 10  // Número máximo de comandos que pueden conectarse mediante pipe.
#define MAX_ARGS 15      // Número máximo de argumentos por comando.

/* Variables globales para almacenar la información procesada */
char *argvv[MAX_ARGS];   // Arreglo que almacenará el nombre del comando y sus argumentos.
char *filev[3];          // Arreglo para almacenar las redirecciones: 
                         // filev[0] = redirección de entrada, 
                         // filev[1] = redirección de salida, 
                         // filev[2] = redirección de error.
int background = 0;      // Indicador para ejecución en background (1) o en primer plano (0).

/**
 * Función para tokenizar una línea de texto en tokens, usando un delimitador.
 * @param linea: Cadena que se desea tokenizar.
 * @param delim: Cadena con el carácter o caracteres delimitadores.
 * @param tokens: Arreglo donde se almacenarán los tokens resultantes.
 * @param max_tokens: Número máximo de tokens a extraer.
 * @return: Número de tokens encontrados.
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
    int i = 0;  // Contador de tokens.
    char *token = strtok(linea, delim);  // Extrae el primer token usando el delimitador.
    while (token != NULL && i < max_tokens - 1) {  // Mientras existan tokens y no se exceda el límite.
        tokens[i++] = token;  // Almacena el token en el arreglo.
        token = strtok(NULL, delim);  // Extrae el siguiente token.
    }
    tokens[i] = NULL;  // Termina el arreglo de tokens con un NULL.
    return i;  // Retorna el número total de tokens extraídos.
}

/**
 * Función para procesar las redirecciones en la línea de comandos.
 * Busca los símbolos de redirección (“<”, “>” y “!>”) en el arreglo de argumentos
 * y asigna el nombre del fichero correspondiente en el arreglo global filev.
 * Además, anula (pone a NULL) los tokens correspondientes en el arreglo de argumentos.
 */
void procesar_redirecciones(char *args[]) {
    // Se inicializan las posiciones de redirección a NULL (sin redirección)
    filev[0] = NULL;
    filev[1] = NULL;
    filev[2] = NULL;
    // Se recorre el arreglo de argumentos
    for (int i = 0; args[i] != NULL; i++) {
        // Si se encuentra el símbolo de redirección de entrada "<"
        if (strcmp(args[i], "<") == 0) {
            filev[0] = args[i+1];  // Se almacena el nombre del fichero para entrada.
            args[i] = NULL;        // Se elimina el token del símbolo.
            args[i+1] = NULL;      // Se elimina el token del nombre del fichero.
        }
        // Si se encuentra el símbolo de redirección de salida ">"
        else if (strcmp(args[i], ">") == 0) {
            filev[1] = args[i+1];  // Se almacena el nombre del fichero para salida.
            args[i] = NULL;
            args[i+1] = NULL;
        }
        // Si se encuentra el símbolo de redirección de error "!>"
        else if (strcmp(args[i], "!>") == 0) {
            filev[2] = args[i+1];  // Se almacena el nombre del fichero para error.
            args[i] = NULL;
            args[i+1] = NULL;
        }
    }
}

/**
 * Función para procesar una línea de comandos.
 * Esta función realiza lo siguiente:
 * 1. Separa la línea en comandos utilizando el carácter '|' como delimitador.
 * 2. Detecta si el último comando contiene '&' para ejecutar en background.
 * 3. Tokeniza cada comando en función de espacios, tabuladores y saltos de línea para obtener
 *    el nombre del comando y sus argumentos, y luego procesa las redirecciones.
 * 4. Imprime (en modo de depuración) el comando, los argumentos, las redirecciones y el estado
 *    de background.
 *
 * @param linea: Cadena que contiene la línea de comandos completa.
 * @return: Número de comandos (segmentos) encontrados en la línea.
 */
int procesar_linea(char *linea) {
    char *comandos[MAX_COMMANDS];  // Arreglo para almacenar los comandos separados por '|'
    // Se tokeniza la línea en segmentos separados por el delimitador '|'
    int num_comandos = tokenizar_linea(linea, "|", comandos, MAX_COMMANDS);

    // Se verifica si el último comando contiene el carácter '&'
    if (strchr(comandos[num_comandos - 1], '&')) {
        background = 1;  // Se establece que la ejecución es en background
        char *ampersand = strchr(comandos[num_comandos - 1], '&');
        *ampersand = '\0';  // Se elimina el carácter '&' del comando
    } else {
        background = 0;  // Se establece la ejecución en primer plano
    }

    // Se procesa cada comando obtenido
    for (int i = 0; i < num_comandos; i++) {
        // Tokeniza el comando en tokens usando espacios, tabuladores y saltos de línea
        int num_args = tokenizar_linea(comandos[i], " \t\n", argvv, MAX_ARGS);
        
        // Se procesan las redirecciones en el arreglo de argumentos
        procesar_redirecciones(argvv);

        // BLOQUE DE DEPURACIÓN: Se imprime la información procesada
        printf("Comando: %s\n", argvv[0] ? argvv[0] : "NULL");
        for (int j = 1; j < MAX_ARGS; j++) {
            if (argvv[j] != NULL)
                printf("Arg: %s\n", argvv[j]);
        }
        printf("Background: %d\n", background);
        if (filev[0] != NULL)
            printf("Redirección entrada: %s\n", filev[0]);
        if (filev[1] != NULL)
            printf("Redirección salida: %s\n", filev[1]);
        if (filev[2] != NULL)
            printf("Redirección error: %s\n", filev[2]);
        printf("----------------------------------------------------\n");
    }

    return num_comandos;  // Retorna el número total de comandos procesados en la línea.
}

/* Función main para probar el procesamiento de comandos */
int main() {
    /* Línea de ejemplo que contiene: 
       - Un comando con pipe para encadenar procesos ("ls -l", "grep scripter", "wc -l").  
       - Redirección de salida: "> salida.txt".  
       - Ejecución en background: "&".
    */
    char linea_ejemplo[] = "ls -l | grep scripter | wc -l > salida.txt &";

    // Se llama a la función procesar_linea con la línea de ejemplo
    int num_comandos = procesar_linea(linea_ejemplo);

    // Se imprime el número de comandos procesados
    printf("Número de comandos procesados: %d\n", num_comandos);

    return 0;
}
