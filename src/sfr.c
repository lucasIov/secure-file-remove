// secure file deletion
// 1. open file
// 2. write random data | null data
// 3. close file
// 4. rename file to random name
// 5. delete file

// output interface:
// <file_path> <action>
// <file_path> - file path (e.g. ./test.txt)
// <action> - action: obfuscating, renaming, deleting, deleted, error
// if action is obfuscating: (blue)
//  <size> <role> [<progress_bar>] <percentage>
//  file size on 4 bytes: 3 digits + 1 char (B (bytes), K (kilobytes), M (megabytes), G (gigabytes), T (terabytes), P (petabytes))
//  role: /-\| used to confirm that the program is still running
//  progress_bar: [#####    ]
//  percentage: 000-100% (3 digits)
// if action is renaming: (yellow)
//  ro <new_file_path>
// if action is deleting: (light red)
//  <file_path> deleting
// if action is deleted: (green)
//  <file_path> deleted
// if action is error: (red)
//  <file_path> error: <error_message>
// example:
// ./test.txt obfuscating  18B / [#####     ]  50%
// ./test.txt renamed to ./aBcDeFgH
// ./aBcDeFgH deleting
// ./aBcDeFgH deleted
// ./aBcDeFgH error: file not found


#include <stdio.h> // printf, puts, FILE, fopen, fclose, fputc
#include <stdlib.h> // malloc, free
#include <string.h> // strcmp
#include <time.h> // used for random seed

#define SFD_VERSION "1.0"

#ifdef _WIN32
    #define PATH_SEPARATOR '\\'
    #include <windows.h> // GetConsoleScreenBufferInfo, GetStdHandle
#else
    #define PATH_SEPARATOR '/'
    #include <sys/ioctl.h> // ioctl, TIOCGWINSZ
#endif


#define opt_null 0
#define opt_random 1

#define opt_quiet 0
#define opt_graphical 3

static struct options {
    int write;        // 0 = null, 1 = random
    int verbosity;    // 0 = quiet, 3 = graphical
    int limit;        // 0 = all process, 1 = only obfuscate and rename, 2 = only obfuscate

    int file_count;   // number of files
    char** file_path; // list of file paths
    char data;        // char data to write
} options = {/*write*/
    opt_null,
    opt_graphical,
    0,
    0, NULL,
    0x00
};

static char* actual_file_path = NULL;

// == output ===
char* size_string(long long size) {
    char *size_string = malloc(5);
         if ( size < 1024)         sprintf(size_string, "%3lldB", size);
    else if ((size >>= 10) < 1024) sprintf(size_string, "%3lldK", size);
    else if ((size >>= 10) < 1024) sprintf(size_string, "%3lldM", size);
    else if ((size >>= 10) < 1024) sprintf(size_string, "%3lldG", size);
    else if ((size >>= 10) < 1024) sprintf(size_string, "%3lldT", size);
    else                           sprintf(size_string, "%3lldP", size);
    return size_string;
}

static int console_width = 80; // used for graphical output
void get_console_width() {
    #ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        console_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    #else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        console_width = w.ws_col;
    #endif
}

static char roler = '/';

void print_start_obfuscating(char *file_path, long long size) {
    char *size_s = size_string(size);
    printf("\r%s \e[34mobfuscating\e[0m %s", file_path, size_s);
    free(size_s);
}

void print_obfuscating(char *file_path, long long size, long long progress, long long total) {
    // rol
         if (roler == '/' ) roler = '-';
    else if (roler == '-' ) roler = '\\';
    else if (roler == '\\') roler = '|';
    else if (roler == '|' ) roler = '/';

    char *size_s = size_string(size);
    // printf("\r%s \e[34mobfuscating\e[0m %s - [", file_path, size_s);

    int progress_bar_size = console_width - strlen(file_path) - strlen(size_s) - 25;
    for (int i = 0; i < progress_bar_size; i++) putchar(i < progress * progress_bar_size / total ? '#' : ' ');
    printf("]  %lld%%", progress * 100 / total);

    free(size_s);
}

void print_obfuscated(char *file_path, long long size) {
    char *size_s = size_string(size);
    // printf("\r%s \e[32mobfuscated\e[0m  %s %c [", file_path, size_s, roler);
    printf("\r%s \e[32mobfuscated      \e[0m", file_path);

    // int progress_bar_size = console_width - strlen(file_path) - strlen(size_s) - 25;
    // for (int i = 0; i < progress_bar_size; i++) putchar('#');
    // fputs("]  100%", stdout);
    free(size_s);
}

void print_renaming(char *file_path, char *new_file_path) { printf("\n%s \e[33mrenamed to\e[0m %s", file_path, new_file_path); }
void print_deleting(char *file_path) { printf("\n%s \e[91mdeleting\e[0m", file_path); }
void print_deleted(char *file_path) { printf("\r%s \e[32mdeleted \e[0m\n", file_path); }
void print_error(char *file_path, char *error_message) { printf("\n%s \e[91merror\e[0m: %s\n", file_path, error_message); }
// =============

void write_random(FILE *file, long long size) {
    srand(time(NULL));
    // write random data
    fseek(file, 0, SEEK_SET);
    for (long long i = 0; i < size; i++) {
        // if (options.verbosity == opt_graphical) print_obfuscating(actual_file_path, size, i, size);
        fputc(rand(), file);
    }
}

void write_v(FILE *file, long long size) {
    // write null data
    fseek(file, 0, SEEK_SET);
    for (long long i = 0; i < size; i++) {
        // if (options.verbosity == opt_graphical) print_obfuscating(actual_file_path, size, i, size);
        fputc(options.data, file);
    }
}

void clear_file(FILE *file) {
    // get file size
    long long size = 0;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (options.verbosity == opt_graphical) print_start_obfuscating(actual_file_path, size);

    // write data
    if (options.write) write_random(file, size);
    else write_v(file, size);
    if (options.verbosity == opt_graphical) print_obfuscated(actual_file_path, size);
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

    if (options.verbosity == opt_graphical) print_renaming(file_path, new_file_path);

    // rename file
    int result = rename(file_path, new_file_path);

    // free memory
    free(new_file_name);

    return result ? NULL : new_file_path;
}

int remove_file(char *file_path) {
    actual_file_path = file_path;
    // open file
    FILE *file = fopen(file_path, "r+"); // r+ to read and write (file must exist)
    if (file == NULL) {
        if (options.verbosity == opt_graphical) print_error(file_path, "file not found");
        return 1;
    }

    clear_file(file); // clear file
    fclose(file); // close file
    if (options.limit == 2) return 0;
    char *new_file_path = rename_file(file_path); // rename file
    if (new_file_path == NULL) return 1;
    if (options.limit == 1) return 0;

    // delete file
    if (options.verbosity == opt_graphical) print_deleting(new_file_path);
    if (remove(new_file_path)) {
        if (options.verbosity == opt_graphical) print_error(new_file_path, "file not found");
        return 1;
    }
    if (options.verbosity == opt_graphical) print_deleted(new_file_path);
    return 0;
}

// == options ==
void print_version() {
    if (options.verbosity == opt_quiet) return;
    puts("sfr v" SFD_VERSION);
}

void print_help() {
    if (options.verbosity == opt_quiet) return;
    print_version();
    puts("Usage: sfr <files paths ...> [option]\n\
option:\n\
-r: write random data instead of null\n\
-O: only obfuscate file (do not delete)\n\
-R: only obfuscate and rename file (do not delete)\n\
-q: quiet output (no output) fastests\n\
-h: display help and exit");
}

void parse_option(char* str) {
    for (int i = 0; i < strlen(str); i++) {
        switch (str[i]) {
            case 'r': options.write = opt_random; break;
            case 'n': options.write = opt_null; options.data = 0x00; break;
            case 'q': options.verbosity = opt_quiet; break;
            case 'O': options.limit = 2; break;
            case 'R': options.limit = 1; break;
            case 'h': print_help(); exit(0);
            default:
                if (options.verbosity != opt_quiet) printf("Error: invalid option %c\n", str[i]);
                exit(1);
        }
    }
}

int push_file(char *file_path) {
    options.file_count++;
    options.file_path = realloc(options.file_path, options.file_count * sizeof(char*));
    options.file_path[options.file_count - 1] = file_path;
    return 0;
}
// =============

int main(int argc, char *argv[]) {
    // check for options
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') parse_option(argv[i] + 1);
        else push_file(argv[i]);
    }

    print_version();
    if (options.verbosity == opt_graphical) get_console_width();

    // check for file path
    if (options.file_count == 0) {
        if (options.verbosity != opt_quiet) puts("Error: no file path specified");
        return 1;
    }

    // remove files
    for (int i = 0; i < options.file_count; i++) if (remove_file(options.file_path[i])) return 1;
    return 0;
}

