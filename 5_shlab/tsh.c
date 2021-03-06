/*
 * tsh - A tiny shell program with job control
 *
 * @author Li Li
 * @e_mail LiL147@pitt.edu
 */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t {             /* The job struct */
    pid_t pid;             /* job PID */
    int jid;               /* job ID [1, 2, ...] */
    int state;             /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);
int updatejob(struct job_t *jobs, pid_t pid, int state);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv) {
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
            case 'h': /* print help message */
                usage();
                break;
            case 'v': /* emit additional diagnostic info */
                verbose = 1;
                break;
            case 'p':            /* don't print a prompt */
                emit_prompt = 0; /* handy for automatic testing */
                break;
            default:
                usage();
        }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {
        /* Read command line */
        if (emit_prompt) {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin)) { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/* (Graded)
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(char *cmdline) {
    pid_t pid;           /* Process id */
    int bg;              /* Should the job runin bg or fg */
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    sigset_t mask_all, mask_one, prev_one;

    strcpy(buf, cmdline);

    /* true if the user has requested a BG job
     * false if the user has requested a FG job. */
    bg = parseline(cmdline, argv);

    if (argv[0] == NULL) return;

    sigfillset(&mask_all);         /* use it to mask all signals */
    sigemptyset(&mask_one);        /* empty mask_one first */
    sigaddset(&mask_one, SIGCHLD); /* use it to mask SIGCHLD */

    if (!builtin_cmd(argv)) { /* if it is builtin_cmd */
        /* Block SIGCHLD before parent forks the child */
        sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

        if ((pid = fork()) == 0) { /* child execve user's command */
            /* Unblock SIGCHLD in the child */
            sigprocmask(SIG_SETMASK, &prev_one, NULL);
            setpgid(0, 0);

            /* execve returns ???1 on error */
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found\n", argv[0]);
                fflush(stdout);
                exit(0);
            }
        } else { /* parent add the job to the joblist */
            /*** IMPORTANT ***
             * the parent need to block SIGCHLD to avoid the race condition
             * where the child is reaped by sigchld_handler before the parent
             * calls addjob */
            sigprocmask(SIG_BLOCK, &mask_all, NULL);
            addjob(jobs, pid, (bg == 1 ? BG : FG), cmdline);
            sigprocmask(SIG_SETMASK, &prev_one, NULL);

            if (!bg) /* parent waits for foreground job to terminate or stop */
                waitfg(pid);
            else /* parent print for backgroung job */
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        }
    }
    return;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv) {
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
        buf++;
        delim = strchr(buf, '\'');
    } else {
        delim = strchr(buf, ' ');
    }

    while (delim) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;

        if (*buf == '\'') {
            buf++;
            delim = strchr(buf, '\'');
        } else {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;

    if (argc == 0) /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0) {
        argv[--argc] = NULL;
    }
    return bg;
}

/* (Graded)
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv) {
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
    {
        return 1;
    }

    if (!strcmp(argv[0], "quit")) /* quit command */
    {
        exit(0);
    }
    if (!strcmp(argv[0], "fg")) /* fg command */
    {
        do_bgfg(argv);
        return 1;
    }
    if (!strcmp(argv[0], "bg")) /* bg command */
    {
        do_bgfg(argv);
        return 1;
    }
    if (!strcmp(argv[0], "jobs")) /* jobs command */
    {
        listjobs(jobs);
        return 1;
    }

    return 0; /* not a builtin command */
}

/* (Graded)
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) {
    struct job_t *jobp;      /* current job pointer */
    char *cmd = argv[0];     /* pointer to "bg" or "fg" */
    char *arg = &argv[1][1]; /* pointer to pid or jid */
    int jid = atoi(arg);     /* jid (without %) */
    int pid;                 /* pid */

    /* ignore command if no argument */
    if (argv[1] == NULL) {
        printf("%s command requires PID or %%jobid argument\n", cmd);
        return;
    }

    if (argv[1][0] == '%') { /* handle for jid */
        jobp = getjobjid(jobs, jid);

        if (jobp == NULL) {
            printf("%s: No such job\n", argv[1]);
            return;
        }

        pid = jobp->pid;
    } else { /* handle for pid */
        pid = atoi(argv[1]);

        if (pid == 0) {
            printf("%s: argument must be a PID or %%jobid\n", cmd);
            return;
        }

        jobp = getjobpid(jobs, pid);
        if (jobp == NULL) {
            printf("(%d): No such process\n", pid);
            return;
        }
    }

    if (!strcmp(cmd, "bg")) {
        /* if the process in the group in suspended, then it'll continue */
        kill(-pid, SIGCONT); /* Send SIGCONT to the whole pid process group */
        updatejob(jobs, pid, BG);
        printf("[%d] (%d) %s", pid2jid(pid), pid, jobp->cmdline);
    }
    if (!strcmp(cmd, "fg")) {
        /* if the process in the group in suspended, then it'll continue */
        kill(-pid, SIGCONT); /* Send SIGCONT to the whole pid process group */
        updatejob(jobs, pid, FG);
        waitfg(pid); /* wait for the fg job to terminate */
    }
}

/* (Graded)
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid) {
    sigset_t mask_temp;

    /* can be interrupted by any signals */
    sigemptyset(&mask_temp);  /* unblock any signal */
    while (fgpid(jobs) > 0) { /* use a busy loop around sleep function */
        /* blocked until process pid is no longer fg process */
        sigsuspend(&mask_temp);
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/* (Graded)
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */
void sigchld_handler(int sig) {
    pid_t pid;
    int status;
    sigset_t mask_all, prev_all;

    if (verbose) printf("sigchld_handler: entering \n");

    /*
     * Reap any zombie jobs.
     * The WNOHANG here is important. Without it, the the handler would wait for
     * all running or stopped BG jobs to terminate, during which time the shell
     * would not be able to accept input.
     *
     * WUNTRACED | WNOHANG: return immediately, with a retrun value of 0, if
     * none of the children in the wait set has stopped or terminated, or with a
     * return value equal to the PID of one of the stopped or terminated
     * children.
     */
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {
        /* WIFSTOPPED(status) returns true if the child that caused the
         * return is currently stopped */
        if (WIFSTOPPED(status)) {
            /* FG job has stopped. Change its state in jobs list */
            sprintf(sbuf, "Job [%d] (%d) stopped by signal %d", pid2jid(pid),
                    pid, WSTOPSIG(status));
            printf("%s\n", sbuf);
            updatejob(jobs, pid, ST);
        }

        else {
            /* check if job was terminated by an uncaught signal.
             * WIFSIGNALED(status) returns true if the child process terminated
             * beacuse of a signal that was not caught */
            if (WIFSIGNALED(status)) {
                /* FG job has terminated. Remove it from jobs list */
                sprintf(sbuf, "Job [%d] (%d) terminated by signal %d",
                        pid2jid(pid), pid, WTERMSIG(status));
                printf("%s\n", sbuf);
                fflush(stdout);
                deletejob(jobs, pid);
            }

            /* Block all signals and save previous blocked set */
            sigfillset(&mask_all);
            sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            deletejob(jobs, pid);
            /* Restore previous blocked set */
            sigprocmask(SIG_SETMASK, &prev_all, NULL);

            if (verbose) printf("sigchld_handler: job %d deleted\n", pid);
        }
    }

    /*
     * Check for normal loop termination.
     * This is a little tricky. For our purposes,
     * the waitpid loop terminates normally for one of
     * two reasons: (a) there are no children left
     * (pid == -1 and errno == ECHILD) or (b) there are
     * still children left, but none of them are zombies (pid == 0).
     */
    if (!((pid == 0) || (pid == -1 && errno == ECHILD)))
        unix_error("sigchld_handler wait error");

    if (verbose) printf("sigchld_handler: exiting\n");

    return;
}

/* (Graded)
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig) {
    /* only fg job receive SIGINT */
    pid_t pid = fgpid(jobs);
    if (pid) {
        /* Send SIGINT signal to the entire fg process group */
        if (kill(-pid, SIGINT) != 0) { /* returns 0 if OK, -1 on error */
            unix_error("Kill failed");
        }
    }
    return;
}

/* (Graded)
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig) {
    /* only fg job receive SIGINT */
    pid_t pid = fgpid(jobs);
    if (pid != 0) {
        /* Send SIGTSTP signal to the entire fg process group */
        if (kill(-pid, SIGTSTP) != 0) { /* returns 0 if OK, -1 on error */
            unix_error("Suspend failed!");
        }
    }
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++) clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) {
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid > max) max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) {
    int i;

    if (pid < 1) return 0;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS) nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            if (verbose) {
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid,
                       jobs[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1) return 0;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == pid) {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs) + 1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].state == FG) return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1) return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid) return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) {
    int i;

    if (jid < 1) return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid) return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) {
    int i;

    if (pid < 1) return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid != 0) {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state) {
                case BG:
                    printf("Running ");
                    break;
                case FG:
                    printf("Foreground ");
                    break;
                case ST:
                    printf("Stopped ");
                    break;
                default:
                    printf("listjobs: Internal error: job[%d].state=%d ", i,
                           jobs[i].state);
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}

/* updatejob - update the state of a job with PID=pid */
int updatejob(struct job_t *jobs, pid_t pid, int state) {
    int i;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == pid) {
            jobs[i].state = state;
            return 1;
        }
    }
    printf("Job %d not found\n", pid);
    return 0;
}

/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) {
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg) {
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg) {
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) {
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0) unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) {
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
