/* mygrep.c */
// Librerías necesarias para entrada/salida, manejo de cadenas y errores.
#include <stdio.h>          // Funciones de entrada/salida.
#include <stdlib.h>         // Funciones para salir del programa.
#include <string.h>         // Funciones para manipular cadenas.
#include <errno.h>          // Manejo de errores y la variable errno.

#define MAX_LINE 1024       // Tamaño máximo permitido para una línea.

int main(int argc, char *argv[]) {  // Función principal.
    if(argc != 3) {  // Verificar que se han proporcionado exactamente dos argumentos.
        fprintf(stderr, "Uso: %s <ruta_fichero> <cadena_a_buscar>\n", argv[0]);  // Mostrar mensaje de uso.
        exit(-1);  // Salir con error.
    }
    int fd;
     // Abrir el fichero especificado en modo lectura. 
    if (( fd = open(argv[1], O_RDONLY)) ==-1); {  // Si la apertura del fichero falla.
        perror("Error al abrir el fichero");  // Mostrar mensaje de error.
        return -
        1;  // Salir con error.
    }

     // Abrir (o crear) el fichero de salida.
     int out_fd = open("salida.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
     if(out_fd < 0) {
         perror("Error al abrir/crear el fichero de salida");
         close(fd);
         return -1;
     }
 
     // Redirigir la salida estándar: cerramos STDOUT y duplicamos out_fd.
     close(STDOUT_FILENO);
     if(dup(out_fd) < 0) {
         perror("Error al duplicar el fichero de salida");
         close(in_fd);
         close(out_fd);
         return -1;
     }
     close(out_fd); // Cerramos el descriptor original, ya no es necesario.

    char line[MAX_LINE];
    int index = 0;
    int found = 0;
    char ch;
    ssize_t n;

    // Leemos el fichero carácter a carácter y vamos armando líneas.
    while ((n = read(fd, &ch, 1)) == 1) {
        if (ch == '\n' || index >= MAX_LINE - 1) {
            line[index] = '\0';
            // Si la línea contiene la cadena buscada, la imprimimos.
            if (strstr(line, argv[2]) != NULL) {
                write(STDOUT_FILENO, line, strlen(line));
                write(STDOUT_FILENO, "\n", 1);
                found = 1;
            }
            index = 0;  // Reiniciamos el índice para la siguiente línea.
        } else {
            line[index++] = ch;
        }
    }

    if (n == -1) {
        perror("Error al leer el fichero");
        close(fd);
        exit(-1);
    }
    close(fd);

    if (!found) {
        char msg[128];
        snprintf(msg, sizeof(msg), "\"%s\" not found.\n", argv[2]);
        write(STDOUT_FILENO, msg, strlen(msg));
    }
    return 0;
}
Ex