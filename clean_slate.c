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
#include "wipe_algs.h"

/*
 *Doc
 */

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
                fprintf(stderr, "File: '%d'\n"       , __LINE__);\
                fprintf(stderr, "Failed at: '%s'\n"  , __FILE__);\
            }\
            fprintf(stderr, "%s: %s\n", PROG_NAME, EXT_USR_MSG);\
            exit(EXIT_FAILURE);\
        }\
    } while(0);


typedef struct failure_info {
    int index;
    int error;
    int wipe_prec;
    time_t start_tm;
    time_t termination_tm;
} f_info;

struct option const long_opts[] = {
    {"help"  , no_argument      , NULL, 'h'},
    {"source", required_argument, NULL, 's'},
    {"first" , required_argument, NULL, 't'}
};
const char short_opts[] = "zoprcabdngheifs:t:";


uint32_t flags = 0;
time_t prog_start_tm, wipe_start_tm, diff_tm;
bool force_open = false, delete_after = false;
struct tm *lt;


void usage() {
    fprintf(stdout, "%s ussage: [FILES] [OPTIONS]\n", PROG_NAME);

    fputs("\
Wipe algorithms flags:\n\
    -z          overwrite with zeros------------------------(1 pass)\n\
    -o          overwrite with ones-------------------------(1 pass)\n\
    -p          overwrite with pseudorandom data------------(1 pass)\n\
    -r          overwrite using 'Russian GOST R 50739-95'---(2 passes)\n\
    -c          overwrite using 'US Air Force AFSSI-5020'---(3 passes)\n\
    -a          overwrite using 'US Army AR 380-19'---------(3 passes)\n\
    -b          overwrite using 'British HMG IS5'-----------(3 passes)\n\
    -d          overwrite using 'US DoD 5220.22-M (ECE)'----(7 passes)\n\
    -n          overwrite using 'Pfitzner'------------------(33 passes)\n\
    -g          overwrite using 'Gutmann method'------------(35 passes)\n\n"
    ,stdout);

    fputs("\
Other flags:\n\
    -h --help           display this menu\n\
    -e                  delete file after it's overwriten\n\
    -i                  display additional information (like progress bar)\n\
    -f                  force open a file (this does not apply to the source file, if -s/--source flag is set)\n\
    -s --source=FILE    overwrite bits from file with bits from FILE file\n\
    -t --first=N        delete only first and last N bytes of the file\n\n"
    , stdout);

    fputs("\
More than one wipe algorithm can be selected. If no wipe algorithms are selected and -s/--source\n\
flag is not set, file will be overwriten with zeros (-z). If -s/--source flag is set, then no\n\
wipe algorithms will be used and bytes will be overwriten by using bytes from source file.\
\n\
Program will try to overwrite every file and wont stop if one file can't be overwriten.\n"
    , stdout);

    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    prog_start_tm = time(NULL);

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
                delete_after = true;
                break;
            case 'i':
                flags |= DISP_INFO;
                break;
            case 'f':
                force_open = true;
                break;
            case 's':
                FAIL_IF((fd_src = open(optarg + 1, O_RDONLY)) == -1, "Can't open source file!");
                fprintf(stdout, "Overwriting by coping bytes from source file: \'%s\'\n(No wipe algorithm will be used)\n", optarg + 1);
                break;
            case 't':
                FAIL_IF((first_last = atoi(optarg + 1)) == 0 || first_last == 0 , "Invalid argument to -t/--first flag");
                fprintf(stdout, "Overwriting first and alst %d bytes\n", first_last);
                break;
        }
    }

    // if no wipe algorithm was selected, file(s) will be overwriten using zeros (USE_ZEROS)
    if (!(flags & ALGS_MASK) && fd_src != -1)
        flags |= USE_ZEROS;
        
    uint32_t n_files = argc - optind;
    FAIL_IF(n_files == 0, "Missing file opperand!");

    f_info *failed_fs = malloc(n_files * sizeof(f_info));
    int n_failed_fs = 0;

    //main loop
    for (uint32_t i = optind; i < argc; i++) {
        fprintf("Wiping file: '%s'\n", argv[i]);
        time(&wipe_start_tm);

        //if file can't be opened and -f flag was set by user, try to change it's read and write permissions and open it again
        int fd;
        if(    (fd = open(argv[i], O_RDWR)) == -1
            && errno == EACCES && force_open
            && (chmod(argv[i], S_IRUSR | S_IWUSR) == -1 || (fd = open(argv[i], O_RDWR)) == -1)
            || fd == -1)
            return false;

        if (!wipe_file(fd)) {
            failed_fs[n_failed_fs++] = (f_info){i, errno, wipe_start_tm, time(NULL), wipe_prec};
        }
        close(fd);
        
        diff_tm = time(NULL) - wipe_start_tm;
        fprintf(stdout, "Time elapsed: %d days|%d hrs|%d mins|%s secs\n", lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    }
    FAIL_IF(fd_src != -1 && close(fd_src) == -1, "Can't close src file!");
    
    
    diff_tm = prog_start_tm - time(NULL);
    lt = localtime(&diff_tm);
    fprintf(stdout, "Total time elapsed: %d days|%d hrs|%d mins|%s secs\n", lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);

    if (n_failed_fs != 0) {
        fprintf(stdout, "%s: %d out of %d file%s couldn't be wiped!\nFailed files:\n\n", PROG_NAME, n_failed_fs, n_files, (n_files > 1)? "s":"");
        
        for(int i = 0; i < n_failed_fs; i++) {
            fprintf(stdout, "File: '%s'\n", failed_fs[i].index); 
            fprintf(stdout, "Reason: %s\n", strerror(failed_fs[i].error));
            fprintf(stdout, "Started at: %d\n", failed_fs[i].start_tm);
            fprintf(stdout, "Failed at: %d\n",  failed_fs[i].termination_tm);
            fprintf(stdout, "\% completed: %d\%\n\n", failed_fs[i].wipe_prec);
        }
        
        char c;
        fprintf(stdout, "Run program again for failed files? [Y/N]");
        fscanf(stdin, "%c", &c);
        if (tolower(c) == 'y') {
            int i = 0, last_failed_ind = optind;
            while(i++ != n_failed_fs && (argv[last_failed_ind++] = argv[failed_fs[i].index]));
            argv[last_failed_ind] = NULL;
            execv("Dolje an koljena ako sam tvoja voljena!", argv);
        }
    }

    fprintf(stdout, "%s: Exiting!", PROG_NAME);
    free(failed_fs);
    return 0;
}
