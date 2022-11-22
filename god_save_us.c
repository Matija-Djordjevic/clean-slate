#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

typedef struct dir_info {
    bool empty;
    size_t depth;
    char *d_name;
} dir_info;

typedef struct stack
{
    size_t capacity;
    size_t len;
    dir_info *array;
} stack;

void free_dir_info (dir_info **info){
    if (!(*info))
        return

    free((*info)->d_name);
    free((*info));
    *info = NULL;
}

dir_info *new_dir_info(const bool empty, 
                       const size_t depth, 
                       const char *d_name) {

    dir_info *ret = malloc(sizeof(dir_info));

    ret->empty = empty;
    ret->depth = depth;    
    ret->d_name = strdup(d_name);
    
    return ret;
}

stack *new_stack(){
    stack *ret = malloc(sizeof(stack));

    ret->len = 0;
    ret->capacity = 1;
    ret->array = malloc(sizeof(dir_info));

    return ret;
}

void dir_info_cpy(dir_info *dst, dir_info *src) {
    dst->empty = src->empty;
    dst->depth = src->depth;
    
    dst->d_name = realloc(dst->d_name, strlen(src->d_name) + 1);
    strcpy(dst->d_name, src->d_name);
}

void push_stack(stack *st, dir_info *src) {
    if (st->len == st->capacity)  
        st->array = realloc(st->array, (st->capacity *= 2) * sizeof(dir_info));

    dir_info_cpy(&st->array[st->len++], src);
}

bool empty_stack(stack *st) {
    return (st->len == 0);
}

int stack_last(stack *st, dir_info *buf) {
    if (empty_stack(st))
        return -1;

    dir_info_cpy(buf, &st->array[st->len - 1]);
}

int pop_stack(stack *st, dir_info *buf) {
    if (empty_stack(st))
        return -1;

    stack_last(st, buf);
    if (st->array[--st->len].d_name)
    {
        free(st->array[st->len].d_name);
        st->array[st->len].d_name = NULL;
    }
}

bool free_stack(stack *st) {
    for (size_t i = 0; i < st->len; i++)
    {
        free(st->array[i].d_name);
    }
    st->capacity = st->len = 0;
}

void print_stack (stack *st) {
    printf("Stack:\n");
    for (size_t i = 0; i < st->len; i++)
    {
        printf("%s %ld %s\n", (st->array[i].empty)? "true" : "false", st->array[i].depth,  st->array[i].d_name);
    }
    printf("\n");
}


void wipe_name(const char *name) {return;}
void wipe_file(const char *path) {return;}

bool this(const char *path) {
    dir_info *cur_di = new_dir_info(false, 1, path);

    stack *st = new_stack();
    push_stack(st, cur_di);

    
    size_t cur_depth = 0;
    while(!empty_stack(st)) 
    {
        
        // print_stack(st);
        pop_stack(st, cur_di);
        printf("%ld %ld %s\n", cur_depth, cur_di->depth, cur_di->d_name);
        if (cur_depth == cur_di->depth)
        {
            chdir("..");
            cur_depth--;
        }


        
        DIR *dir_fd = opendir(cur_di->d_name);
        if(!dir_fd) 
            return false;
        
        if(cur_di->empty)
        {
            printf("wipe->%s\n", cur_di->d_name);
            wipe_name(cur_di->d_name);
            continue;
        }

        cur_di->empty = true;
        push_stack(st, cur_di);

        chdir(cur_di->d_name);
        cur_depth++;
        struct dirent *entry;
        while(entry = readdir(dir_fd))
        {
            
            char *en_name = strdup(entry->d_name);
            if (strcmp(en_name, ".") && strcmp(en_name, "..")) {
                struct stat f_st;
                if (lstat(en_name, &f_st)) 
                    return false;

                if (S_ISDIR(f_st.st_mode))
                {
                    free(cur_di->d_name);
                    cur_di->d_name = strdup(en_name);
                    cur_di->depth = cur_depth + 1; 
                    cur_di->empty = false;
                    push_stack(st, cur_di);
                } 
                else 
                {
                    wipe_file(cur_di->d_name);
                }
            }
            free(en_name);
        }
        closedir(dir_fd);
    }

    return true;
}

int main() {
 
    printf("%d", this("neki2"));
    exit(1);
}