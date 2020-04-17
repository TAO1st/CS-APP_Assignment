#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

int main(int argc, char *argv[])
{
    extern char *optarg;

    int verbose = 0;
    int s = 0;
    int E = 0;
    int b = 0;
    char *fileName = NULL;
    char opt;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'v':
            verbose = 1;
            break;
        case 's':
            s = atoi(optarg);
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            fileName = optarg;
            break;
        case 'h':
            printf("usage: hvs:E:b:t:");
            exit(1);
        }
    }
    if (verbose)
    {
        printf("%d %d %d %s\n", s, E, b, fileName);
    }

    printSummary(0, 0, 0);
    return 0;
}
