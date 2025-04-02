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
            i = i + 1;
        } else if (strcmp(args[i], ">") == 0) {
            filev[1] = args[i+1];
            args[i] = NULL;
            args[i + 1] = NULL;
            i = i + 1;
        } else if (strcmp(args[i], "!>") == 0) {
            filev[2] = args[i+1];
            args[i] = NULL; 
            args[i + 1] = NULL;
            i = i + 1;
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
    /*Cada comando, excepto del primero y el último, nesetiran conestrarse con el anterior y el siguiente. 
    De manera que si tenemos n comandos solo necesitamos n-1 concexiones*/
    int pipes[num_comandos - 1][2];
    //Cramos un array para guardar cada identificador de cada hijo
    pid_t pids[num_comandos];
    //Creamos los pipes
    for (int i = 0; i < num_comandos - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Error al crear pipe");
            exit(-1);
        }
    } 

    //Creanis un bucle que recorre cada comnaod
    for (int i = 0; i < num_comandos; i++) {
        memset(argvv,0,sizeof(argvv));
        /*Vamos da dvidiadr el comando en partes usandoo como delimitadores
        espacio, tabubulación o salto de línea. De esta maenra en el primer 
        tolen tendrémos el nombre del coamdno y en los siguientes sus argumentos*/
        tokenizar_linea(comandos[i], " \t\n", argvv, max_args);
        procesar_redirecciones(argvv);
        
        //Creamos un nuevo hijo con fork y el resultado se almacena en pids[i]
        pids[i] = fork();
        //Si su valor es -1 significa que hubo un error
        if (pids[i] == -1) {
            perror("Error en fork");
            exit(-2);
        }
        //Si el valor del pid es 0 significa que estamos en el hijo
        if (pids[i] == 0) {
            //Si i no es 0 signfica que no es el primer comando de la cadena, entonces lo que 
            //será conectarse con un pipe al comando anterior
            if (i != 0) {
                close(0);
                if (dup(pipes[i - 1][0]) < 0) {      // Duplicamos el extremo de lectura del pipe anterior
                    perror("Error en dup para STDIN");
                    exit(-3);
                }                
            }
            //Si i no es el último coamndo, dbe enviar su salida al siguiente comando mediante el pipe
            if (i != num_comandos - 1) {
                close(1);
                if (dup(pipes[i][1]) < 0) {          // Duplicamos el extremo de escritura del pipe actual
                    perror("Error en dup para STDOUT");
                    exit(-4);
                }
            }

            //Una vez redirigidos los descriptores, entonces podemos cerrar los originales
            for (int j = 0; j < num_comandos - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            //Si filev[0] no es null entnces signifca qeu se indico un archivo para la entrada
            if (filev[0] != NULL) {
                //Abrimos el archvo en modo lectura
                int fd_in = open(filev[0], O_RDONLY);
                // Si falla mostramos error
                if (fd_in < 0) {
                    perror("Error abriendo fichero de entrada");
                    exit(-5);
                }
                close(0);
                if (dup(fd_in) < 0) {
                    perror("Error duplicando fichero de entrada");
                    exit(-6);
                }
                close(fd_in);
            }

            //Similar al anterior, si filev[1] no es null significa que debemos poner nuestro descriptor en la salida estandar
            if (filev[1] != NULL) {
                int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out < 0) {
                    perror("Error abriendo el fichero de salida");
                    exit(-7);
                }
                close(1);
                if (dup(fd_out) < 0) {
                    perror("Error duplicando fichero de salida");
                    exit(-8);
                }
                close(fd_out);
            }
            //Similar al anterior, si filev[2] no es null significa que debemos poner nuestro descriptor en la salida error
            if (filev[2] != NULL) {
                int fd_err = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_err < 0) {
                    perror("Error abriendo fichero de error");
                    exit(-9);
                }
                close(2);
                if (dup(fd_err) < 0) {
                    perror("Error duplicando fichero de error");
                    exit(-10);
                }
                close(fd_err);
            }
            if (strcmp(argvv[0], "mygrep") == 0) {
                // Ejecuta "mygrep" sin especificar la ruta completa
                if (execvp("mygrep", argvv) == -1) {
                    perror("Error ejecutando mygrep");
                    exit(-11);
                }
            }
            else {
                // Para cualquier otro comando, se ejecuta normalmente.
                if (execvp(argvv[0], argvv) == -1) {
                    perror("Error ejecutando comando");
                    exit(-11);
                }
            }
        }
    }
    //Cerramos los pipes en el proceso padre ya que para este no son necesarios
    for (int i = 0; i < num_comandos - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    /*Si backgrounf es 0, significa que ejecutamos el proceso en primer plano, lo que implica que 
    el padre va a esperar a que cada uno de sus hijos termine*/
    if (background == 0) {
        for (int i = 0; i < num_comandos; i++) {
            wait(NULL);
        }
    }
    else {
        //en caso contrario el padre imprimirá el 
        for (int i = 0; i < num_comandos; i++) {
            printf("Proceso en background: %d\n", pids[i]);
        }
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
        perror("Error: número de argumentos incorrecto");
        return -12;
    }
    
    int fd;

    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror("No es posible abrir el fichero");
        return -13;
    }
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    //line es un array donde vamos a ir guardando la líena actual
    char line[max_line];
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
                    if (close(fd) == -1){
                        perror("Error cerrando el fichero");
                        return -14;
                    }
                    return -15;
                }
                // se verifica que la linea cumple con el formato esperado
                if (line_number == 0){
                    if ((es_linea_valida(line)) == -1) {
                        perror("El encabezado es diferente de ## Script de SSOO");
                        if (close(fd) == -1){
                            perror("Error cerrando el fichero");
                            return -16;
                        }                        
                        return -17;                
                    }
                }
                else{
                    // Si es válida se procesocesa la línea para separar el comando
                    procesar_linea(line);
                    background = 0;
                }
                line_number++;  // Incrementamos el contador de líneas
                line_pos = 0;   // Reiniciamos la posición para la siguiente línea

            } else {
                /*Max line es la longuitud máxima que puede tener una línea. Entonces antes de agregar un nuevo caracter debemos verificar que hay espacio para 
                agregarlos. Asimismo, usamos max_line -1 ya que la última posición esta reservada para el \0, que indica el fin de la cadana
                */
                if (line_pos < max_line - 1) {
                    line[line_pos++] = buffer[i];
                } else {
                    perror("Se excede el tamaño permitido por línea");
                    if (close(fd) == -1){
                        perror("Error cerrando el fichero");
                        return -18;
                    }                    
                    return -19;
                }
            }
        }
    }

    if (close(fd) == -1){
        perror("Error cerrando el fichero");
        return -20;
    }
    return 0;
}