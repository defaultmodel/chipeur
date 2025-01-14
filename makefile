SRC_FILES=chipeur.c
SRC_DIR=src/
INCLUDE_DIR=include/
OBJ_DIR=obj/

# génération de Junk Code
JUNK_CODE_INSERTER=$(SRC_DIR)junk_code_inserter

# .o
OBJ_FILES=$(OBJ_DIR)chipeur.o

CC=gcc
CFLAGS=-g -fPIE -O2 -Warray-bounds -Wsequence-point -Walloc-zero -Wnull-dereference \
-Wpointer-arith -Wcast-qual -Wcast-align=strict -I$(INCLUDE_DIR)

LDFLAGS=
DEBUG=-DDEBUG

.PHONY: all clean junk

all: junk chipeur

# Junk Code
junk: $(SRC_DIR)chipeur.c $(JUNK_CODE_INSERTER)
	$(CC) -o $(JUNK_CODE_INSERTER) $(JUNK_CODE_INSERTER).c
	./$(JUNK_CODE_INSERTER) $(SRC_DIR)chipeur.c

# Compilation du fichier avec Junk Code
$(OBJ_DIR)chipeur.o: $(SRC_DIR)chipeur.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

chipeur: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -rf $(OBJ_DIR) chipeur $(JUNK_CODE_INSERTER)
