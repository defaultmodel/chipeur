#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE_LENGTH 1024
#define JUNK_CODE_PROBABILITY 80 // Probabilité (%) d'insérer du Junk Code

// Fonction pour générer des instructions de Junk Code
void generate_junk_code(FILE *output) {
    int rand_value = rand() % 7; 

    switch (rand_value) {
        case 0: // Variable inutile
            fprintf(output, "    volatile int junk_var_%d = %d;\n", rand() % 1000, rand() % 1000);
            break;

        case 1: // Calcul inutile
            fprintf(output, "    volatile int calc = (%d * %d) / (%d + 1);\n", rand() % 100, rand() % 50, rand() % 10 + 1);
            break;

        case 2: // Boucle inutile
            fprintf(output, "    for (int i = 0; i < %d; i++) {\n", rand() % 10 + 1);
            fprintf(output, "        volatile int temp = i * i + %d;\n", rand() % 50);
            fprintf(output, "    }\n");
            break;

        case 3: // Condition inutile avec logique plus complexe
            fprintf(output, "    if (rand() %% 4 == 0) {\n");
            fprintf(output, "        volatile int temp = %d;\n", rand() % 100);
            fprintf(output, "    } else if (rand() %% 3 == 0) {\n");
            fprintf(output, "        volatile int temp = %d;\n", rand() % 200);
            fprintf(output, "    } else {\n");
            fprintf(output, "        volatile int temp = %d;\n", rand() % 300);
            fprintf(output, "    }\n");
            break;

        case 4: // Tableaux inutiles
            fprintf(output, "    volatile int junk_array_%d[5] = {", rand() % 1000);
            for (int i = 0; i < 5; i++) {
                fprintf(output, "%d, ", rand() % 100);
            }
            fprintf(output, "};\n");
            break;

        case 5: // Structure et initialisation inutile
            fprintf(output, "    struct JunkStruct_%d {\n", rand() % 1000);
            fprintf(output, "        int a;\n");
            fprintf(output, "        float b;\n");
            fprintf(output, "    } junk_struct;\n");
            fprintf(output, "    junk_struct.a = %d;\n", rand() % 100);
            fprintf(output, "    junk_struct.b = %.2f;\n", (rand() % 1000) / 100.0);
            break;

        case 6: // Déclaration et manipulation de pointeurs
            fprintf(output, "    volatile int *junk_ptr = NULL;\n");
            fprintf(output, "    int junk_value = %d;\n", rand() % 100);
            fprintf(output, "    junk_ptr = &junk_value;\n");
            fprintf(output, "    volatile int deref = *junk_ptr;\n");
            break;
    }
}


// Fonction pour insérer du Junk Code dans les fonctions existantes
void insert_junk_code(const char *file_path) {
    char temp_file[] = "chipeur_tmp.c";
    FILE *input = fopen(file_path, "r");
    FILE *output = fopen(temp_file, "w");

    if (!input || !output) {
        perror("Erreur lors de l'ouverture des fichiers");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];
    int inside_function = 0;

    srand(time(NULL));

    while (fgets(line, sizeof(line), input)) {
        // Copier la ligne originale
        fprintf(output, "%s", line);

        // Détecter si on entre ou sort d'une fonction
        if (strstr(line, "{")) {
            inside_function = 1;
        }
        if (strstr(line, "}")) {
            inside_function = 0;
        }

        // Insérer du Junk Code dans les fonctions existantes
        if (inside_function && strstr(line, ";") && rand() % 100 < JUNK_CODE_PROBABILITY) {
            generate_junk_code(output);
        }

        // Ajouter des fonctions inutiles entre deux définitions de fonctions
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

    // Remplacer le fichier original par le fichier temporaire
    if (rename(temp_file, file_path) != 0) {
        perror("Erreur lors de la mise à jour du fichier source");
        exit(EXIT_FAILURE);
    }
    printf("Junk Code ajouté dans : %s\n", file_path);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    insert_junk_code(argv[1]);
    return EXIT_SUCCESS;
}
