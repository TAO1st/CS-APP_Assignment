/* 
 * This file is a cache simulator that takes a valgrind memory trace as input, 
 * simulates the hit/miss behavior of a cache memory on this trace, and outputs 
 * the total number of hits, misses, and evictions.
 * 
 * @author Li Li
 * @E-mail Lil147@pitt.edu
 * @date April 17, 2020
 */

#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <math.h>
#include <string.h>

#define m 64 // the address field specifies a 64-bit hex memory

struct cache_line
{
    int valid;
    __uint64_t tag;

    struct cache_line *prev;
    struct cache_line *next;
};

struct cache_set
{
    int E; // E cache lines in each of the cache sets
    struct cache_line *head;
    struct cache_line *tail;
};

struct cache_sim
{
    int verbose;
    int s; // 2^s cache sets in cache_sim
    int E; // E cache lines in each of the cache sets
    int b; // 2^b bytes block each line
    char *trace_file;

    int hit_count;
    int miss_count;
    int eviction_count;

    struct cache_set **cache;
};

void alloc_cache(struct cache_sim *csim);
void print_help(void);
void add_cline(struct cache_set *cset);
void simulate(struct cache_sim *csim);
void cache_access(struct cache_sim *csim, __uint64_t addr, int size);
void prepend_line(struct cache_set *cset, struct cache_line *cline);
void remove_line(struct cache_line *cline);
int is_arg_valid(struct cache_sim *csim);
struct cache_sim *new_csim(void);
struct cache_sim *cache_init(int argc, char *argv[]);
struct cache_line *find_cline(struct cache_set *cset, __uint64_t ct);
struct cache_line *get_cline(struct cache_set *cset, __uint64_t ct);
struct cache_set *new_cset(int E);

int main(int argc, char *argv[])
{
    struct cache_sim *csim;

    csim = cache_init(argc, argv);
    if (is_arg_valid(csim))
    {
        alloc_cache(csim);
    }

    simulate(csim);

    printSummary(csim->hit_count, csim->miss_count, csim->eviction_count);
    return 0;
}

int is_arg_valid(struct cache_sim *csim)
{
    return csim->s > 0 && csim->E > 0 && csim->b > 0 && csim->trace_file;
}

void alloc_cache(struct cache_sim *csim)
{
    int S = 1 << (csim->s); // S = 2^s cache sets in cache_sim
    int E = csim->E;        // E cache lines in each of the cache sets
    struct cache_set **cache = (struct cache_set **)malloc(sizeof(struct cache_set *) * S);

    // allocate space for cache lines
    for (int i = 0; i < S; i++)
    {
        cache[i] = new_cset(E);
    }

    csim->cache = cache;
}

/* Return new cache set with E lines.
 * If E<0, then print error and return null */
struct cache_set *new_cset(int E)
{
    if (E < 0)
    {
        printf("Illegal argument");
        return NULL;
    }
    int i;
    struct cache_set *cset = (struct cache_set *)malloc(sizeof(struct cache_set));
    struct cache_line *head = (struct cache_line *)malloc(sizeof(struct cache_line));
    struct cache_line *tail = (struct cache_line *)malloc(sizeof(struct cache_line));

    head->prev = NULL;
    head->next = tail;

    tail->prev = head;
    tail->next = NULL;

    cset->E = E;
    cset->head = head;
    cset->tail = tail;

    for (i = 0; i < E; i++)
        add_cline(cset);
    return cset;
}

void add_cline(struct cache_set *cset)
{
    struct cache_line *curr_line = (struct cache_line *)malloc(sizeof(struct cache_line));
    curr_line->valid = 0;
    prepend_line(cset, curr_line);
}

/* 
 * add an empty cline to the beginning of the given cache_set
 */
void prepend_line(struct cache_set *cset, struct cache_line *cline)
{
    struct cache_line *head = cset->head;
    struct cache_line *head_next = head->next;

    cline->prev = head;
    cline->next = head_next;

    head->next = cline;
    head_next->prev = cline;
}

/* 
 * remove a given cline when the cset is not NULL
 */
void remove_line(struct cache_line *cline)
{
    struct cache_line *next_cline = cline->next;
    struct cache_line *prev_cline = cline->prev;
    prev_cline->next = next_cline;
    next_cline->prev = prev_cline;
}

struct cache_sim *new_csim(void)
{
    struct cache_sim *csim = malloc(sizeof(struct cache_sim));
    csim = calloc(1, sizeof(struct cache_sim)); // set all space in csim to zero
    return csim;
}

void simulate(struct cache_sim *csim)
{
    FILE *f;
    __uint64_t addr;
    int size;
    int verbose = csim->verbose;
    char type;

    f = fopen(csim->trace_file, "r");

    while (fscanf(f, " %c %lx,%d\n", &type, &addr, &size) > 0)
    {
        if (verbose && type != 'I')
            printf("%c %lx,%d", type, addr, size);
        switch (type)
        {
        case 'L':
        case 'S':
            // a data store
            cache_access(csim, addr, size);
            break;
        case 'M':
            // a data modify
            cache_access(csim, addr, size); // a data load
            cache_access(csim, addr, size); // followed by a data store
            break;
        }
        if (verbose)
            printf("\n");
    }
    fclose(f);
}

/*
 * simulate the process of accesing cache memory
 */
void cache_access(struct cache_sim *csim, __uint64_t addr, int size)
{
    int s = csim->s;
    int b = csim->b;
    int verbose = csim->verbose;
    __uint64_t ci_mask = (1 << (s + 1)) - 1;
    __uint64_t ci = (addr >> b) & ci_mask;
    __uint64_t ct = addr >> (s + b);
    struct cache_set *cset = csim->cache[ci];
    struct cache_line *cline = find_cline(cset, ct);
    if (cline)
    {
        if (verbose)
            printf(" hit");

        // first remove then prepend, so the used cline will always at
        // the beginning of cset, which is the last one for evictions
        remove_line(cline);
        prepend_line(cset, cline);

        (csim->hit_count)++;
    }
    else
    {
        if (verbose)
            printf(" miss");
        // TODO:
    }
}

/*
 * return valid cache line in a given cache set
 * return NULL if not find or cache line not valid
 */
struct cache_line *find_cline(struct cache_set *cset, __uint64_t ct)
{
    struct cache_line *curr_line = cset->head->next;
    struct cache_line *tail = cset->tail;

    while (curr_line != tail)
    {
        if (curr_line->valid && curr_line->tag == ct)
            return curr_line;

        curr_line = curr_line->next;
    }

    return NULL;
}

/*
 * return an unused(invalid) line for miss case
 */
struct cache_line *get_cline(struct cache_set *cset, __uint64_t ct)
{
    struct cache_line *curr_line = cset->head->next;
    struct cache_line *tail = cset->tail;

    while (curr_line != tail)
    {
        // return an unused(invalid) line
        // so that data in valid lines won't lose
        if (!curr_line->valid)
        {
            return curr_line;
        }
        curr_line = curr_line->next; // find the next unused line
    }
    return NULL;
}

struct cache_sim *cache_init(int argc, char *argv[])
{
    struct cache_sim *csim = new_csim();
    extern char *optarg;
    char opt;
    char *trace_name;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            print_help();
            exit(1);
        case 'v':
            csim->verbose = 1;
            break;
        case 's':
            csim->s = atoi(optarg);
        case 'E':
            csim->E = atoi(optarg);
            break;
        case 'b':
            csim->b = atoi(optarg);
            break;
        case 't':
            trace_name = malloc(strlen(optarg) + 1);
            strcpy(trace_name, optarg);
            csim->trace_file = optarg;
            break;
        }
    }

    return csim;
}

void print_help(void)
{
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h              Print this help message.\n");
    printf("  -v              Optional verbose flag.\n");
    printf("  -s <num>        Number of set index bits.\n");
    printf("  -E <num>        Number of lines per set.\n");
    printf("  -b <num>        Number of block offset bits.\n");
    printf("  -t <tracefile>  Name of the valgrind trace to replay.\n\n");
    printf("Examples:\n");
    printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim -v -s 4 -E 1 -b 4 -t traces/yi.trace\n");
}