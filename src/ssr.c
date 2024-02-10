// secure file deletion
// 1. open file
// 2. write random data | null data
// 3. close file
// 4. rename file to random name
// 5. delete file

#include <stdio.h> // printf, puts, FILE, fopen, fclose, fputc
#include <stdlib.h> // malloc, free
#include <string.h> // strcmp

#define SSR_VERSION "1.0"
// #define SILENT 1 // uncoment this line to complitly remove output

#ifdef _WIN32
    #define PATH_SEPARATOR '\\'
#else
    #define PATH_SEPARATOR '/'
#endif

void clear_file(FILE *file) {
    // get file size
    long long size = 0;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    for (long long i = 0; i < size; i++) fputc(0x00, file);
}

#define file_name_allowed_chars "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

// rename file to random name with same length
char* rename_file(char *file_path) {
    // rename file

    // get file name
    char *file_name = strrchr(file_path, PATH_SEPARATOR);
    if (file_name == NULL) file_name = file_path;
    else file_name++;

    // get file name length
    int file_name_length = strlen(file_name);

    // generate random file name
    char *new_file_name = malloc(file_name_length + 1);
    for (int i = 0; i < file_name_length; i++) new_file_name[i] = file_name_allowed_chars[rand() % strlen(file_name_allowed_chars)];
    new_file_name[file_name_length] = '\0';

    int file_path_length = strlen(file_path);
    char *new_file_path = malloc(file_path_length + 1);
    strcpy(new_file_path, file_path); // copy original file path
    // overwrite file name
    int start = file_path_length - file_name_length;
    for (int i = 0; i < file_name_length; i++) new_file_path[start + i] = new_file_name[i];
    new_file_path[file_path_length] = '\0';

    // rename file
    int result = rename(file_path, new_file_path);

    // free memory
    free(new_file_name);

    return result ? NULL : new_file_path;
}

int remove_file(char *file_path) {
    // open file
    FILE *file = fopen(file_path, "r+"); // r+ to read and write (file must exist)
    if (file == NULL) {
        #ifdef SILENT
        printf("file %s not found\n", file_path);
        #endif
        return 1; }

    clear_file(file); // clear file
    fclose(file); // close file
    char *new_file_path = rename_file(file_path); // rename file
    if (new_file_path == NULL) { free(new_file_path); return 1; }

    // delete file
    if (remove(new_file_path)) {
        free(new_file_path);
        #ifdef SILENT
        printf("renamed file %s not found\n", new_file_path);
        #endif
        return 1; }
    return 0;
}

int main(int argc, char *argv[]) {
    #ifdef SILENT
    puts("sfr " SSR_VERSION);
    puts("Usage: sfr <files paths ...> [option]\n\
option:\n\
-r: write random data instead of null\n\
-O: only obfuscate file (do not delete)\n\
-R: only obfuscate and rename file (do not delete)\n\
-q: quiet output (no output) fastests\n\
-h: display help and exit");
    #endif

    // check for file path
    if (argc < 2) {
        #ifdef SILENT
        puts("Error: no file path specified");
        #endif
        return 1;
    }

    // remove files
    for (int i = 1; i < argc; i++) if (remove_file(argv[i])) return 1;
    return 0;
}

