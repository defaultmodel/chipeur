// gcc -o junk_code_inserter src/junk_code_inserter.c -Iinclude    -->
// ./build.sh
#include "junk_code_inserter.h"

// Generates random junk code to obfuscate the source code.
void generate_junk_code(FILE *output) {
  unsigned int rand_value;
  rand_value = rand() % 5;  // Selects a random junk code pattern

  switch (rand_value) {
    case 0: {
      // Introduces a volatile integer and performs random bitwise operations
      int var_id = rand() % 1000;
      fprintf(output, "    volatile int random_val_%d = rand() %% 256;\n",
              var_id);
      fprintf(output, "    for (int i = 0; i < 3 + (rand() %% 3); i++) {\n");
      fprintf(
          output,
          "        random_val_%d ^= (random_val_%d + i) & (rand() %% 100);\n",
          var_id, var_id);
      fprintf(output, "    }\n");
      fprintf(output,
              "    if ((random_val_%d & (1 << (rand() %% 8))) != 0) {\n",
              var_id);
      fprintf(output, "        random_val_%d |= (rand() %% 0xFF);\n", var_id);
      fprintf(output, "    } else {\n");
      fprintf(output, "        random_val_%d &= ~(rand() %% 0x7F);\n", var_id);
      fprintf(output, "    }\n");
      break;
    }

    case 1: {
      // Allocates a small array, fills it with random values, modifies one
      // element, and frees it
      int array_id = rand() % 1000;
      int array_size = 2 + (rand() % 3);
      fprintf(
          output,
          "    volatile int* mem_block_%d = (int*)malloc(sizeof(int) * %d);\n",
          array_id, array_size);
      fprintf(output, "    if (mem_block_%d) {\n", array_id);
      fprintf(output, "        for (int i = 0; i < %d; i++) {\n", array_size);
      fprintf(
          output,
          "            mem_block_%d[i] = (i * rand()) ^ (rand() %% 0xFFFF);\n",
          array_id);
      fprintf(output, "        }\n");
      fprintf(output,
              "        mem_block_%d[rand() %% %d] ^= (rand() %% 0xFF);\n",
              array_id, array_size);
      fprintf(output, "        free((void*)mem_block_%d);\n", array_id);
      fprintf(output, "    }\n");
      break;
    }

    case 2: {
      // Creates a small heap allocation, fills it with random values, performs
      // some calculations, and frees it
      int ptr_id = rand() % 1000;
      fprintf(
          output,
          "    volatile int* heap_var_%d = (int*)malloc(sizeof(int) * 3);\n",
          ptr_id);
      fprintf(output, "    if (heap_var_%d) {\n", ptr_id);
      fprintf(output, "        for (int i = 0; i < 3; i++) {\n");
      fprintf(output, "            heap_var_%d[i] = (rand() << 16) | rand();\n",
              ptr_id);
      fprintf(output, "        }\n");
      fprintf(output,
              "        heap_var_%d[1] ^= heap_var_%d[2] * heap_var_%d[0];\n",
              ptr_id, ptr_id, ptr_id);
      fprintf(output, "        free((void*)heap_var_%d);\n", ptr_id);
      fprintf(output, "    }\n");
      break;
    }

    case 3: {
      // Introduces a control flow variable with bitwise operations and loops
      int logic_id = rand() % 1000;
      fprintf(output,
              "    volatile int control_flow_%d = rand() ^ (rand() << 16);\n",
              logic_id);
      fprintf(output,
              "    for (int i = (control_flow_%d & 0x3); i > 0; i--) {\n",
              logic_id);
      fprintf(output,
              "        control_flow_%d ^= (i * control_flow_%d) | (rand() %% "
              "0xFFFF);\n",
              logic_id, logic_id);
      fprintf(output, "    }\n");
      fprintf(output, "    if ((control_flow_%d & 0x80000000)) {\n", logic_id);
      fprintf(output, "        control_flow_%d += rand();\n", logic_id);
      fprintf(output, "    } else {\n");
      fprintf(output, "        control_flow_%d -= (rand() %% 0x3FF);\n",
              logic_id);
      fprintf(output, "    }\n");
      break;
    }

    case 4: {
      // Generates a random seed and modifies it within a loop
      int loop_id = rand() % 1000;
      fprintf(output, "    volatile int seed_%d = (rand() << 16) | rand();\n",
              loop_id);
      fprintf(output, "    volatile int modifier_%d = rand() %% 100 + 1;\n",
              loop_id);
      fprintf(output,
              "    for (int i = (seed_%d & 0x3); i < (4 + (modifier_%d & "
              "0x1)); i++) {\n",
              loop_id, loop_id);
      fprintf(output,
              "        seed_%d = ((seed_%d * 0x5DEECE66DULL + 0xB) ^ (rand() "
              "<< 16)) + modifier_%d;\n",
              loop_id, loop_id, loop_id);
      fprintf(output, "        if ((seed_%d & 0xF) == 0) break;\n", loop_id);
      fprintf(output, "    }\n");
      break;
    }
  }
}

// Generates random control flow obfuscation to make reverse engineering harder.
void generate_control_flow(FILE *output) {
  unsigned int rand_value;
  rand_value = rand() % 5;  // Selects a random control flow obfuscation pattern

  switch (rand_value) {
    case 0: {
      // Creates a volatile control state variable and modifies it based on
      // bitwise conditions
      int var_id = rand() % 1000;
      fprintf(output,
              "    volatile int control_state_%d = rand() ^ (rand() << 16);\n",
              var_id);
      fprintf(output, "    if ((control_state_%d & 0x8000) == 0) {\n", var_id);
      fprintf(output, "        control_state_%d ^= (rand() << 8) | 0xFF00FF;\n",
              var_id);
      fprintf(output, "    } else {\n");
      fprintf(output, "        control_state_%d |= (rand() %% 0xFFFF);\n",
              var_id);
      fprintf(output, "    }\n");
      break;
    }

    case 1: {
      // Declares a small buffer and modifies its contents based on random
      // values
      int buffer_id = rand() % 1000;
      fprintf(output, "    volatile int buffer_control_%d[3];\n", buffer_id);
      fprintf(output, "    for (int i = 0; i < 3; i++) {\n");
      fprintf(
          output,
          "        buffer_control_%d[i] = (rand() << 24) | (rand() << 8);\n",
          buffer_id);
      fprintf(output, "    }\n");
      fprintf(output,
              "    if ((buffer_control_%d[0] ^ buffer_control_%d[1]) > "
              "buffer_control_%d[2]) {\n",
              buffer_id, buffer_id, buffer_id);
      fprintf(output, "        buffer_control_%d[1] ^= buffer_control_%d[2];\n",
              buffer_id, buffer_id);
      fprintf(output, "    }\n");
      break;
    }

    case 2: {
      // Uses bitwise operations and conditional logic to modify a variable
      int branch_id = rand() % 1000;
      fprintf(output, "    volatile int decision_%d = rand() %% 0x7FFF;\n",
              branch_id);
      fprintf(output,
              "    volatile int bitmask_%d = (rand() << 16) | rand();\n",
              branch_id);

      fprintf(output, "    if (decision_%d > (0x7FFF * 3 / 4)) {\n", branch_id);
      fprintf(output, "        decision_%d ^= (bitmask_%d & 0x3F3F3F3F);\n",
              branch_id, branch_id);
      fprintf(output, "    } else {\n");
      fprintf(output, "        decision_%d &= ~(0xF0F0F0F);\n", branch_id);
      fprintf(output, "    }\n");
      break;
    }

    case 3: {
      // Implements an opaque predicate for obfuscation
      int rand_id = rand() % 1000;
      fprintf(output, "    /* Opaque predicate */\n");
      fprintf(output, "    int key_%d = rand() %% 255;\n", rand_id);
      fprintf(output,
              "    int predicate_%d = ((key_%d * 0x6B8B4567) & 0x42) != 0;\n",
              rand_id, rand_id);
      fprintf(output, "    if (predicate_%d) {\n", rand_id);
      fprintf(output, "        volatile int opaque_%d = rand();\n", rand_id);
      fprintf(output, "        opaque_%d ^= (opaque_%d << 16);\n", rand_id,
              rand_id);
      fprintf(output, "    }\n");
      break;
    }
    case 4: {
      // Uses a jump table with computed goto for control flow obfuscation
      int label_id = rand() % 1000;
      fprintf(output,
              "    void* jmp_table_%d[] = {&&label_%d_A, &&label_%d_B};\n",
              label_id, label_id, label_id);
      fprintf(output, "    goto *jmp_table_%d[rand() %% 2];\n", label_id);
      fprintf(output, "label_%d_A:\n", label_id);
      fprintf(output, "    volatile int control_var_%d = rand() %% 100;\n",
              label_id);
      fprintf(output, "    control_var_%d ^= (rand() %% 20);\n", label_id);
      fprintf(output, "    goto end_%d;\n", label_id);
      fprintf(output, "label_%d_B:\n", label_id);
      fprintf(output, "    volatile int alt_var_%d = rand() %% 50;\n",
              label_id);
      fprintf(output, "    alt_var_%d += (rand() %% 30);\n", label_id);
      fprintf(output, "end_%d:\n", label_id);
    } break;
  }
}

// Applies junk code and control flow obfuscation to a given source file.
void insert_obfuscation(const char *file_path) {
  char temp_file[] = "obfuscated_temp.c";
  FILE *input = fopen(file_path, "r");
  FILE *output = fopen(temp_file, "w");

  // Check if files opened successfully
  if (!input || !output) {
    perror("Error opening files");
    exit(EXIT_FAILURE);
  }

  char line[MAX_LINE_LENGTH];
  int inside_function = 0;  // Flag to track if we are inside a function
  int obfuscations_in_function =
      0;                  // Counter to limit obfuscations per function
  int inside_struct = 0;  // Flag to track if we are inside a struct definition

  // Process the input file line by line
  while (fgets(line, sizeof(line), input)) {
    // Detect struct definitions to avoid modifying them
    if (strstr(line, "struct ") || strstr(line, "typedef struct")) {
      inside_struct = 1;
    }
    if (strstr(line, "};") && inside_struct) {
      inside_struct = 0;
    }
    // Detect function start by encountering an opening brace '{' outside of a
    // struct
    if (strstr(line, "{") && !inside_struct) {
      inside_function = 1;
      obfuscations_in_function =
          0;  // Reset obfuscation counter for new function
    }
    // Detect function end by encountering a closing brace '}' outside of a
    // struct
    if (strstr(line, "}") && !inside_struct) {
      inside_function = 0;
    }

    // Insert junk code before return statements if inside a function
    if (inside_function && strstr(line, "return") &&
        obfuscations_in_function < MAX_OBFUSCATIONS_PER_FUNCTION) {
      if (rand() % 100 < JUNK_CODE_PROBABILITY) {
        generate_junk_code(output);
        obfuscations_in_function++;
      }
    }

    // Write the current line to the output file
    fputs(line, output);

    // Insert obfuscation after semicolon if inside a function and not a return
    // statement
    if (inside_function && strstr(line, ";") && !strstr(line, "return") &&
        obfuscations_in_function < MAX_OBFUSCATIONS_PER_FUNCTION) {
      // Randomly insert junk code
      if (rand() % 100 < JUNK_CODE_PROBABILITY) {
        generate_junk_code(output);
        obfuscations_in_function++;
      }
      // Randomly insert control flow obfuscation
      if (rand() % 100 < CONTROL_FLOW_PROBABILITY) {
        generate_control_flow(output);
        obfuscations_in_function++;
      }
      // Randomly insert opaque predicates for additional obfuscation
      if (rand() % 100 < OPAQUE_PREDICATE_PROBABILITY) {
        generate_control_flow(output);
        obfuscations_in_function++;
      }
    }
  }

  // Close file streams
  fclose(input);
  fclose(output);

  // Replace the original file with the obfuscated version
  if (rename(temp_file, file_path) != 0) {
    perror("Error replacing source file");
    exit(EXIT_FAILURE);
  }
  printf("Obfuscation successful for: %s\n", file_path);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <file_path>\n", argv[0]);
    return EXIT_FAILURE;
  }

  insert_obfuscation(argv[1]);
  return EXIT_SUCCESS;
}