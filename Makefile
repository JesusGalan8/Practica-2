# NO TOCAR / NOT MODIFIED ME ##
CC=gcc
FLAGS=-Wno-implicit-function-declaration
CFLAGS=-I.
###############################

# Makefile para compilar scripter y mygrep

# Objetivo por defecto que compila ambos programas
all: scripter mygrep

# Regla para compilar el programa scripter a partir de scripter.c
scripter: scripter.c
	$(CC) $(CFLAGS) -o scripter scripter.c  # Compilar scripter.c y generar el ejecutable "scripter"

# Regla para compilar el programa mygrep a partir de mygrep.c
mygrep: mygrep.c
	$(CC) $(CFLAGS) -o mygrep mygrep.c      # Compilar mygrep.c y generar el ejecutable "mygrep"

# Regla para limpiar los ejecutables y ficheros objeto generados
clean:
	rm -f scripter mygrep *.o  # Eliminar "scripter", "mygrep" y cualquier archivo objeto (*.o)
