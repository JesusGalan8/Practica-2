/* scripter.c */
// Librerías necesarias para entrada/salida, manejo de cadenas, procesos y errores.
#include <stdio.h>              // Funciones de entrada/salida estándar.
#include <stdlib.h>             // Funciones de asignación de memoria y salida del programa.
#include <string.h>             // Funciones para manipular cadenas de caracteres.
#include <unistd.h>             // Funciones POSIX como fork, execvp, dup2, etc.
#include <sys/types.h>          // Definiciones de tipos para procesos.
#include <fcntl.h>              // Funciones y constantes para operaciones con archivos.
#include <errno.h>              // Manejo de errores (variable errno).

#define MAX_LINE 1024           // Tamaño máximo para una línea del script.
#define MAX_ARGS 100            // Número máximo de argumentos para un comando.
#define MAX_CMDS 3              // Número máximo de comandos en una secuencia (uso de pipes).

// Prototipo de la función que procesa una línea del script.
int procesar_linea(char *linea);

int main(int argc, char *argv[]) {  // Función principal.
    if(argc != 2) {  // Verificar que se ha proporcionado exactamente un argumento.
        fprintf(stderr, "Uso: %s <archivo_script>\n", argv[0]);  // Imprimir mensaje de uso.
        exit(EXIT_FAILURE);  // Salir del programa con código de error.
    }

    FILE *fp = fopen(argv[1], "r");  // Abrir el archivo de script en modo lectura.
    if(!fp) {  // Si falla la apertura del archivo.
        perror("Error al abrir el archivo");  // Mostrar mensaje de error.
        exit(-1);  // Salir con error.
    }

    char line[MAX_LINE];  // Buffer para almacenar cada línea del archivo.
    int line_number = 0;  // Contador de líneas.
    // Leer la primera línea y verificar que sea el encabezado correcto.
    if(fgets(line, MAX_LINE, fp) == NULL) {  // Leer la primera línea del archivo.
        perror("Error al leer el archivo");  // Mostrar error en caso de fallo.
        fclose(fp);  // Cerrar el archivo.
        exit(-1);  // Salir con error.
    }
    line_number++;  // Incrementar el contador de líneas.
    line[strcspn(line, "\n")] = '\0';  // Eliminar el salto de línea al final de la cadena.
    if(strcmp(line, "## Script de SSOO") != 0) {  // Comprobar que la primera línea sea la esperada.
        perror("El archivo no es un script de SSOO");  // Mostrar error si no coincide.
        fclose(fp);  // Cerrar el archivo.
        exit(-1);  // Salir con error.
    }

    // Procesar cada línea del script.
    while(fgets(line, MAX_LINE, fp) != NULL) {  // Leer cada línea del archivo.
        line_number++;  // Incrementar el contador de líneas.
        line[strcspn(line, "\n")] = '\0';  // Eliminar el salto de línea.
        if(strlen(line) == 0) {  // Si se encuentra una línea vacía.
            perror("Línea vacía encontrada");  // Mostrar mensaje de error.
            fclose(fp);  // Cerrar el archivo.
            exit(-1);  // Salir con error.
        }
        if(procesar_linea(line) < 0) {  // Procesar la línea y verificar si hubo error.
            fclose(fp);  // Cerrar el archivo en caso de error.
            exit(-1);  // Salir con error.
        }
    }

    fclose(fp);  // Cerrar el archivo de script.
    return 0;  // Terminar el programa exitosamente.
}

int procesar_linea(char *linea) {  // Función para procesar una línea de comando.
    // Separa la línea en comandos usando el carácter '|'.
    char *commands[MAX_CMDS];  // Array para almacenar cada comando separado.
    int num_cmds = 0;  // Contador de comandos obtenidos.
    char *token = strtok(linea, "|");  // Obtener el primer comando separando por '|'.
    while(token != NULL && num_cmds < MAX_CMDS) {  // Iterar mientras existan tokens y no se exceda el límite.
        while(*token == ' ') token++;  // Eliminar espacios en blanco iniciales.
        char *end = token + strlen(token) - 1;  // Puntero al final del token.
        while(end > token && (*end == ' ')) {  // Eliminar espacios en blanco finales.
            *end = '\0';  // Reemplazar el espacio por el carácter nulo.
            end--;  // Retroceder el puntero.
        }
        commands[num_cmds++] = token;  // Almacenar el comando en el array.
        token = strtok(NULL, "|");  // Obtener el siguiente comando.
    }
    if(token != NULL) {  // Si quedan tokens, significa que hay más comandos de los permitidos.
        fprintf(stderr, "Secuencia de comandos con más de %d comandos no permitida\n", MAX_CMDS);
        return -1;  // Retornar error.
    }

    int background = 0;  // Variable para indicar ejecución en background.
    int pipefds[2*(num_cmds - 1)];  // Array para almacenar los descriptores de los pipes (se necesitan n-1 pipes).

    // Crear los pipes necesarios para la secuencia de comandos.
    for (int i = 0; i < num_cmds - 1; i++) {  // Iterar por cada pipe a crear.
        if (pipe(pipefds + i*2) < 0) {  // Crear un pipe y verificar si ocurre algún error.
            perror("Error al crear pipe");  // Mostrar error.
            return -1;  // Retornar error.
        }
    }

    pid_t pid, last_pid = 0;  // Variables para almacenar el PID del proceso y el último PID.
    int status;  // Variable para almacenar el estado de salida del proceso hijo.
    for (int i = 0; i < num_cmds; i++) {  // Iterar sobre cada comando.
        // Procesar cada comando para extraer los argumentos y redirecciones.
        char *args[MAX_ARGS];  // Array para almacenar los argumentos del comando.
        char *infile = NULL, *outfile = NULL, *errfile = NULL;  // Variables para redirecciones de entrada, salida y error.
        int arg_index = 0;  // Índice para los argumentos.
        char cmd_copy[MAX_LINE];  // Copia del comando para no modificar el original.
        strncpy(cmd_copy, commands[i], MAX_LINE);  // Copiar el comando.
        cmd_copy[MAX_LINE-1] = '\0';  // Asegurar la terminación de la cadena.

        char *arg = strtok(cmd_copy, " ");  // Dividir el comando en tokens por espacio.
        while(arg != NULL && arg_index < MAX_ARGS - 1) {  // Iterar sobre cada token.
            if(strcmp(arg, "<") == 0) {  // Si el token indica redirección de entrada.
                arg = strtok(NULL, " ");  // Obtener el nombre del fichero.
                if(arg == NULL) {  // Si falta el fichero.
                    perror("Falta fichero para redirección de entrada");  // Mostrar error.
                    return -1;  // Retornar error.
                }
                infile = arg;  // Guardar el fichero de entrada.
            } else if(strcmp(arg, ">") == 0) {  // Si el token indica redirección de salida.
                arg = strtok(NULL, " ");  // Obtener el nombre del fichero.
                if(arg == NULL) {  // Si falta el fichero.
                    perror("Falta fichero para redirección de salida");  // Mostrar error.
                    return -1;  // Retornar error.
                }
                outfile = arg;  // Guardar el fichero de salida.
            } else if(strcmp(arg, "!>") == 0) {  // Si el token indica redirección de error.
                arg = strtok(NULL, " ");  // Obtener el nombre del fichero.
                if(arg == NULL) {  // Si falta el fichero.
                    perror("Falta fichero para redirección de error");  // Mostrar error.
                    return -1;  // Retornar error.
                }
                errfile = arg;  // Guardar el fichero de error.
            } else if(strcmp(arg, "&") == 0) {  // Si el token indica ejecución en background.
                background = 1;  // Activar la bandera de background.
            } else {  // Si es un argumento normal.
                args[arg_index++] = arg;  // Almacenar el argumento.
            }
            arg = strtok(NULL, " ");  // Obtener el siguiente token.
        }
        args[arg_index] = NULL;  // Finalizar el array de argumentos con NULL.
        if(arg_index == 0) {  // Si no hay argumentos, la línea no contiene un comando válido.
            perror("Línea sin comando válido");  // Mostrar error.
            return -1;  // Retornar error.
        }

        pid = fork();  // Crear un proceso hijo.
        if(pid < 0) {  // Si fork falla.
            perror("Error en fork");  // Mostrar error.
            return -1;  // Retornar error.
        } else if(pid == 0) {  // Código del proceso hijo.
            // Si no es el primer comando, redirigir la entrada al pipe anterior.
            if(i > 0) {
                if(dup2(pipefds[(i-1)*2], 0) < 0) {  // Redirigir descriptor de entrada.
                    perror("Error en dup2 (entrada)");  // Mostrar error.
                    exit(-1);  // Salir con error.
                }
            }
            // Si no es el último comando, redirigir la salida al siguiente pipe.
            if(i < num_cmds - 1) {
                if(dup2(pipefds[i*2 + 1], 1) < 0) {  // Redirigir descriptor de salida.
                    perror("Error en dup2 (salida)");  // Mostrar error.
                    exit(-1);  // Salir con error.
                }
            }
            // Cerrar todos los descriptores de pipe heredados en el hijo.
            for (int j = 0; j < 2*(num_cmds - 1); j++) {
                close(pipefds[j]);  // Cerrar cada descriptor.
            }
            // Manejo de redirección de entrada, si se especifica.
            if(infile != NULL) {
                int fd_in = open(infile, O_RDONLY);  // Abrir el fichero de entrada.
                if(fd_in < 0) {  // Si falla la apertura.
                    perror("Error al abrir fichero de entrada");  // Mostrar error.
                    exit(-1);  // Salir con error.
                }
                if(dup2(fd_in, 0) < 0) {  // Redirigir descriptor de entrada.
                    perror("Error en redirección de entrada");  // Mostrar error.
                    exit(-1);  // Salir con error.
                }
                close(fd_in);  // Cerrar descriptor del fichero.
            }
            // Manejo de redirección de salida, si se especifica.
            if(outfile != NULL) {
                int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);  // Abrir/crear el fichero de salida.
                if(fd_out < 0) {  // Si falla la apertura.
                    perror("Error al abrir fichero de salida");  // Mostrar error.
                    exit(-1);  // Salir con error.
                }
                if(dup2(fd_out, 1) < 0) {  // Redirigir descriptor de salida.
                    perror("Error en redirección de salida");  // Mostrar error.
                    exit(-1);  // Salir con error.
                }
                close(fd_out);  // Cerrar descriptor del fichero.
            }
            // Manejo de redirección de error, si se especifica.
            if(errfile != NULL) {
                int fd_err = open(errfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);  // Abrir/crear el fichero de error.
                if(fd_err < 0) {  // Si falla la apertura.
                    perror("Error al abrir fichero de error");  // Mostrar error.
                    exit(-1);  // Salir con error.
                }
                if(dup2(fd_err, 2) < 0) {  // Redirigir descriptor de error.
                    perror("Error en redirección de error");  // Mostrar error.
                    exit(-1);  // Salir con error.
                }
                close(fd_err);  // Cerrar descriptor del fichero.
            }
            // Ejecutar el comando con execvp usando el primer argumento y el arreglo de argumentos.
            execvp(args[0], args);  // Ejecutar el comando.
            perror("Error en execvp");  // Si execvp falla, mostrar error.
            exit(-1);  // Salir con error.
        } else {
            // En el proceso padre, almacenar el PID del hijo.
            last_pid = pid;  // Guardar el último PID obtenido.
        }
    }

    // Cerrar en el padre todos los descriptores de pipe ya que ya no se necesitan.
    for (int i = 0; i < 2*(num_cmds - 1); i++) {
        close(pipefds[i]);  // Cerrar cada descriptor.
    }

    if(background) {  // Si el comando se debe ejecutar en background.
        printf("Proceso en background, pid: %d\n", last_pid);  // Imprimir el PID del proceso en background.
    } else {
        // Si se ejecuta en primer plano, esperar a que todos los procesos hijos finalicen.
        for (int i = 0; i < num_cmds; i++) {
            wait(&status);  // Esperar a cada proceso hijo.
        }
    }
    return 0;  // Retornar éxito.
}