# NO TOCAR / NOT MODIFIED ME ##
CC=gcc
FLAGS=-Wno-implicit-function-declaration
CFLAGS=-I.
###############################

# Makefile para compilar scripter y mygrep

# Objetos y ejecutable
PROG1 = scripter
PROG2 = mygrep

SRC1 = scripter.c
SRC2 = mygrep.c

OBJ1 = $(SRC1:.c=.o)
OBJ2 = $(SRC2:.c=.o)

all: $(PROG1) $(PROG2)

$(PROG1): $(OBJ1)
	$(CC) $(CFLAGS) -o $@ $^

$(PROG2): $(OBJ2)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PROG1) $(PROG2) $(OBJ1) $(OBJ2)

distclean: clean
	rm -f *~