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

enum FailStatus {
    FAILURE_EXIT    = 0,     
    FALIURE_RETRY   = 1,     
    SUCCESS_EXIT    = 2     
};

typedef enum WipeAlgs {
    USE_NONE        = 0,
    USE_ZEROS       = 1, 
    USE_ONES        = 2,
    USE_PSEUDO      = 3,
    USE_GOST        = 4,
    USE_AIRFORCE    = 5,
    USE_ARMY        = 6,
    USE_HMG         = 7,
    USE_DOD         = 8,
    USE_PFITZNER    = 9,
    USE_GUTMANN     = 10,
    USE_SOURCE      = 11
} WipeAlgs;

static WipeAlgs wipe_with;


typedef struct Options {
    bool displ_info;
    bool delete_after;
    bool force_open;
    int fd_src;
    int first_last;
    int file_size_bytes;
    int firs_fp;
    int last_fp;
} Options;

static Options options;


typedef struct w_info {
    bool wipe_failed;
    char **str_errors_buf;
    int buf_length;
    int prec;
    time_t start_tm;
    time_t end_time;
} w_info;


struct option const long_opts[] = {
    {"help"  , no_argument      , NULL, 'h'},
    {"source", required_argument, NULL, 's'},
    {"first" , required_argument, NULL, 't'}
};
const char short_opts[] = "zoprcabdngheifs:t:";


bool wipe_source (const int fd, w_info *info) {

    return true;
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
    -t --first=N        wipe first and last N bytes of file\n\
\n\
Only one wipe agorithm flag can be set. If more than one are set, the last one set will be selected.\n\
If no wipe algorithm flag is set, -z flag will be defaulty set.");
    exit(EXIT_SUCCESS);
}

bool failed_here(w_info *info, const char *err_msg) {
    info->end_time = time(NULL);
    
    info->str_errors_buf = realloc(info->str_errors_buf, sizeof(char *) * ++(info->buf_length));
    info->str_errors_buf[info->buf_length - 1] = malloc(strlen(err_msg) + 1);
    strcpy(info->str_errors_buf[info->buf_length - 1], err_msg);

    info->wipe_failed = true;
    return false;
}

bool wipe_file(const char *path, w_info *info) {
    info->prec = 0;
    info->buf_length = 0;
    info->str_errors_buf = NULL;
    info->start_tm = time(NULL);
    fprintf("Wiping file: '%s'", path);

    int fd;
    if ((fd = open(path, O_RDWR)) == -1
        && errno == EACCES && options.force_open
        && (chmod(path, S_IRUSR | S_IWUSR) == -1 || (fd = open(path, O_RDWR)) == -1)
        || fd == -1) return failed_here(info, "Caon't open file or change it's mode!");

    struct stat sb;;
    if (fstat(fd, &sb) == -1) return failed_here(info, "Can't get file size!");
    options.file_size_bytes = sb.st_blocks * 512;

    if (S_ISSOCK(sb.st_mode) || S_ISFIFO(sb.st_mode)) return failed_here(info, "File is either a Socket or a FIFO (named pipe).");


    return !(  wipe_with == USE_SOURCE      && !wipe_source(fd, info)
             ||wipe_with == USE_ZEROS       && !wipe_zeros(fd, info)
             ||wipe_with == USE_ONES        && !wipe_ones(fd, info)
             ||wipe_with == USE_PSEUDO      && !wipe_pseudo(fd, info)
             ||wipe_with == USE_GOST        && !wipe_gost(fd, info)
             ||wipe_with == USE_AIRFORCE    && !wipe_airforce(fd, info)
             ||wipe_with == USE_ARMY        && !wipe_army(fd, info)
             ||wipe_with == USE_HMG         && !wipe_hmg(fd, info)
             ||wipe_with == USE_DOD         && !wipe_dod(fd, info)
             ||wipe_with == USE_PFITZNER    && !wipe_pfitzner(fd, info)
             ||wipe_with == USE_GUTMANN     && !wipe_gutmann(fd, info));
}

void display_wipe_info (char * const argv[], const w_info *fs_info, const int n_fs_info) {

    for(int i = 0; i < n_fs_info; i++) {
            fprintf(stdout, "File: '%s'\n", argv[i + options.firs_fp]);

            if (fs_info[i].wipe_failed) fprintf(stdout, "File couldn't be wiped: %s\n", fs_info[i].str_errors_buf);
            
            fprintf(stdout, "Started at: %d\n", fs_info[i].start_tm);
            
            fprintf(stdout, "%s at: %d\n", (fs_info[i].wipe_failed)? "Failed" : "Finished", fs_info[i].end_time);
            
            fprintf(stdout, "Completed: %d\%\n\n",  fs_info[i].prec);
    }
}

void free_info (w_info **fs_info, const int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < (*fs_info)[i].buf_length; j++) free((*fs_info)->str_errors_buf[j]);
        free((*fs_info)[i].str_errors_buf);
    }
    free((*fs_info));
}

int wipe_files (const int argc, char *argv[]) {

    options.last_fp = options.firs_fp;
    while(argv[++options.last_fp] != NULL);

    bool one_failed = false;
    int n_fs_info = options.last_fp - options.firs_fp;
    w_info *fs_info = malloc((n_fs_info) * sizeof(w_info));
    for (int i = options.firs_fp; i < options.last_fp; i++) {
        one_failed = (wipe_file(argv[i], &fs_info[i - options.firs_fp]))? one_failed : true;
    }

    display_wipe_info(argv, fs_info, n_fs_info);
    
    if (one_failed) {
        fprintf(stdout, "Run program again for failed files? [Y/N]");

        char c;
        fscanf(stdin, "%c", &c);
        if (tolower(c) == 'y') {
            int last_failed_fp = options.firs_fp;
            for (int i = 0; i < options.last_fp - options.firs_fp; i++) 
                if (fs_info[i].wipe_failed) 
                    argv[last_failed_fp++] = argv[i + options.firs_fp];

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

void set_flags (const int argc, char * const argv[]) {
    wipe_with = USE_NONE;

    int opt;
    while((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'z':
                wipe_with = USE_ZEROS;
                break;
            case 'o':
                wipe_with = USE_ONES;
                break;
            case 'p':
                wipe_with = USE_PSEUDO;
                break;
            case 'r':
                wipe_with = USE_GOST;
                break;
            case 'c':
                wipe_with = USE_AIRFORCE;
                break;
            case 'a':
                wipe_with = USE_ARMY;
                break;
            case 'b':
                wipe_with = USE_HMG;
                break;
            case 'd':
                wipe_with = USE_DOD;
                break;
            case 'n':
                wipe_with = USE_PFITZNER;
                break;
            case 'g':
                wipe_with = USE_GUTMANN;
                break;
            case 's':
                wipe_with = USE_SOURCE;
                FAIL_IF((options.fd_src = open(optarg + 1, O_RDONLY)) == -1, "Can't open source file!");
                fprintf(stdout, "Overwriting by coping bytes from source file: \'%s\'\n(No wipe algorithm will be used)\n", optarg + 1);
                break;

            //others
            case 'h':
                usage();
                break;
            case 'e':
                options.delete_after = true;
                break;
            case 'i':
                options.displ_info = true;
                break;
            case 'f':
                options.force_open = true;
                break;
            case 't':
                options.first_last = atoi(optarg + 1);
                FAIL_IF(options.first_last == 0 || options.first_last != strtol(optarg + 1, NULL, 10), "Argument to -t/--first must be a positive intiger!");
                break;
        }
    }

    if (wipe_with == USE_NONE) wipe_with = USE_ZEROS;

    FAIL_IF(optind == argc, "Missing file opperand!");
    options.firs_fp = optind;
}

int main (int argc, char **argv) {
    time_t start_tm = time(NULL);

    set_flags(argc, argv);

    
    int fin_status;
    while((fin_status = wipe_files(argc, argv)) == FALIURE_RETRY);
    
    fprintf(stdout, "%s", (fin_status == SUCCESS_EXIT)? "%s: finished with no errors!" : "%s: finished with errors!", PROG_NAME);

    time_t diff_tm = start_tm - time(NULL);
    struct tm *lt = localtime(&diff_tm);
    fprintf(stdout, "Total time elapsed: %d days|%d hrs|%d mins|%s secs\n", lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

    FAIL_IF(options.fd_src != -1 && close(options.fd_src) == -1, "Can't close src file!");

    return 0;
}