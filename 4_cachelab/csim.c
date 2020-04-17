#include "cachelab.h"

int main(int argc, char *argv[])
{
    extern char *optarg;
    extern int optind, opterr, optopt;

    int verbose = 0;
    int n = 0;
    int s = 0;
    int E = 0;
    int b = 0;
    char fileName[10];
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
        case 't':
            *fileName = optarg;
        case 'h':
            printf("usage: hvs:E:b:t:");
            exit(1);
        }
    }
    int i;
    for (i = 0; i < n; i++)
    {
        if (verbose)
            printf("%d\n", i);
    }

    printSummary(0, 0, 0);
    return 0;
}
