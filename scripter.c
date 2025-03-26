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
    char *comandos[max_commands];
    int num_comandos = tokenizar_linea(linea, "|", comandos, max_commands);

    //Check if background is indicated
    if (strchr(comandos[num_comandos - 1], '&')) {
        background = 1;
        char *pos = strchr(comandos[num_comandos - 1], '&'); 
        //remove character 
        *pos = '\0';
    }

    //Finish processing
    for (int i = 0; i < num_comandos; i++) {
        int args_count = tokenizar_linea(comandos[i], " \t\n", argvv, max_args);
        procesar_redirecciones(argvv);

        /********* This piece of code prints the command, args, redirections and background. **********/
        /*********************** REMOVE BEFORE THE SUBMISSION *****************************************/
        /*********************** IMPLEMENT YOUR CODE FOR PROCESSES MANAGEMENT HERE ********************/
        printf("Comando = %s\n", argvv[0]);
        for(int arg = 1; arg < max_args; arg++)
            if(argvv[arg] != NULL)
                printf("Args = %s\n", argvv[arg]); 
                
        printf("Background = %d\n", background);
        if(filev[0] != NULL)
            printf("Redir [IN] = %s\n", filev[0]);
        if(filev[1] != NULL)
            printf("Redir [OUT] = %s\n", filev[1]);
        if(filev[2] != NULL)
            printf("Redir [ERR] = %s\n", filev[2]);
        /**********************************************************************************************/
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
                    //Llamamos a ejecutar comando para que se ejecute por separado
                    ejecutar_comando(); 
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

    // 6️⃣ Cerrar el archivo
    close(fd);
    return 0;
}

/* Ejecuta el comando procesado */
void ejecutar_comando() {
    // Dividimos el proceso en 2
    int fd_0, fd_1, fd_2;
    pid_t pid = fork();

    // Código del hijo
    if (pid == 0) {  
        // Si tenemos un archivo en filev[0] es que tenemos una redirección de entrada STDIN, es decir el 0 o que va conectado al teclado
        if (filev[0] != NULL) {
            if ((fd_0= open(filev[0], O_RDONLY)) == -1) {
                perror("Error al abrir archivo de entrada");
                exit(-5);
            }
            //Copiamos el descriptor fd_1 al descriptor STDIN
            dup2(fd_0, STDIN_FILENO);
            //Cerramos el descriptor orignial
            close(fd_0);
        }

        // Si tenemos un archivo en el 1, es que tenmos una redirección a la salida STDOUT, que es el que está conectado a la pantalla
        if (filev[1] != NULL) {
            // Abrimos el fichero en modo escritura, si no existe se crea y si existe se trunca
            if ((fd_1 = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
                perror("Error al abrir archivo de salida");
                exit(-6);
            }
            //Redirigimos la salida estandar para que apunte al fichero
            dup2(fd_1, STDOUT_FILENO);
            //Cerramos el descriptor original
            close(fd_1);
        }

        // Si tenemos una archivo en 2 es que tenemos una redirección STDERR, es decir para errores
        if (filev[2] != NULL) {
            // Abrimos el fichero en modo escritura, si no existe se crea y si existe se trunca
            if ((fd_2 = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
                perror("Error al abrir archivo de error");
                exit(-7);
            }
            //Redirigimos el descriptor de errores para que apunte al fichero abierto
            dup2(fd_2, STDERR_FILENO);
            //Cerramos el descriptor origianal
            close(fd_2);
        }
        
        //agrvv[0] contiene el nomnre del comando original y agrrvv es una lista con el comando y sus argumentos. 
        //Y lo que haremos será remplazar el hijo por el comnado que tenemos en agrvv
        execvp(argvv[0], argvv);
        // Si el código ha llegado hasta aquí es que no se ha redirigido bien el hijo, entonces tiramos error
        perror("Error al ejecutar el comando");
        exit(-8);
    } else if (pid > 0) {  // Proceso padre
        if (!background) {
            wait(NULL); // Esperar si es foreground
        }
    } else {
        //Si el forck falla lanzamos error
        perror("Error en fork");
        return(-9);
    }
}