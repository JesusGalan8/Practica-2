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

    FILE *fp = fopen(argv[1], "r");  // Abrir el fichero especificado en modo lectura.
    if(!fp) {  // Si la apertura del fichero falla.
        perror("Error al abrir el fichero");  // Mostrar mensaje de error.
        exit(-1);  // Salir con error.
    }

    char line[MAX_LINE];  // Buffer para almacenar cada línea leída.
    int found = 0;       // Variable para indicar si se encontró la cadena.
    while(fgets(line, MAX_LINE, fp) != NULL) {  // Leer el fichero línea a línea.
        if(strstr(line, argv[2]) != NULL) {  // Buscar la cadena especificada en la línea.
            printf("%s", line);  // Imprimir la línea si contiene la cadena.
            found = 1;  // Marcar que se encontró la cadena.
        }
    }

    if(ferror(fp)) {  // Verificar si ocurrió algún error durante la lectura.
        perror("Error al leer el fichero");  // Mostrar mensaje de error.
        fclose(fp);  // Cerrar el fichero.
        exit(-1);  // Salir con error.
    }
    fclose(fp);  // Cerrar el fichero después de terminar la lectura.

    if(!found) {  // Si no se encontró la cadena en ninguna línea.
        printf("\"%s\" not found.\n", argv[2]);  // Imprimir mensaje indicando que la cadena no se encontró.
    }
    return 0;  // Terminar el programa exitosamente.
}