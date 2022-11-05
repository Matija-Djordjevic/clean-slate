#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>


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
#define ALGS_MASK       (1111111111)

#define DISP_INFO       (1<<10)

int first_last = -1;
int fd_src = -1;
int wipe_prec = 0;
uint32_t flags;

bool wipe_source    (const int fd);
bool wipe_zeros     (const int fd);
bool wipe_ones      (const int fd);
bool wipe_pseudo    (const int fd);
bool wipe_gost      (const int fd);
bool wipe_airforce  (const int fd);
bool wipe_army      (const int fd);
bool wipe_hmg       (const int fd);
bool wipe_dod       (const int fd);
bool wipe_pfitzner  (const int fd);
bool wipe_gutmann   (const int fd);

bool wipe_file(const int fd);


bool wipe_file(const int fd) {
    wipe_prec = 0;
    srand(time(NULL));

    //get space allocated for faile in bytes
    struct stat sb;
    if (fstat(fd, &sb) == -1) 
        return false;
    uint32_t fs_bytes = sb.st_blocks * 512;

    //FIFO and socket files can't be wiped
    if (S_ISSOCK(sb.st_mode) || S_ISFIFO(sb.st_mode)) 
        return false;

    return !(   fd_src != -1         && !wipe_source(fd)
             || flags & USE_ZEROS    && !wipe_zeros(fd)
             || flags & USE_ONES     && !wipe_ones(fd)
             || flags & USE_PSEUDO   && !wipe_pseudo(fd)
             || flags & USE_GOST     && !wipe_gost(fd)
             || flags & USE_AIRFORCE && !wipe_airforce(fd)
             || flags & USE_ARMY     && !wipe_army(fd)
             || flags & USE_HMG      && !wipe_hmg(fd)
             || flags & USE_DOD      && !wipe_dod(fd)
             || flags & USE_PFITZNER && !wipe_pfitzner(fd)
             || flags & USE_GUTMANN  && !wipe_gutmann(fd));
}

bool wipe_source (const int fd) {

    return true;
}

bool wipe_zeros (const int fd) { 
    
    return true;
}

bool wipe_ones (const int fd) { 
    
    return true;
}

bool wipe_pseudo (const int fd) { 
    
    return true;
}

bool wipe_gost (const int fd) { 
    
    return true;
}

bool wipe_airforce (const int fd) { 
 
    return true;   
}

bool wipe_army (const int fd) { 
    
    return true;
}

bool wipe_hmg (const int fd) { 
    
    return true;
}

bool wipe_dod (const int fd) { 
    
    return true;
}

bool wipe_pfitzner (const int fd) { 
    
    return true;
}

bool wipe_gutmann (const int fd) { 
    
    return true;
}

