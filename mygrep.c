/* mygrep.c */

// Incluir las librerías necesarias
#include <stdio.h>          // Funciones de entrada/salida (printf, fgets, etc.)
#include <stdlib.h>         // Funciones de salida y asignación (exit, etc.)
#include <string.h>         // Funciones para manipular cadenas (strstr, strcmp, etc.)
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

// Definir constante para el tamaño máximo de una línea
#define MAX_LINE 1024       // Número máximo de caracteres por línea
#define BUFFER_SIZE 512

int main(int argc, char *argv[]) {  // Función principal, recibe argumentos desde la línea de comandos
    if (argc != 3) {  // Verificar que se han pasado exactamente dos argumentos (más el nombre del programa)
        perror("Tiene más argumentos de los permitidos");  // Mostrar mensaje de uso en caso de error
        return(-1);  // Salir con error
    }
    int in_fd,out_fd;

    // Abrir el fichero de entrada usando open (sólo llamadas al sistema).
    if(in_fd = open(argv[1], O_RDONLY) == -1) {
        perror("Error al abrir el fichero");
        return -2;
    }

      // Abrir (o crear) el fichero de salida.
    if(out_fd = open("salida.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644) == -1) {
        perror("Error al abrir/crear el fichero de salida");
        close(in_fd);
        return -3;
    }

    close(STDOUT_FILENO);
    if (dup(out_fd) == -1) {
        perror("Error al duplicar el descriptor de salida");
        close(in_fd);
        close(out_fd);
        return -2;
    }
    close(out_fd);

    char buffer[BUFFER_SIZE];
    char line[MAX_LINE];
    int pos_line = 0, found = 0;
    ssize_t bytes_read;

    // Leemos el fichero de entrada carácter a carácter.
    while ((bytes_read = read(in_fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++){}
            if (buffer[i] == '\n' || pos_line >= MAX_LINE - 1) {
                line[pos_line] = '\0';
                if (strstr(line, argv[2]) != NULL) {
                    write(STDOUT_FILENO, line, strlen(line));
                    write(STDOUT_FILENO, "\n", 1);
                    found = 1;
                }
                pos_line = 0;
            } else {
                line[pos_line++] = buffer[i];
            }
        }
    }

    close(in_fd);

    // Si no se encontró la cadena, se imprime el mensaje correspondiente.
    if (found == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "\"%s\" not found.\n", argv[2]);
        write(STDOUT_FILENO, msg, strlen(msg));
    }
    return 0;