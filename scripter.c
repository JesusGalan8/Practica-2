#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/* CONST VARS */
const int max_line = 1024;
const int max_commands = 10;
#define max_redirections 3 //stdin, stdout, stderr
#define max_args 15
#define BUFFER_SIZE 512
#define MAX_LINE 1024
#define MAX_ARGS 15
/* VARS TO BE USED FOR THE STUDENTS */
char * argvv[max_args];
char * filev[max_redirections];
int background = 0; 

/**
 * This function splits a char* line into different tokens based on a given character
 * @return Number of tokens 
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
    int i = 0;
    char *token = strtok(linea, delim);
    while (token != NULL && i < max_tokens - 1) {
        tokens[i++] = token;
        token = strtok(NULL, delim);
    }
    tokens[i] = NULL;
    return i;
}

/**
 * This function processes the command line to evaluate if there are redirections. 
 * If any redirection is detected, the destination file is indicated in filev[i] array.
 * filev[0] for STDIN
 * filev[1] for STDOUT
 * filev[2] for STDERR
 */
void procesar_redirecciones(char *args[]) {
    //initialization for every command
    filev[0] = NULL;
    filev[1] = NULL;
    filev[2] = NULL;
    //Store the pointer to the filename if needed.
    //args[i] set to NULL once redirection is processed
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            filev[0] = args[i+1];
            args[i] = NULL;
            args[i + 1] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            filev[1] = args[i+1];
            args[i] = NULL;
            args[i + 1] = NULL;
        } else if (strcmp(args[i], "!>") == 0) {
            filev[2] = args[i+1];
            args[i] = NULL; 
            args[i + 1] = NULL;
        }
    }
}

/**
 * This function processes the input command line and returns in global variables: 
 * argvv -- command an args as argv 
 * filev -- files for redirections. NULL value means no redirection. 
 * background -- 0 means foreground; 1 background.
 */
int procesar_linea(char *linea) {
    //Esta función se usara para almacenar cada subcadena de la línea original
    char *comandos[max_commands];
    //Llamamos a tokenizar_linea para que nos divida a la función por |. La función retorna el número 
    //de comandos que teniamos en en esa línea
    int num_comandos = tokenizar_linea(linea, "|", comandos, max_commands);

    /*Utilizamos strchr para buscar & en el último comando. Buscamos en el último comando ya que si 
    enconramos & en el último comando significa que debemos ejecutar la función de background*/
    if (strchr(comandos[num_comandos - 1], '&')) {
        //Si lo encontramos ponemos background a 1
        background = 1;
        //Sustituimos & para que no se ejecute & junto al comando
        char *pos = strchr(comandos[num_comandos - 1], '&'); 
        //remove character 
        *pos = '\0';
    }
    else {
        //En caso contrario le decimos que no se ejecute de fondo
        background = 0;
    }
    // Se necesitan n-1 pipes para n comandos
    int pipes[num_comandos - 1][2];
    pid_t pids[num_comandos];

    // Crear los pipes necesarios
    for (int i = 0; i < num_comandos - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Error al crear el pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Para cada comando se crea un proceso hijo
    for (int i = 0; i < num_comandos; i++) {
        // Separamos el comando actual en tokens (comando y argumentos)
        tokenizar_linea(comandos[i], " \t\n", argvv, MAX_ARGS);
        // Detectamos redirecciones (por ejemplo, "<", ">", "!>")
        procesar_redirecciones(argvv);

        // Crear el proceso hijo para este comando
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("Error en fork");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) {  // Código del hijo
            // Si no es el primer comando, redirigimos la entrada
            if (i != 0) {
                // Cerramos la entrada estándar (descriptor 0)
                close(STDIN_FILENO);
                // Duplica el extremo de lectura del pipe anterior en STDIN
                dup(pipes[i - 1][0]);
            }
            // Si no es el último comando, redirigimos la salida
            if (i != num_comandos - 1) {
                close(STDOUT_FILENO);
                dup(pipes[i][1]);
            }

            // Cerramos todos los pipes en el proceso hijo (ya duplicados)
            for (int j = 0; j < num_comandos - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Si hay redirección de entrada de archivo, la configuramos
            if (filev[0] != NULL) {
                int fd = open(filev[0], O_RDONLY);
                if (fd < 0) {
                    perror("Error abriendo archivo de entrada");
                    exit(-1);
                }
                close(STDIN_FILENO);
                dup(fd);
                close(fd);
            }
            // Redirección de salida de archivo
            if (filev[1] != NULL) {
                int fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("Error abriendo archivo de salida");
                    exit(-1);
                }
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            }
            // Redirección de error a archivo
            if (filev[2] != NULL) {
                int fd = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("Error abriendo archivo de error");
                    exit(-1);
                }
                close(STDERR_FILENO);
                dup(fd);
                close(fd);
            }

            // (Opcional: Puedes incluir aquí printf() de depuración si lo deseas)

            // Ejecutar el comando con sus argumentos
            if (execvp(argvv[0], argvv) == -1) {
                perror("Error ejecutando comando");
                exit(-1);
            }
        } 
        // Proceso padre: si no es background, espera al hijo actual
        else {
            if (background == 0) {
                wait(NULL);
            } else {
                printf("Proceso en background: %d\n", pids[i]);
            }
        }
    }
    // En el proceso padre, cerrar todos los pipes (ya que se heredaron en los hijos)
    for (int i = 0; i < num_comandos - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    return num_comandos;
}

// verifica que la primera líena tengo ##Script SSOO
int es_linea_valida(char *linea) {
    char *inicio_correcto = "## Script de SSOO";
    
    // Compara caracter por caracter y devuelve 0 si coincide -1 de lo contrario
    for (int i = 0; inicio_correcto[i] != '\0'; i++) {
        if (linea[i] != inicio_correcto[i]) {
            return -1; // No coincide, no es válido
        }
    }
    return 0; // Sí coincide
}

int main(int argc, char *argv[]) {
    // Verificar argumentos
    if (argc != 2) {
        perror("Se han dado más argumentos de los permitidos");
        return -1;
    }
    

    /* STUDENTS CODE MUST BE HERE */
    char example_line[] = "ls -l | grep scripter | wc -l > redir_out.txt &";
    int n_commands = procesar_linea(example_line);
    int fd;

    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror("No es posible abrir el fichero");
        return -2;
    }
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    //line es un array donde vamos a ir guardando la líena actual
    char line[MAX_LINE];
    //Aqui cuardamso la posición actual en la línea
    int line_pos = 0;
    int line_number = 0;

    // mientras haya elementos que leer ejecutamos el bucle
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        //Vamos a crecorrer cada byte del buffer
        for (ssize_t i = 0; i < bytes_read; i++) {
            //Si encontramos un \n es que hemos llegado al final de la línea
            if (buffer[i] == '\n') {
                // Ponemos un \0 para indicar que hemos terminado
                line[line_pos] = '\0'; // Termina la línea

                
                if (line_pos == 0){
                    perror("La línea esta vacía");
                    close(fd);
                    return -3;
                }
                // se verifica que la linea cumple con el formato esperado
                if line_number == 0{
                    if (es_linea_valida(line)) == -1 {
                        perror("Encabezado inválido: no es un script de SSOO");
                        close(fd);
                        return -1;                
                    }
                }
                else{
                    // Si es válida se procesocesa la línea para separar el comando
                    procesar_linea(line);
                }
                line_number++;  // Incrementamos el contador de líneas
                line_pos = 0;   // Reiniciamos la posición para la siguiente línea

            } else {
                /*Max line es la longuitud máxima que puede tener una línea. Entonces antes de agregar un nuevo caracter debemos verificar que hay espacio para 
                agregarlos. Asimismo, usamos max_line -1 ya que la última posición esta reservada para el \0, que indica el fin de la cadana
                */
                if (line_pos < MAX_LINE - 1) {
                    line[line_pos++] = buffer[i];
                } else {
                    perror("Se excede el tamaño permitido");
                    close(fd);
                    return -4;
                }
            }
        }
    }

    close(fd);
    return 0;
}