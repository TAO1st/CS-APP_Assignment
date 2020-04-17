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
    int s;
    int E;
    int b;
    char *trace_file;

    int hit_count;
    int miss_count;
    int eviction_count;

    struct cache_set **cache;
};

void print_help(void);
struct cache_sim *new_csim(void);
struct cache_sim *cache_init(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    struct cache_sim *csim;

    csim = cache_init(argc, argv);

    if (csim->verbose)
    {
        printf("%s", csim->trace_file);
    }

    printSummary(csim->hit_count, csim->miss_count, csim->eviction_count);
    return 0;
}

struct cache_sim *new_csim(void)
{
    struct cache_sim *csim = malloc(sizeof(struct cache_sim));
    csim = calloc(1, sizeof(struct cache_sim)); // set all space in csim to zero
    return csim;
}

struct cache_sim *cache_init(int argc, char *argv[])
{
    struct cache_sim *csim = new_csim();
    extern char *optarg;
    char opt;

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
            csim->trace_file = optarg;
            break;
        }
    }

    // csim->hit_count = 0;
    // csim->miss_count = 0;
    // csim->eviction_count = 0;

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