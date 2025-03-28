# NO TOCAR / NOT MODIFIED ME ##
CC=gcc
FLAGS=-Wno-implicit-function-declaration
CFLAGS=-I.
###############################

# Makefile para compilar scripter y mygrep

# Objetos y ejecutable
OBJ = scripter.o
EXEC = scripter

all: $(EXEC)

%.o: %.c
	$(CC) $(FLAGS) $(CFLAGS) -c -o $@ $<

$(EXEC): $(OBJ)
	$(CC) $(FLAGS) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) $(EXEC)