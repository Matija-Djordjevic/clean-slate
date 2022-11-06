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

#define SUCCESS             (-1)
#define FAILURE_AND_EXIT    (0)
#define FAILURE_AND_RETRY   (1)


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
    int error;
    int prec;
    time_t start_tm;
    time_t end_time;
} w_info;

static int fd_source = -1;
static int fd_wipe;
static int first_last = 0;
static int file_size_bytes;
static int firs_fp; 
static struct tm *lt;

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



bool wipe_file() {

    struct stat sb;
    if (fstat(fd_wipe, &sb) == -1) 
        return false;
    file_size_bytes = sb.st_blocks * 512;

    //FIFO and socket files can't be wiped
    if (S_ISSOCK(sb.st_mode) || S_ISFIFO(sb.st_mode)) 
        return false;

    return !(   fd_source != -1      && !wipe_source()
             || flags & USE_ZEROS    && !wipe_zeros()
             || flags & USE_ONES     && !wipe_ones()
             || flags & USE_PSEUDO   && !wipe_pseudo()
             || flags & USE_GOST     && !wipe_gost()
             || flags & USE_AIRFORCE && !wipe_airforce()
             || flags & USE_ARMY     && !wipe_army()
             || flags & USE_HMG      && !wipe_hmg()
             || flags & USE_DOD      && !wipe_dod()
             || flags & USE_PFITZNER && !wipe_pfitzner()
             || flags & USE_GUTMANN  && !wipe_gutmann());
}

void set_options (const int argc, char * const argv[]) {

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
                FAIL_IF((fd_source = open(optarg + 1, O_RDONLY)) == -1, "Can't open source file!");
                fprintf(stdout, "Overwriting by coping bytes from source file: \'%s\'\n(No wipe algorithm will be used)\n", optarg + 1);
                break;
            case 't':
                first_last = atoi(optarg + 1);
                FAIL_IF(first_last == 0 || first_last != strtol(optarg + 1, NULL, 10), "Argument to -t/--first must be a positive intiger!");
                break;
        }
    }

    // if no wipe algorithm was selected and no source file was set, file(s) will be overwriten using zeros (USE_ZEROS)
    if (!(flags & ALGS_MASK) && fd_source != -1)
        flags |= USE_ZEROS;

    
    FAIL_IF(optind == argc, "Missing file opperand!");
    firs_fp = optind;

}

void display_wipe_info (char * const argv[], const w_info *fs_info, const int last_fp) {

    for(int i = 0; i < last_fp - firs_fp; i++) {
            fprintf(stdout, "File: '%s'\n", argv[i + firs_fp]);

            if (fs_info[i].wipe_failed) fprintf(stdout, "File coudn't be wiped: %s\n", strerror(fs_info[i].error));
            
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
        
        fs_info[i].start_tm = time(NULL);
        fs_info[i].prec = 0;
        fs_info[i].wipe_failed = (  (fd_wipe = open(argv[i], O_RDWR)) == -1
                                    && errno == EACCES && flags & FORCE_OPEN
                                    && (chmod(argv[i], S_IRUSR | S_IWUSR) == -1 || (fd_wipe = open(argv[i], O_RDWR)) == -1)
                                    || fd_wipe == -1
                                    || !wipe_file(fd_wipe));
        fs_info[i].error = errno;
        fs_info[i].end_time = time(NULL);

        one_failed = (fs_info[i].wipe_failed)? true : one_failed;
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
            return FAILURE_AND_RETRY;
        }

        free(fs_info);
        return FAILURE_AND_EXIT;
    }

    free(fs_info);
    return SUCCESS;

}

int main (int argc, char **argv) {
    time_t start_tm = time(NULL);

    set_options(argc, argv);

    
    int fin_status;
    while((fin_status = wipe_files(argc, argv)) == FAILURE_AND_RETRY);

    FAIL_IF(fd_source != -1 && close(fd_source) == -1, "Can't close src file!");

    
    fprintf(stdout, "%s", (fin_status == SUCCESS)? "Program finished with no errors!" : "Program finished with errors!");

    time_t diff_tm = start_tm - time(NULL);
    lt = localtime(&diff_tm);
    fprintf(stdout, "Total time elapsed: %d days|%d hrs|%d mins|%s secs\n", lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);


    return 0;
}