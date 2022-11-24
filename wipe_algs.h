#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


static size_t *bytes_wiped;
static bool *has_passes;
static size_t *curr_pass;
static size_t *max_pass;


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
