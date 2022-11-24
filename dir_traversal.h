#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

#define NEW_STACK_CAP(OLD_CAP) ((OLD_CAP) * 2)
    
typedef struct d_info 
{
    char *name;
    bool empty;
    size_t depth;

} d_info;

typedef struct d_stack
{
    size_t capacity;
    size_t len;  // first free slot
    d_info *mem_block;

} d_stack;

static void d_info_print(const d_info *info){
    printf("'%s' %s %ld\n", info->name, 
                        (info->empty)? "TRUE":"FALSE", 
                        info->depth);
}

d_info *d_info_new(const char *name, 
                   const bool empty, 
                   const size_t depth) {
    d_info *ret = malloc(sizeof(d_info));

    //tf?????????
    if(name) {
        ret->name = strdup(name);
    } else {
        ret->name = NULL;
    }

    ret->empty = empty;
    ret->depth = depth;

    return ret;
}

static void d_info_free_ext_mem(d_info *info){
    if(info->name)
        free(info->name);

    info->name = NULL;
}

static void d_info_free(d_info **info) {
    free((*info)->name);
    (*info)->name = NULL;

    free(*info);
    *info = NULL;
}

static d_info *d_info_cpy(d_info *dst, const d_info *src) {
    dst->name = realloc(dst->name, strlen(src->name) + 1);
    strcpy(dst->name, src->name);

    dst->empty = src->empty;
    dst->depth = src->depth;

    return dst;
}


static d_stack *d_stk_new() {
    d_stack *ret = malloc(sizeof(d_stack));

    ret->capacity = 1;
    ret->len = 0;
    ret->mem_block = calloc(0, sizeof(d_info));

    return ret;
} 

static bool d_stk_is_empty(const d_stack *st) {
    return !st->len;
} 

static d_info *d_stk_top(const d_stack *st, d_info *buf) {
    if (d_stk_is_empty(st)) 
        return NULL;

    d_info *elm_p = &st->mem_block[st->len - 1];

    return d_info_cpy(buf, elm_p);
} 

static void d_stk_push(d_stack *st, d_info *buf) {
    if (st->len == st->capacity) 
    { 
        st->capacity = NEW_STACK_CAP(st->capacity);
        st->mem_block = realloc(st->mem_block, st->capacity * sizeof(d_info));
        memset(st->mem_block + st->len, 
               0, 
               sizeof(d_info) * (st->capacity - st->len));
    }

    
    d_info *elm_p = &st->mem_block[st->len++];

    d_info_cpy(elm_p, buf);
} 

static d_info *d_stk_pop(d_stack *st, d_info *buf) {
    if (d_stk_is_empty(st)) 
        return NULL;

    d_info *ret = d_stk_top(st, buf);

    st->len--;

    return ret;
} 

static void d_stk_clear(d_stack *st) {
    st->len = 0;
} 

static void d_stk_free(d_stack **st) {
    for(size_t i = 0; i < (*st)->capacity; i++)
    {
        d_info_free_ext_mem(&(*st)->mem_block[i]);
    }

    free((*st)->mem_block);
    *st = NULL;
}

static void d_stk_shrink_to_fill(d_stack *st) {
    for(size_t i = st->len; i < st->capacity; i++)
    {
        d_info_free_ext_mem(&st->mem_block[i]);
    }

    st->capacity = st->len;
    st->mem_block = realloc(st->mem_block, st->capacity * sizeof(d_info));
}

static void d_stk_print(const d_stack *st) {
    printf("d_stack:\n");
    if(!st || st->len == 0)
    {
        printf("None\n");
        return;
    }
    for(size_t i = 0; i < st->len; i++) 
    {
        d_info_print(&st->mem_block[i]);
    }
}

#define NOT_IN_PAR_DIR(DEPTH, CUR_DIR_DEPTH) ((DEPTH) == (CUR_DIR_DEPTH))
char WORKINGDIR[PATH_MAX];
int traverse_dir(const char *path, 
             bool (* handle_dir) (char *), 
             bool (* handle_file) (char *)){
    
    char init_wd[PATH_MAX];
    getcwd(init_wd, sizeof(init_wd));


    // go to the parent dir of the given one
    chdir(path);
    chdir("..");

    d_info *curr = d_info_new(basename(path), //path relative to the parent path
                              false, 
                              1);
    d_stack *st = d_stk_new();
    d_stk_push(st, curr);


    size_t depth = 0;
    getcwd(WORKINGDIR, PATH_MAX);
    char *a = WORKINGDIR;
    while(!d_stk_is_empty(st))
    {
        d_stk_pop(st, curr);

        if(NOT_IN_PAR_DIR(depth, curr->depth))
        {
            chdir("..");
            depth--;
        }
        getcwd(WORKINGDIR, PATH_MAX);
        a = WORKINGDIR;
        if (curr->empty)
        {
            handle_dir(curr->name);
            continue;
        }
        DIR *dir_fd = opendir(curr->name);
        if(!dir_fd) {
            printf("%s\n", strerror(errno));
            continue;
        }
        
        curr->empty = true;
        d_stk_push(st, curr);
        chdir(curr->name);
        depth++;

        struct dirent *entry;
        while(entry = readdir(dir_fd))
        {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            {
                struct stat f_st;
                int b = 2;
                if (lstat(entry->d_name, &f_st)) 
                    continue;

                int a = S_ISDIR(f_st.st_mode);
                if (S_ISDIR(f_st.st_mode))
                {
                    d_info_free(&curr);
                    curr = d_info_new(entry->d_name, 
                                      false, 
                                      depth + 1);
                    d_stk_push(st, curr);
                    continue;
                }
                handle_file(entry->d_name);
            }
        }
        closedir(dir_fd);
    }

    d_info_free(&curr);
    d_stk_free(&st);
    chdir(init_wd);
}
