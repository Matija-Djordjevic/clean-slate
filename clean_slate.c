/*
    File names will always be wiped the same way, regardless of the wipe method selected.
    ext4

*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>

#define PROG_NAME "Clean Slate"

#ifndef DEBUG
    #define DEBUG_MODE 0
#else
    #define DEBUG_MODE 1
#endif

#define FAIL_IF(COND, EXT_USR_MSG)\
    do {\
        if (COND) {\
            if (DEBUG_MODE) {\
                fprintf(stderr, "File: '%s'\n"  , __FILE__);\
                fprintf(stderr, "Line: '%d'\n"  , __LINE__);\
            }\
            fprintf(stderr, "%s: %s\n", PROG_NAME, EXT_USR_MSG);\
            exit(EXIT_FAILURE);\
        }\
    } while(0);

#define DEF_BUF_SIZE (8000)

enum FinishStatus {
    FAILURE_EXIT    = 0,     
    FALIURE_RETRY   = 1,     
    SUCCESS_EXIT    = 2     
};


typedef enum WipeAlgs {
    USE_NONE        = 0,
    USE_ZEROS       = 1,  // -z    
    USE_ONES        = 2,  // -o
    USE_PSEUDO      = 3,  // -p
    USE_GOST        = 4,  // -r
    USE_AIRFORCE    = 5,  // -c
    USE_ARMY        = 6,  // -a
    USE_HMG         = 7,  // -b
    USE_DOD         = 8,  // -d
    USE_PFITZNER    = 9,  // -n
    USE_GUTMANN     = 10, // -g
    USE_SOURCE      = 11  // -s
} WipeAlgs;

static WipeAlgs wipe_with;


typedef struct Options {
    bool delete_after;  // -e
    bool displ_info;    // -i
    bool force_open;    // -f
    int first;          // -t (0 if flag is not set)
} Options;

static Options options;


typedef struct w_info {
    bool wipe_failed;
    char **errors;
    int n_errors;
    int completed_prec;
    time_t start_tm;
    time_t end_tm;
} w_info;


const struct option long_opts[] = {
    {"source", required_argument, NULL, 's'}, 
    {"help"  , no_argument      , NULL, 'h'},
    {"first" , required_argument, NULL, 't'}
};
const char short_opts[] = "zoprcabdngs:heift:";

int first_fp;   
int last_fp;
int size;


void usage() {
    fprintf(stdout, "Usage: %s [FILES] [FLAGS]", PROG_NAME);
    fprintf(stdout, "\
\n\
Wipe algorithm flags:\n\
    -z                  overwrite bytes from wile with only zeros\n\
    -o                  overwrite bytes from wile with only ones\n\
    -p                  overwrite bytes from wile with zeros and ones (randomly)\n\
    -r                  use 'Russian GOST R 50739-95' method to wipe the file\n\
    -c                  use 'US Air Force SSI-5020' method to wipe the file\n\
    -a                  use 'US Army 380-19' method to wipe the file\n\
    -b                  use 'British HMG IS5' method to wipe the file\n\
    -d                  use 'US DoD 5220.22-M (ECE)' method to wipe the file\n\
    -n                  use 'Gutmann' method to wipe the file\n\
    -g                  use 'Pfitzner' method to wipe the file\n\
    -s --source=SPATH   overwrite bytes from file with bytes from SPATH file\n\
\n\
Other flags:\n\
    -h --help           display this menu\n\
    -e                  delete file after it's been succesfully wiped\n\
    -i                  get enriched information (like status bar)\n\
    -f                  try and force open a file\n\
    -t --first=N        wipe first and last N bytes of file\n\
\n\
Only one wipe agorithm flag can be set. If more than one are set, the last one set will be selected.\n\
If no wipe algorithm flag is set, -z flag will be defaulty set.");
    exit(EXIT_SUCCESS);
}

bool wipe_zeros (const int fd, w_info *info) {

    return true;
}
bool wipe_ones (const int fd, w_info *info) {

    return true;
}
bool wipe_pseudo (const int fd, w_info *info) {

    return true;
}
bool wipe_gost (const int fd, w_info *info) {

    return true;
}
bool wipe_airforce (const int fd, w_info *info) {

    return true;
}
bool wipe_army (const int fd, w_info *info) {

    return true;
}
bool wipe_hmg (const int fd, w_info *info) {

    return true;
}
bool wipe_dod (const int fd, w_info *info) {

    return true;
}
bool wipe_pfitzner (const int fd, w_info *info) {

    return true;
}
bool wipe_gutmann (const int fd, w_info *info) {

    return true;
}


bool wipe_file_name(const char *old_path, w_info *info) { 
    // check if / or "\"
    const int file_name_beggining = (strrchr(old_path, '/'))? old_path - strrchr(old_path, '/') : 0;
    // terminated by \0?
    char *new_path = malloc(strlen(old_path) + 1);
    new_path[strlen(old_path)] = '\0';

    int iters = 100;
    while (iters) {
        int i = file_name_beggining;
        while (i < strlen(new_path)) {
            char c = rand();
            if (c != '.' || c != '..' || c != '/') {
                new_path[i++] = c;
            }
        }
        struct stat sb;
        // mkaing sure the file doesn't exist
        if (lstat(new_path, &sb) == -1) {
            // if errno indcates that file already exists - try again
            // if errno isdicates anything but that - failurer with message
        }
        rename(old_path, new_path);
        // ^ fail if this can't rename
        iters--; //on succes
    }
}

bool failed_here(w_info *info, const char *err_msg) {
    info->end_tm = time(NULL);
    info->wipe_failed = true;
    
    info->errors = realloc(info->errors, sizeof(char *) * ++(info->n_errors));
    info->errors[info->n_errors - 1] = malloc(strlen(err_msg) + 1);
    strcpy(info->errors[info->n_errors - 1], err_msg);

    return false;
}


bool wipe_file(const char *path, w_info *info) {
    info[0] = (w_info) {
        false,      // wipe_failed
        NULL,       // errors
        0,          // n_errors
        0,          // completed_prec
        time(NULL), // start_tm
        time(NULL)  // end_tm
    };

    fprintf("Wiping file: '%s'", path);

    // if the file can't be opened, try changing the users permissions opening it again
    int fd;
    if ((fd = open(path, O_RDWR)) == -1
        && errno == EACCES && options.force_open
        && (chmod(path, S_IRUSR | S_IWUSR) == -1 || (fd = open(path, O_RDWR)) == -1)
        || fd == -1) return failed_here(info, "Couldn't open the file!");

    struct stat sb;
    if (fstat(fd, &sb) == -1) return failed_here(info, "Couldn't get file size!");

    // socket or fifo files can't be overwriten
    if (S_ISSOCK(sb.st_mode) || S_ISFIFO(sb.st_mode)) return failed_here(info, "File is either a Socket or a FIFO (named pipe).");


    return !(  wipe_with == USE_ZEROS       && !wipe_zeros(fd, info)
             ||wipe_with == USE_ONES        && !wipe_ones(fd, info)
             ||wipe_with == USE_PSEUDO      && !wipe_pseudo(fd, info)
             ||wipe_with == USE_GOST        && !wipe_gost(fd, info)
             ||wipe_with == USE_AIRFORCE    && !wipe_airforce(fd, info)
             ||wipe_with == USE_ARMY        && !wipe_army(fd, info)
             ||wipe_with == USE_HMG         && !wipe_hmg(fd, info)
             ||wipe_with == USE_DOD         && !wipe_dod(fd, info)
             ||wipe_with == USE_PFITZNER    && !wipe_pfitzner(fd, info)
             ||wipe_with == USE_GUTMANN     && !wipe_gutmann(fd, info)
             
             ||options.delete_after         && !wipe_file_name(path, info));
}

void display_wipe_info (char * const argv[], const w_info *fs_info, const int n_fs_info) {

    for(int i = 0; i < n_fs_info; i++) {
            fprintf(stdout, "%d)\nFile: '%s'\n", i + 1, argv[i + first_fp]);

            fprintf(stdout, "%s\n", (fs_info[i].wipe_failed)? "Success!\n" : "Failure!\n");

            if (fs_info[i].wipe_failed) {
                fprintf(stdout, "Failure reason(s): \n");
                for (int j = 0; j < fs_info[i].n_errors; j++) {
                    fprintf(stdout, "- %s\n", fs_info[i].errors[j]);
                }
            }
            
            fprintf(stdout, "Completed: %d\%\n\n",  fs_info[i].completed_prec);
            
            fprintf(stdout, "Started at: %d\n", ctime(fs_info[i].start_tm));
            
            fprintf(stdout, "Ended at: %d\n", ctime(fs_info[i].end_tm));
    }
}

void free_info (w_info **fs_info, const int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < (*fs_info)[i].n_errors; j++) free((*fs_info)->errors[j]);
        free((*fs_info)[i].errors);
    }
    free((*fs_info));
}

int wipe_files (const int argc, char *argv[]) {
    last_fp = first_fp;
    while(argv[++last_fp] != NULL);
    int n_fs_info = last_fp - first_fp;

    bool one_failed = false;
    w_info *fs_info = malloc((n_fs_info) * sizeof(w_info));
    for (int i = first_fp; i < last_fp; i++) {
        one_failed = (wipe_file(argv[i], &fs_info[i - first_fp]))? one_failed : true;
    }

    display_wipe_info(argv, fs_info, n_fs_info);
    
    if (one_failed) {
        fprintf(stdout, "Run program again for failed files? [Y/N]");

        char c;
        fscanf(stdin, "%c", &c);
        if (tolower(c) == 'y') {
            int last_failed_fp = first_fp;
            for (int i = 0; i < n_fs_info; i++) 
                if (fs_info[i].wipe_failed) 
                    argv[last_failed_fp++] = argv[i + first_fp];

            argv[last_failed_fp] = NULL;

            free_info(fs_info, n_fs_info);
            return FALIURE_RETRY;
        }

        free_info(fs_info, n_fs_info);
        return FAILURE_EXIT;
    }

    free_info(fs_info, n_fs_info);
    return SUCCESS_EXIT;

}

void set_options (const int argc, char * const argv[]) {
    wipe_with = USE_NONE;
    options = (Options) {
        false,  // delete_after
        false,  // displ_info
        false,  // force_open
        0,      // first
    };

    int opt;
    while((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            // wipe methods
            case 'z':   wipe_with = USE_ZEROS;          break;
            case 'o':   wipe_with = USE_ONES;           break;
            case 'p':   wipe_with = USE_PSEUDO;         break;
            case 'r':   wipe_with = USE_GOST;           break;
            case 'c':   wipe_with = USE_AIRFORCE;       break;
            case 'a':   wipe_with = USE_ARMY;           break;
            case 'b':   wipe_with = USE_HMG;            break;
            case 'd':   wipe_with = USE_DOD;            break;
            case 'n':   wipe_with = USE_PFITZNER;       break;
            case 'g':   wipe_with = USE_GUTMANN;        break;
            // other flags
            case 'h':   usage();                        break;
            case 'e':   options.delete_after = true;    break;
            case 'i':   options.displ_info = true;      break;
            case 'f':   options.force_open = true;      break;
            case 't':   FAIL_IF((options.first = atoi(optarg + 1)) <= 0 
                                || options.first != strtol(optarg + 1, NULL, 10)
                                , "Argument to -t/--first must be a positive intiger!"); break;
        }
    }

    if (wipe_with == USE_NONE) wipe_with = USE_ZEROS;

    FAIL_IF((first_fp = optind) == argc, "Missing file opperand!");
}

int main (int argc, char **argv) {
    time_t start_tm = time(NULL);

    set_options(argc, argv);

    int fin_status;
    while((fin_status = wipe_files(argc, argv)) == FALIURE_RETRY);
    
    fprintf(stdout, "%s", (fin_status == SUCCESS_EXIT)? "%s: finished with no errors!" : "%s: finished with errors!", PROG_NAME);

    time_t diff_tm = start_tm - time(NULL);
    struct tm *lt = localtime(&diff_tm);
    fprintf(stdout, "Total time elapsed: %d days|%d hrs|%d mins|%s secs\n", lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);


    return 0;
}