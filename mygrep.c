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
    if ((in_fd = open(argv[1], O_RDONLY)) == -1) {
        perror("Error al abrir el fichero");
        return -2;
    }

    char buffer[BUFFER_SIZE];
    char line[MAX_LINE];
    int pos_line = 0, found = 0;
    ssize_t bytes_read;

    // Leemos el fichero de entrada carácter a carácter.
    while ((bytes_read = read(in_fd, buffer, BUFFER_SIZE)) > 0) {
        //Leemos caracter por caracter el buffer
        for (ssize_t i = 0; i < bytes_read; i++){
            // Si encontramos un salto de línea, significa que hemos llegado al final de la línea.
            if (buffer[i] == '\n' || pos_line >= MAX_LINE - 1) {
                //Si la línea coincide con la cadena que nos han pasado como argumento, la imprimimos.
                if (strstr(line, argv[2]) != NULL) {
                    printf("%s\n",line);
                    // Marcamos que hemos encontrado la cadena.
                    found = 1;
                }
                // Pasamos a la siguiente línea
                pos_line = 0;
            } else {
                // Si no hemos llegado al final de la línea, guardamos el carácter en la línea
                line[pos_line++] = buffer[i];
            }
        }
    }

    if (close(in_fd)==-1){
        perror("Error cerrando el fichero");
        return -3;
    }

    // Si no se encontró la cadena, se imprime el mensaje correspondiente.
    if (found == 0) {
        printf("\"%s\" not found.\n",argv[2]);
    }
    return 0;
}