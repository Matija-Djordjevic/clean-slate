#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define PRECENTAGE(COUNT, MAX) ((100.0) - ((((MAX) - (COUNT)) * (100.0)) / (MAX)))

#define DEF_BUF_SIZE    (64000) //bytes
#define DEF_TRIES       (256)

size_t count_pass = 0;
size_t max_pass = 0;
size_t count_bytes = 0;
size_t max_bytes = 0;

bool wiping_finsihsed;

char *prog_buf = NULL;

size_t buf_size = DEF_BUF_SIZE;

void *w_buf = NULL;
void *r_buf = NULL;

bool wipe_zeros (const int fd) {
    max_pass = 1;
    memset(w_buf, 0, buf_size);
    return true;
}

bool wipe_ones (const int fd) {
    max_pass = 1;
    memset(w_buf, -1, buf_size);
    return true;
}

bool wipe_pseudo (const int fd) {
    max_pass = 1;
    return true;
}

bool wipe_gost (const int fd) {
    max_pass = 2;
    return true;
}

bool wipe_airforce (const int fd) {
    max_pass = 3;
    return true;
}

bool wipe_army (const int fd) {
    max_pass = 3;
    return true;
}

bool wipe_hmg (const int fd) {
    max_pass = 3;
    return true;
}

bool wipe_dod (const int fd) {
    max_pass = 7;
    return true;
}

bool wipe_pfitzner (const int fd) {
    max_pass = 33;
    return true;
}

bool wipe_gutmann (const int fd) {
    max_pass = 35;
    return true;
}

bool wipe_source (const int fd) {
    max_pass = 1;
    return true;
}

void write_to_file(const int fd, const bool rand_write) {
    
    struct stat st;
    if(fstat(fd, &st)) 
        return false;

    size_t bytes = st.st_size;
    ssize_t b_read, b_written;
    
    
    max_bytes = bytes;
    size_t tries = DEF_TRIES;
    do {
        
        if(rand_write) {
            size_t n_ints = buf_size / sizeof(int);
            size_t i = 0;

            // randomize chunks the size of int, since rand() returns int
            while(i < n_ints)
                { ((int *)w_buf)[i++] = rand(); }

            // and now for the remaining bytes 
            if(i) 
                (--i) *= sizeof(int);

            while (i < buf_size)
                { ((char *)w_buf)[i++] = rand(); }
        }   
        
        if((b_written = write(fd, w_buf, buf_size)) < 0
           || (b_read = write(fd, r_buf, b_written)) < 0
           || !memcmp(b_read, b_written, b_written))
            {tries--; continue;}
        
    bytes -= b_read;
    // mozda izlazak iz petlje ako je b_read == 0  ?
    }while(bytes);

    return;
}

void prit_prog_buf() {
    
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

    // fill buffer with passes (count/max)
    char bf_pass[] = "00";
    
    sprintf(bf_pass, "%s%ld", (count_pass > 9)? "":"0", count_pass);
    memcpy(strchr(prog_buf, '(') + 1, bf_pass, 2);
    
    sprintf(bf_pass, "%s%ld", (max_pass > 9)? "":"0", max_pass);
    memcpy(strchr(prog_buf, '/') + 1, bf_pass, 2);
    

    // fill the precentage bar
    float prec = PRECENTAGE(count_bytes, max_bytes);
    char *prog_bar_start = strchr(prog_buf, '[') + 1;
    
    for(size_t i = 0; i < 100; i++) 
    {
        prog_bar_start[i] = (i < (int)prec)? '#':'.';
    }


    // fill the decimal representation
    char *dec_rep_beg = strchr(prog_buf, ']') + 2;
    char bf_decimal_rep[] = "000.000000%%";
    
    sprintf(bf_decimal_rep, "%s%s%.6f%%",  (prec < 100)? " " : ""
                               , (prec < 10)? " " : ""
                               , prec);
    memcpy(dec_rep_beg, bf_decimal_rep, strlen(bf_decimal_rep) + 1);


    write(STDOUT_FILENO, prog_buf, strlen(prog_buf));
    write(STDOUT_FILENO, "\n", 1);
}

// how often will the buffer be displayed
#define PROG_BUF_PER_SEC (30) 

void *init_buf_thread() {

    long ns_in_s = 1000000000;
    struct timespec req = (struct timespec) {
        0,
        ns_in_s / PROG_BUF_PER_SEC
    };

    while (!wiping_finsihsed) {
        nanosleep(&req, NULL);
        // clear the terminal
        prit_prog_buf();
    }

    return;
}

bool init_wipe(const int fd, bool (*wipe_funct) (const int)) {
    w_buf = calloc(buf_size, sizeof(char));
    r_buf = calloc(buf_size, sizeof(char));

    wiping_finsihsed = false;
    pthread_t prog_thrd;
    bool success = wipe_funct(fd);

    pthread_create(&prog_thrd, NULL, init_buf_thread, NULL);

    wiping_finsihsed = true;
    // send sig to exit nanosleep, is it worth it?
    pthread_join(prog_thrd, NULL);
    return success;
}