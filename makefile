SRC_FILES= chipeur.c

# Directories used
SRC_DIR=src/
INCLUDE_DIR=include/
OBJ_DIR=obj/

# add the object file used here
OBJ_FILES=$(OBJ_DIR)chipeur.o

# Junk Code Inserter
JUNK_CODE_INSERTER=$(SRC_DIR)junk_code_inserter

CC=gcc
CFLAGS=-g -fPIE -O2 -Warray-bounds -Wsequence-point -Walloc-zero -Wnull-dereference \
-Wpointer-arith -Wcast-qual -Wcast-align=strict -I$(INCLUDE_DIR)

# not needed for now
LDFLAGS= # -Wl,--strip-all

LLIB=
DEBUG=-DDEBUG

.PHONY: all help clean

all: chipeur_no_obf chipeur_obf

# Compile the non-obfuscated version
chipeur_no_obf: $(SRC_DIR)chipeur.c $(INCLUDE_DIR)chipeur.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(DEBUG) $(CFLAGS) $< -o chipeur_no_obf

# Apply junk code obfuscation and compile the obfuscated version
chipeur_obf: $(SRC_DIR)chipeur.c $(JUNK_CODE_INSERTER).c $(INCLUDE_DIR)/junk_code_inserter.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(DEBUG) $(CFLAGS) -o $(JUNK_CODE_INSERTER) $(JUNK_CODE_INSERTER).c
	./$(JUNK_CODE_INSERTER) $(SRC_DIR)chipeur.c
	$(CC) $(DEBUG) $(CFLAGS) $(SRC_DIR)chipeur.c -o chipeur_obf

# The following syntax to create object file:
# $(OBJ_DIR)file1.o : $(SRC_DIR)file1.c $(INCLUDE_DIR)hdr_used1.h ... $(INCLUDE_DIR)hdr_usedn.h
#	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

# Create the object files
$(OBJ_DIR)chipeur.o : $(SRC_DIR)chipeur.c $(INCLUDE_DIR)chipeur.h $(INCLUDE_DIR)/junk_code_inserter.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

# make the binary
chipeur: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LLIB)

clean:
	rm -fv $(OBJ_DIR)* chipeur.exe chipeur chipeur_no_obf chipeur_obf $(JUNK_CODE_INSERTER)

help:
	@echo "chipeur:\tto create the binary of the project"
	@echo "chipeur_no_obf:\tto create the non-obfuscated version of the binary"
	@echo "chipeur_obf:\tto create the obfuscated version of the binary"
	@echo "clean:\tto remove the binary and .o files"
	@echo "help: to display this help"
