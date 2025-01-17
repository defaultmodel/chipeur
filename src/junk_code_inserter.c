#include "junk_code_inserter.h"




// Function to generate junk code snippets
void generate_junk_code(FILE *output) {
    int rand_value = rand() % 8; // Increased to 8 for more diversity

    switch (rand_value) {
        case 0: // Unused variable
            fprintf(output, "    // Variable used for critical error management\n");
            fprintf(output, "    volatile int junk_var_%d = %d;\n", rand() % 1000, rand() % 1000);
            
            break;

        case 1: // Unnecessary computation
            fprintf(output, "    volatile int calc = (%d * %d) / (%d + 1);\n", rand() % 100, rand() % 50, rand() % 10 + 1);
            fprintf(output, "    // Intermediate calculation for performance optimization\n");
            break;

        case 2: // Useless loop
            fprintf(output, "    // Iterating through a series of critical operations\n");
            fprintf(output, "    for (int i = 0; i < %d; i++) {\n", rand() % 10 + 1);
            fprintf(output, "        volatile int temp = i * i + %d;\n", rand() % 50);
            fprintf(output, "    }\n");
            break;

        case 3: // Unnecessary condition
            fprintf(output, "    // Critical condition to adjust dynamic parameters\n");
            fprintf(output, "    if (rand() %% 4 == 0) {\n");
            fprintf(output, "        volatile int temp = %d;\n", rand() % 100);
            fprintf(output, "    } else {\n");
            fprintf(output, "        volatile int temp = %d;\n", rand() % 200);
            fprintf(output, "    }\n");
            break;

        case 4: // Useless array
            fprintf(output, "    volatile int junk_array_%d[5] = {", rand() % 1000);
            for (int i = 0; i < 5; i++) {
                fprintf(output, "%d, ", rand() % 100);
            }
            fprintf(output, "};\n");
            fprintf(output, "    // Static configuration data\n");
            break;

        case 5: // Unnecessary pointers
            fprintf(output, "    // Critical pointer manipulation for memory access security\n");
            fprintf(output, "    volatile int *junk_ptr = NULL;\n");
            fprintf(output, "    int junk_value = %d;\n", rand() % 100);
            fprintf(output, "    junk_ptr = &junk_value;\n");
            fprintf(output, "    volatile int deref = *junk_ptr;\n");
            break;

        case 6: // Useless structure
            fprintf(output, "    // Structure used for critical data serialization\n");
            fprintf(output, "    struct JunkStruct {\n");
            fprintf(output, "        int field_a;\n");
            fprintf(output, "        float field_b;\n");
            fprintf(output, "    } junk_struct;\n");
            fprintf(output, "    junk_struct.field_a = %d;\n", rand() % 100);
            fprintf(output, "    junk_struct.field_b = %.2f;\n", (rand() % 1000) / 100.0);
            break;

        case 7: // Call to an unused function
            fprintf(output, "    // Call to a critical function for validation\n");
            fprintf(output, "    junk_function_%d();\n", rand() % 1000);
            break;
    }
}

// Function to generate control flow obfuscations
void generate_control_flow(FILE *output) {
    int rand_value = rand() % 5; // Increased to 5 for more diversity

    switch (rand_value) {
        case 0: // Fake switch
            fprintf(output, "    // Managing critical cases in branch distribution\n");
            fprintf(output, "    switch (rand() %% 3) {\n");
            fprintf(output, "        case 0: break;\n");
            fprintf(output, "        case 1: break;\n");
            fprintf(output, "        case 2: break;\n");
            fprintf(output, "    }\n");
            break;

        case 1: // Simple condition
            fprintf(output, "    // Validating dynamic states\n");
            fprintf(output, "    if (rand() %% 2 == 0) {\n");
            fprintf(output, "        volatile int temp = 42;\n");
            fprintf(output, "    } else {\n");
            fprintf(output, "        volatile int temp = 84;\n");
            fprintf(output, "    }\n");
            break;

        case 2: // Fake path
            fprintf(output, "    // Critical path for random values\n");
            fprintf(output, "    if (rand() %% 3 == 0) {\n");
            fprintf(output, "        volatile int temp = rand();\n");
            fprintf(output, "    }\n");
            break;

        case 3: // Nested condition
            fprintf(output, "    // Double validation for enhanced security\n");
            fprintf(output, "    if (rand() %% 2 == 0) {\n");
            fprintf(output, "        if (rand() %% 2 == 0) {\n");
            fprintf(output, "            volatile int temp = 42;\n");
            fprintf(output, "        }\n");
            fprintf(output, "    }\n");
            break;

        case 4: // Fake label with goto
        {
        int label_id = rand() % 1000; // Generate a single label ID
        fprintf(output, "    // Managing critical jumps\n");
        fprintf(output, "    goto label_%d;\n", label_id);
        fprintf(output, "label_%d:\n", label_id);
        break;
        }
    }
}

// Function to insert junk code and control flow obfuscations into the target file
void insert_obfuscation(const char *file_path) {
    char temp_file[] = "chipeur_tmp.c";
    FILE *input = fopen(file_path, "r");
    FILE *output = fopen(temp_file, "w");

    if (!input || !output) {
        perror("Error opening files");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];
    int inside_function = 0;
    int obfuscations_in_function = 0;
    int has_returned = 0; // Indicates if a `return` statement has been encountered

    srand(time(NULL));

    while (fgets(line, sizeof(line), input)) {
        // Copy the original line
        fprintf(output, "%s", line);

        // Detect if entering or exiting a function
        if (strstr(line, "{")) {
            inside_function = 1;
            obfuscations_in_function = 0; // Reset for each function
            has_returned = 0; // Reset for each function
        }
        if (strstr(line, "}")) {
            inside_function = 0;
        }

        // Check if a `return` statement is present
        if (inside_function && strstr(line, "return")) {
            has_returned = 1;
        }

        // Add obfuscations within existing functions
        if (inside_function && !has_returned && strstr(line, ";") && obfuscations_in_function < MAX_OBFUSCATIONS_PER_FUNCTION) {
            if (rand() % 100 < JUNK_CODE_PROBABILITY) {
                generate_junk_code(output);
                obfuscations_in_function++;
            }
            if (rand() % 100 < CONTROL_FLOW_PROBABILITY) {
                generate_control_flow(output);
                obfuscations_in_function++;
            }
        }

        // Add unused functions between function definitions
        if (!inside_function && strstr(line, "}") && rand() % 100 < JUNK_CODE_PROBABILITY) {
            fprintf(output, "void junk_function_%d() {\n", rand() % 1000);
            fprintf(output, "    volatile int temp = rand();\n");
            fprintf(output, "    temp += %d;\n", rand() % 100);
            fprintf(output, "    temp *= %d;\n", rand() % 50);
            fprintf(output, "}\n\n");
        }
    }

    fclose(input);
    fclose(output);

    // Replace the original file with the obfuscated one
    if (rename(temp_file, file_path) != 0) {
        perror("Error replacing source file");
        exit(EXIT_FAILURE);
    }
    printf("Obfuscation added to: %s\n", file_path);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    insert_obfuscation(argv[1]);
    return EXIT_SUCCESS;
}
