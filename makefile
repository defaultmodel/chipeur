SRC_FILES= chippeur.c

# Directories used
SRC_DIR=src/
INCLUDE_DIR=include/
OBJ_DIR=obj/

# add the object file used here
OBJ_FILES=$(OBJ_DIR)chipeur.o $(OBJ_DIR)chromium.o $(OBJ_DIR)path.o $(OBJ_DIR)sqlite3.o $(OBJ_DIR)aes.o

CC=x86_64-w64-mingw32-gcc
CFLAGS=-g -fPIE -O2 -Warray-bounds -Wsequence-point -Walloc-zero -Wnull-dereference \
-Wpointer-arith -Wcast-qual -Wcast-align=strict -I$(INCLUDE_DIR)

# not needed for now
LDFLAGS= # -Wl,--strip-all

LLIB= -luuid -lole32 -lcrypt32
DEBUG=-DDEBUG

.PHONY: all help clean

all: chipeur


# The following syntax to create object file:
# $(OBJ_DIR)file1.o : $(SRC_DIR)file1.c $(INCLUDE_DIR)hdr_used1.h ... $(INCLUDE_DIR)hdr_usedn.h
#	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

# Create the object files
$(OBJ_DIR)chipeur.o : $(SRC_DIR)chipeur.c $(INCLUDE_DIR)chipeur.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)chromium.o : $(SRC_DIR)chromium.c $(INCLUDE_DIR)chromium.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)path.o : $(SRC_DIR)path.c $(INCLUDE_DIR)path.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)sqlite3.o : $(SRC_DIR)sqlite3.c $(INCLUDE_DIR)sqlite3.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)aes.o : $(SRC_DIR)aes.c $(INCLUDE_DIR)aes.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

# make the binary
chipeur: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LLIB)

clean:
	rm $(OBJ_DIR)* chipeur

help:
	@echo "chipeur:\tto create the binary of the project"
	@echo "clean:\tto remove the binary and .o files"
	@echo "help: to display this help"
