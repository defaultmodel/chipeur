SRC_FILES= chippeur.c

# Directories used
SRC_DIR=src/
INCLUDE_DIR=include/
OBJ_DIR=obj/

# add the object file used here
OBJ_FILES=$(OBJ_DIR)chipeur.o $(OBJ_DIR)find_ssh_key.o $(OBJ_DIR)extract_file.o

CC=gcc

# -Walloc-zero
CFLAGS=-g -static -fPIE -O2 -Warray-bounds -Wsequence-point -Wnull-dereference \
-Wpointer-arith -Wcast-qual -Wcast-align -I$(INCLUDE_DIR)

# not needed for now
LDFLAGS= # -Wl,--strip-all

LLIB=
DEBUG=-DDEBUG

.PHONY: all help clean

all: chipeur


# The following syntax to create object file:
# $(OBJ_DIR)file1.o : $(SRC_DIR)file1.c $(INCLUDE_DIR)hdr_used1.h ... $(INCLUDE_DIR)hdr_usedn.h
#	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

# Create the object files
$(OBJ_DIR)chipeur.o : $(SRC_DIR)chipeur.c $(INCLUDE_DIR)chipeur.h $(INCLUDE_DIR)find_ssh_key.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)find_ssh_key.o : $(SRC_DIR)find_ssh_key.c $(INCLUDE_DIR)find_ssh_key.h $(INCLUDE_DIR)extract_file.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)extract_file.o : $(SRC_DIR)extract_file.c $(INCLUDE_DIR)extract_file.h
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
