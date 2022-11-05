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


typedef struct fail_info {
    int index;
    int error;
    int prec;
    time_t start_tm;
    time_t termination_tm;
} fail_info;

static int fd_source = -1;
static int fd_wipe;
static int first_last = 0;
static int file_size_bytes;
static int wipe_prec;
static int i_first_file; //index of first file path in argv after getopt_long() modifies it
static time_t start_tm, wipe_start_tm, diff_tm;
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
    i_first_file = optind;

}

int wipe_files (const int argc, char *argv[]) {

    // see how much files we have to wipe
    uint32_t n_files = i_first_file;
    while(argv[++n_files] != NULL);

    // for gathering info on failed files
    fail_info *failed_fs = malloc(n_files * sizeof(fail_info));
    int n_failed_fs = 0;


    for (int i = i_first_file; argv[i] != NULL; i++) {
        fprintf("Wiping file: '%s'\n", argv[i]);
        
        wipe_start_tm = time(NULL);
        wipe_prec = 0;

        // if the file can't be opened, try to change it's read and write permissions and open it again
        if((fd_wipe = open(argv[i], O_RDWR)) == -1
            && errno == EACCES && flags & FORCE_OPEN
            && (chmod(argv[i], S_IRUSR | S_IWUSR) == -1 || (fd_wipe = open(argv[i], O_RDWR)) == -1)
            || fd_wipe == -1
            // if we get fd than we can try and wipe. Wipe file takes care of fd_wipe disposal
            || !wipe_file(fd_wipe))
            failed_fs[n_failed_fs++] = (fail_info){i, errno, wipe_start_tm, time(NULL), wipe_prec}; 
        
        diff_tm = time(NULL) - wipe_start_tm;
        lt = localtime(&diff_tm);
        fprintf(stdout, "Time elapsed: %d days|%d hrs|%d mins|%s secs\n", lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    }

    diff_tm = start_tm - time(NULL);
    lt = localtime(&diff_tm);
    fprintf(stdout, "Total time elapsed: %d days|%d hrs|%d mins|%s secs\n", lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

    // if existing, display info about failed files and prompt user to run the program agiain with only failed files and same flags
    if (n_failed_fs != 0) {
        fprintf(stdout, "%s: Failed to wipe %d out of %d file%s!\nFailed files info:\n\n"
                , PROG_NAME, n_failed_fs, n_files, (n_files > 1)? "s":"");
        
        for(int i = 0; i < n_failed_fs; i++) {
            fprintf(stdout, "File: '%s'\n",         failed_fs[i].index); 
            fprintf(stdout, "Reason: %s\n",         strerror(failed_fs[i].error));
            fprintf(stdout, "Started at: %d\n",     failed_fs[i].start_tm);
            fprintf(stdout, "Failed at: %d\n",      failed_fs[i].termination_tm);
            fprintf(stdout, "Completed: %d\%\n\n",  failed_fs[i].prec);
        }
        
        fprintf(stdout, "Run program again for failed files? [Y/N]");

        char c;
        fscanf(stdin, "%c", &c);
        if (tolower(c) == 'y') {
            /* 
             * getopt_long modified argv so that flags come first than file paths
             * change argv again by putting failed file paths right after the flags, terminate it wihth NULL 
             * after the last failed file path and pass it to execv
             */             
            int i = 0, last_failed_ind = i_first_file;
            while(i++ != n_failed_fs && (argv[last_failed_ind++] = argv[failed_fs[i].index]));
            argv[last_failed_ind] = NULL;
            
            free(failed_fs);
            return FAILURE_AND_RETRY;
        }

        free(failed_fs);
        return FAILURE_AND_EXIT;
    }

    free(failed_fs);
    return SUCCESS;

}

int main (int argc, char **argv) {
    start_tm = time(NULL);
    srand(time(start_tm));

    set_options(argc, argv);

    int fin_status;
    while((fin_status = wipe_files(argc, argv)) == FAILURE_AND_RETRY);

    fprintf(stdout, "%s", (fin_status == SUCCESS)? "Program finished with no errors!" : "Program finished with errors!");

    FAIL_IF(fd_source != -1 && close(fd_source) == -1, "Can't close src file!");
    return 0;
}