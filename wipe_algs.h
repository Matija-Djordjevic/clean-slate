#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PRECENTAGE(COUNT, MAX) ((100.0) - ((((MAX) - (COUNT)) * (100.0)) / (MAX)))

static size_t *bytes_wiped;
static bool *has_passes;
static size_t *curr_pass;
static size_t *max_pass;


bool wipe_none (const int fd) {
    *max_pass = 1;
    return true;
}

bool wipe_zeros (const int fd) {
    *max_pass = 1;
    return true;
}

bool wipe_ones (const int fd) {
    *max_pass = 1;
    return true;
}

bool wipe_pseudo (const int fd) {
    *max_pass = 1;
    return true;
}

bool wipe_gost (const int fd) {
    *max_pass = 2;
    return true;
}

bool wipe_airforce (const int fd) {
    *max_pass = 3;
    return true;
}

bool wipe_army (const int fd) {
    *max_pass = 3;
    return true;
}

bool wipe_hmg (const int fd) {
    *max_pass = 3;
    return true;
}

bool wipe_dod (const int fd) {
    *max_pass = 7;
    return true;
}

bool wipe_pfitzner (const int fd) {
    *max_pass = 33;
    return true;
}

bool wipe_gutmann (const int fd) {
    *max_pass = 35;
    return true;
}

bool wipe_source (const int fd) {
    *max_pass = 1;
    return true;
}

// proto
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


bool init_wipe(const int fd, bool (*wipe_funct) (const int)) {
    *bytes_wiped = 0;
    // fork the program
    // if parrent
        bool success = wipe_funct(fd);
    // if child
        // start a buffer that reads 
    // close child
    // semafori
}