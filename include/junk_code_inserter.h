#ifndef JUNK_CODE_INSERTER_H
#define JUNK_CODE_INSERTER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



// Constants for junk code and control flow obfuscation probabilities
#define MAX_LINE_LENGTH 1024
#define MAX_OBFUSCATIONS_PER_FUNCTION 1
#define JUNK_CODE_PROBABILITY 15
#define CONTROL_FLOW_PROBABILITY 10
#define OPAQUE_PREDICATE_PROBABILITY 10

/**
 * @brief Generates random junk code to obfuscate the source code.
 * 
 * @param output The file pointer where the junk code will be written.
 */
void generate_junk_code(FILE *output);

/**
 * @brief Generates random control flow obfuscation to make reverse engineering harder.
 * 
 * @param output The file pointer where the control flow obfuscation will be written.
 */
void generate_control_flow(FILE *output);

/**
 * @brief Applies junk code and control flow obfuscation to a given source file.
 * 
 * @param file_path The path to the source file to be obfuscated.
 */
void insert_obfuscation(const char *file_path);

#endif // JUNK_CODE_INSERTER_H
