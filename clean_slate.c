#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <sys/vfs.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "dir_traversal.h"

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


#define DEF_RW_BUF_SIZE (1 << 16)

typedef enum w_method {
    W_NONE,
    W_ZEROS,      // -z    
    W_ONES,       // -o
    W_PSEUDO,     // -p
    W_GOST,       // -r
    W_AIRFORCE,   // -c
    W_ARMY,       // -a
    W_HMG,        // -b
    W_DOD,        // -d
    W_PFITZNER,   // -n
    W_GUTMANN,    // -g
    W_SOURCE,     // -s
} w_method;

typedef struct Options {
    w_method method;
    bool delete_after;  // -e
    bool displ_info;    // -i
    bool force_open;    // -f
    int first;          // -t (0 if flag is not set)
} Options;

Options options;


// index of the first file path in argv
size_t i_first_fp;

// buffer for representing progress bar and/or num of passes
char *prog_buf = NULL;


const struct option long_opts[] = {
    {"source", required_argument, NULL, 's'}, 
    {"help"  , no_argument      , NULL, 'h'},
    {"first" , required_argument, NULL, 't'}
};
const char short_opts[] = "zoprcabdngs:heift:";

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

#define MAX_RENAME_TRIES (100)

bool wipe_failure(const char *msg) {
    fprintf(stdout, "%s\n", msg);
    return false;
}

bool wipe_name(char *old_path, const bool is_dir) {
    char *new_path = malloc(strlen(old_path) + 1); 
    memcpy(new_path, old_path, strlen(old_path) + 1);

    char *base = basename(new_path);
    char *base_p = new_path + strlen(new_path) - strlen(base);
    
    const int dir_fd = open(dirname(old_path), O_RDONLY | O_DIRECTORY | O_SYNC);

    size_t len = strlen(base) + 1;
    while (len--)
    {   
        base[len] = '\0';

        bool len_fail = true;
        size_t tries = MAX_RENAME_TRIES;
        // if each and every try of finding a new file name fails, skip this file length
        while(tries--)
        {
            // randomize bytes of the base
            size_t i = 0;                                           
            while ((base[i] = rand()) == '\0' || base[i] == '/' 
                   || ++i < len);

            // check if new name doesnt exists
            strcpy(base_p, base);
            if (access(new_path, F_OK)) 
            {
                len_fail = false;
                break;
            }
        }

        if (len_fail)
            continue;

        if (rename(old_path, new_path)) 
            return wipe_failure("Can't rename file. Deletion failed!");

        strcpy(old_path, new_path);
    }

    if(is_dir && rmdir(new_path)
       || !is_dir && unlink(new_path)) 
        return wipe_failure("Can't delete file!");
    
    if(dir_fd != -1 && close(dir_fd))
        return wipe_failure("Can't close directory file descriptor!");
}

bool wipe_meta(const char *path) {

}

bool wipe_data(const char *path) {

}

bool wipe_dir(char *path) {
    //if (options.delete_after)
        wipe_name(path, true);
}

// wipe non dir file
bool wipe_non_dir(const char *path) {
    fprintf(stdout, "Wiping file '%s'\n", path);

    // if opening fd fails and force_open is set, try to change it's access and open it again
    int fd;
    int flags = O_RDWR | __O_DIRECT | O_SYNC;
    if ((fd = open(path, flags)) == -1 
        && errno == EACCES && options.force_open
        && (chmod(path, S_IRUSR | S_IWUSR) == -1 || (fd = open(path, flags)) == -1)
        || fd == -1)
        return wipe_failure("Can't open file!");

    // give up if the partition the file is on doesn't use ext4
    struct statfs fs_stats;                           //EXT4_SUPER_MAGIC
    if (!fstatfs(fd, &fs_stats) && fs_stats.f_type != 0xef53) 
        return wipe_failure("File is on the wrong file system. File system must be ext4!");

    // get the size of the space allocated for the file's cont
    struct stat st;
    if (fstat(fd, &st) == -1) 
        return wipe_failure("Couldn't get file size!");

    // socket or fifo files can't be overwriten
    if (S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode)) 
        return wipe_failure("File is eather a FIFO(named pipe) or socket!");

    // try and restrict acces to file from all other processes
    flock(fd, LOCK_EX | LOCK_NB);

    
    // first wipe file contents
    // wipe file inode

    // if requested by user (by setting delete after flag) wipe file name
    if (options.delete_after)
        wipe_name(path, false);

    if (close(fd))
        return wipe_failure("Can't close file descriptor!");

}

void wipe_file(const char *path) {
    struct stat st;
    stat(path, &st);

    if(S_ISDIR(st.st_mode))
        traverse_dir(path, wipe_dir, wipe_non_dir);
    else
        wipe_non_dir(path);
}


#define PRECENTAGE(COUNT, MAX) ((100.0) - ((((MAX) - (COUNT)) * (100.0)) / (MAX)))

void prit_prog_buf(const size_t count_pass, const size_t max_pass, 
                   const size_t count_prog, const size_t max_prog) {
    
    if (!prog_buf)
    {
        const char passes_pref[]    = "Pass: (00/00)\n";

        const char prog_bar_pref[]  = "Progress: [";
        const char prog_bar_suf[]   = "] 000.000000%%";
        
        const int len_prog_bar_pref = strlen(prog_bar_pref);
        const int len_prog_bar_suf  = strlen(prog_bar_suf);
        const int len_passes_pref   = strlen(passes_pref);

        const int len_prog_buf = len_passes_pref 
                                + len_prog_bar_pref 
                                + 100 
                                + len_prog_bar_suf 
                                + 1;

        prog_buf = malloc(len_prog_buf);
        
        char *tmp = prog_buf;
        memcpy(tmp, passes_pref, len_passes_pref);

        tmp += len_passes_pref;
        memcpy(tmp, prog_bar_pref, len_prog_bar_pref);

        tmp += len_prog_bar_pref + 100;
        memcpy(tmp, prog_bar_suf, len_prog_bar_suf);

        prog_buf[len_prog_buf - 1] = '\0';
    }

    if (max_pass) 
    {
        char bf[] = "00";

        sprintf(bf, "%s%ld", (count_pass > 9)? "":"0", count_pass);
        memcpy(strchr(prog_buf, '(') + 1, bf, 2);

        sprintf(bf, "%s%ld", (max_pass > 9)? "":"0", max_pass);
        memcpy(strchr(prog_buf, '/') + 1, bf, 2);
    }
    
    float prec = PRECENTAGE(count_prog, max_prog);

    char *prog_bar_start = strchr(prog_buf, '[') + 1;
    for(size_t i = 0; i < 100; i++) 
    {
        prog_bar_start[i] = (i < (int)prec)? '#':'.';
    }

    char *prog_bar_end = strchr(prog_buf, ']');
    char bf[] = "000.000000%%"; 
    sprintf(bf, "%s%s%.6f%%",  (prec < 100)? " " : ""
                               , (prec < 10)? " " : ""
                               , prec);
    memcpy(prog_bar_end + 2, bf, strlen(bf) + 1);


    fprintf(stdout, "%s\n", (max_pass)? prog_buf : strchr(prog_buf, '\n') + 1);
}

void set_options (const int argc, char * const argv[]) {
    options = (Options) {
        W_NONE,     // method
        false,      // delete_after
        false,      // displ_info
        false,      // force_open
        0,          // first
    };

    int opt;
    while((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            // wipe methods
            case 'z':   options.method = W_ZEROS;          break;
            case 'o':   options.method = W_ONES;           break;
            case 'p':   options.method = W_PSEUDO;         break;
            case 'r':   options.method = W_GOST;           break;
            case 'c':   options.method = W_AIRFORCE;       break;
            case 'a':   options.method = W_ARMY;           break;
            case 'b':   options.method = W_HMG;            break;
            case 'd':   options.method = W_DOD;            break;
            case 'n':   options.method = W_PFITZNER;       break;
            case 'g':   options.method = W_GUTMANN;        break;
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

    if (options.method == W_NONE) options.method = W_ZEROS;

    FAIL_IF((i_first_fp = optind) == argc, "Mising file operand!"); 
}


int main(int argc, char *argv[]) {
    wipe_file("a/b/c");
    wipe_file("a/b");
    exit(1);
    time_t start_tm = time(NULL);
    
    srand(time(NULL));
    
    set_options(argc, argv);
    
    
    while(i_first_fp < argc)
        wipe_file(argv[i_first_fp++]);


    free(prog_buf);

    fprintf(stdout, "%s: finished!\n", PROG_NAME);
    
    time_t d, h, m , s = time(NULL) - start_tm;
    s -= (d = s / 86400) * 86400;
    s -= (h = s / 3600) * 3600;
    s -= (m = s / 60) * 60;   
    fprintf(stdout, "Time elapsed: %ldd : %ldh : %ldm : %lds\n", d, h, m, s);

    return 0;
}