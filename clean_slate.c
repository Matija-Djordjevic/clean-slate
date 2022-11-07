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

#define FATAL_FALIURE_IF(COND, EXT_USR_MSG)\
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

#define SUCCESS_EXIT         (-1)
#define FAILURE_EXIT    (0)
#define FALIURE_RETRY   (1)


#define USE_ZEROS       (1<<0)
#define USE_ONES        (1<<1)
#define USE_PSEUDO      (1<<2)
#define USE_GOST        (1<<3)
#define USE_AIRFORCE    (1<<4)
#define USE_ARMY        (1<<5)
#define USE_HMG         (1<<6)
#define USE_DOD         (1<<7)
#define USE_PFITZNER    (1<<8)
#define USE_GUTMANN     (1<<9)

#define DISP_INFO       (1<<10)
#define DELETE_AFTER    (1<<11)
#define FORCE_OPEN      (1<<12)

#define ALGS_MASK       (1111111111)

uint32_t flags = 0;


typedef struct info {
    bool wipe_failed;
    char *error_msg;
    int prec;
    time_t start_tm;
    time_t end_time;
} w_info;

static int fd_src = -1;

// 
static int first_last = 0;

static int file_size_bytes;

// index of first file path in argv
static int firs_fp; 

struct option const long_opts[] = {
    {"help"  , no_argument      , NULL, 'h'},
    {"source", required_argument, NULL, 's'},
    {"first" , required_argument, NULL, 't'}
};
const char short_opts[] = "zoprcabdngheifs:t:";


bool wipe_source    ();
bool wipe_zeros     ();
bool wipe_ones      ();
bool wipe_pseudo    ();
bool wipe_gost      ();
bool wipe_airforce  ();
bool wipe_army      ();
bool wipe_hmg       ();
bool wipe_dod       ();
bool wipe_pfitzner  ();
bool wipe_gutmann   ();

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
Only one wipe agorithm flag can be set. If no wipe algorithm flag is set, -z flag will be defaulty set.");
    exit(EXIT_SUCCESS);
}

bool failed_here(w_info *info, const char *err_msg) {
    info->end_time = time(NULL);
    
    info->error_msg = malloc(strlen(err_msg));
    info->error_msg[0] = '\0';
    strcpy(info->error_msg, err_msg);

    info->wipe_failed = true;
    return false;
}

bool wipe_file(const char *path, w_info *info) {
    info->prec = 0;
    info->start_tm = time(NULL);
    fprintf("Wiping file: '%s'", path);

    int fd;
    if ((fd = open(path, O_RDWR)) == -1
        && errno == EACCES && flags & FORCE_OPEN
        && (chmod(path, S_IRUSR | S_IWUSR) == -1 || (fd = open(path, O_RDWR)) == -1)
        || fd == -1) return failed_here(info, "Caon't open file or change it's mode!");

    struct stat sb;;
    if (fstat(fd, &sb) == -1) return failed_here(info, "Can't get file size!");
    file_size_bytes = sb.st_blocks * 512;

    if (S_ISSOCK(sb.st_mode) || S_ISFIFO(sb.st_mode)) return failed_here(info, "File is either a Socket or a FIFO (named pipe).");

    return !(   fd_src != -1         && !wipe_source(fd, info)
             || flags & USE_ZEROS    && !wipe_zeros(fd, info)
             || flags & USE_ONES     && !wipe_ones(fd, info)
             || flags & USE_PSEUDO   && !wipe_pseudo(fd, info)
             || flags & USE_GOST     && !wipe_gost(fd, info)
             || flags & USE_AIRFORCE && !wipe_airforce(fd, info)
             || flags & USE_ARMY     && !wipe_army(fd, info)
             || flags & USE_HMG      && !wipe_hmg(fd, info)
             || flags & USE_DOD      && !wipe_dod(fd, info)
             || flags & USE_PFITZNER && !wipe_pfitzner(fd, info)
             || flags & USE_GUTMANN  && !wipe_gutmann(fd));
}

void display_wipe_info (char * const argv[], const w_info *fs_info, const int last_fp) {

    for(int i = 0; i < last_fp - firs_fp; i++) {
            fprintf(stdout, "File: '%s'\n", argv[i + firs_fp]);

            if (fs_info[i].wipe_failed) fprintf(stdout, "File couldn't be wiped: %s\n", fs_info[i].error_msg);
            
            fprintf(stdout, "Started at: %d\n", fs_info[i].start_tm);
            
            fprintf(stdout, "%s at: %d\n", (fs_info[i].wipe_failed)? "Failed" : "Finished", fs_info[i].end_time);
            
            fprintf(stdout, "Completed: %d\%\n\n",  fs_info[i].prec);
    }

}

int wipe_files (const int argc, char *argv[]) {

    int last_fp = firs_fp;
    while(argv[++last_fp] != NULL);

    bool one_failed = false;
    w_info *fs_info = malloc((last_fp - firs_fp) * sizeof(w_info));
    for (int i = firs_fp; i < last_fp; i++) {
        one_failed = (wipe_file(argv[i], &fs_info[i - firs_fp]))? one_failed : true;
    }

    display_wipe_info(argv, fs_info, last_fp - firs_fp);
    
    if (one_failed) {
        fprintf(stdout, "Run program again for failed files? [Y/N]");

        char c;
        fscanf(stdin, "%c", &c);
        if (tolower(c) == 'y') {
            int last_failed_fp = firs_fp;
            for (int i = 0; i < last_fp - firs_fp; i++) 
                if (fs_info[i].wipe_failed) 
                    argv[last_failed_fp++] = argv[i + firs_fp];

            argv[last_failed_fp] = NULL;

            free(fs_info);
            return FALIURE_RETRY;
        }

        free(fs_info);
        return FAILURE_EXIT;
    }

    free(fs_info);
    return SUCCESS_EXIT;

}

void set_flags (const int argc, char * const argv[]) {

    int opt;
    while((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'z':
                flags |= USE_ZEROS;
                break;
            case 'o':
                flags |= USE_ONES;
                break;
            case 'p':
                flags |= USE_PSEUDO;
                break;
            case 'r':
                flags |= USE_GOST;
                break;
            case 'c':
                flags |= USE_AIRFORCE;
                break;
            case 'a':
                flags |= USE_ARMY;
                break;
            case 'b':
                flags |= USE_HMG;
                break;
            case 'd':
                flags |= USE_DOD;
                break;
            case 'n':
                flags |= USE_PFITZNER;
                break;
            case 'g':
                flags |= USE_GUTMANN;
                break;

            //others
            case 'h':
                usage();
                break;
            case 'e':
                flags |= DELETE_AFTER;
                break;
            case 'i':
                flags |= DISP_INFO;
                break;
            case 'f':
                flags |= FORCE_OPEN;
                break;
            case 's':
                FATAL_FALIURE_IF((fd_src = open(optarg + 1, O_RDONLY)) == -1, "Can't open source file!");
                fprintf(stdout, "Overwriting by coping bytes from source file: \'%s\'\n(No wipe algorithm will be used)\n", optarg + 1);
                break;
            case 't':
                first_last = atoi(optarg + 1);
                FATAL_FALIURE_IF(first_last == 0 || first_last != strtol(optarg + 1, NULL, 10), "Argument to -t/--first must be a positive intiger!");
                break;
        }
    }

    bool chosen = (fd_src == -1)? false : true;
    uint32_t mask = 1;
    while (mask < ALGS_MASK) { 
        if (flags && mask) {
            FATAL_FALIURE_IF(chosen, "Only one wipe method can be selected\nType --help to display help menu!");
            chosen = true;
        }
        mask <<= 1;
    }

    if (!chosen) flags &= USE_ZEROS;

    FATAL_FALIURE_IF(optind == argc, "Missing file opperand!");
    firs_fp = optind;
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

    FATAL_FALIURE_IF(fd_src != -1 && close(fd_src) == -1, "Can't close src file!");

    return 0;
}