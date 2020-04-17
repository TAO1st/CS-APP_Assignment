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
int is_arg_valid(struct cache_sim *csim);
struct cache_sim *new_csim(void);
struct cache_sim *cache_init(int argc, char *argv[]);
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
    for (int i = 0; i < E; i++)
    {
        cache[i] = (struct cache_set *)malloc(sizeof(struct cache_line *) * E);
    }
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
    struct cache_line *this_line = (struct cache_line *)malloc(sizeof(struct cache_line));
    this_line->valid = 0;
    this_line->prev = cset->head;
    this_line->next = cset->head->next;
    cset->head->next = this_line;
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
    unsigned long long addr;
    int size;
    int verbose = csim->verbose;
    char type;

    f = fopen(csim->trace_file, "r");

    while (fscanf(f, " %c %llx,%d\n", &type, &addr, &size) > 0)
    {
        if (verbose && type != 'I')
            printf("%c %llx,%d", type, addr, size);
        switch (type)
        {
        case 'L':
        case 'S':
            //TODO: do sth with csim,addr, size;
            break;
        case 'M':
            //TODO: do sth with csim,addr, size;
            break;
        }
        if (verbose)
            printf("\n");
    }
    fclose(f);
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