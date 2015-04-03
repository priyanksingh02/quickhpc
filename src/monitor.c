#include <sys/ptrace.h>
#include <limits.h>
#include "config.h"

#ifdef _AIX
#define _LINUX_SOURCE_COMPAT
#endif

#if defined(__FreeBSD__)
# define PTRACE_ATTACH PT_ATTACH
# define PTRACE_CONT PT_CONTINUE
#endif

inline unsigned long gettime() {
  volatile unsigned long tl;
  asm __volatile__("lfence\nrdtsc" : "=a" (tl): : "%edx");
  return tl;
}

void initPapi();
void initEventSet(int *);
void addEvent(int, char *eventName);
void monitor(int, long long **values);
void buildCSVLine(char *line, long long *values, int numItems);
void cleanup(int *eventSet);
void usage(char *name);

int main( int argc, char *argv[] )
{
        int eventSet = PAPI_NULL;
        // unsigned long t1, t2; // Used to measure timing of various calls
        int numEventSets = 1; // One set should be enough
        int idx;
        int retval;
        long long **values;
        //long long elapsed_us, elapsed_cyc, elapsed_virt_us, elapsed_virt_cyc;
        char monitorLine[PAPI_MAX_STR_LEN];
        pid_t pid = -1;
        config cfg;
        bool processAlive = true;

        if (argc < 3) {
            usage(argv[0]);
        }

        initPapi();

        /* Load configuration from arguments */
        setup_config(argc, argv, &cfg);
       
        if (cfg.run && cfg.attach) {
            fprintf(stderr, "Cannot attach and run! Choose one.\n");
            exit(1);
        }

        if (cfg.run) {
            pid = 0; // TODO run process and get the PID
        }

        if (cfg.attach) {
            pid = cfg.pid;
            printf("Attaching to PID: %d\n", pid);
        }

        if (pid < 0) {
            printf("Please specify a PID to attach to with -a" /* or -r to run a process*/);
        }

        initEventSet(&eventSet);

        for (idx = 0; idx < cfg.numEvents; idx++) {
            addEvent(eventSet, cfg.events[idx]); // TODO
        }

        retval = PAPI_attach( eventSet, ( unsigned long ) pid );
        if ( retval != PAPI_OK )
                test_fail_exit( __FILE__, __LINE__, "PAPI_attach", retval );

        values = allocate_test_space(numEventSets, cfg.numEvents);

        /* Print CSV headers */
        printf("\n");
        for (idx = 0; idx < cfg.numEvents; idx++) {
            printf("%s", cfg.events[idx]);
            if (idx < cfg.numEvents - 1) {
                printf(",");
            }
        }
        retval = PAPI_start( eventSet );
        if ( retval != PAPI_OK )
            test_fail_exit( __FILE__, __LINE__, "PAPI_start", retval );
        /* Start monitoring and print values */
        printf("\n");
        for (idx = 0; processAlive && (idx < cfg.iterations || cfg.iterations == -1); idx++) {
            // t1 = gettime();

            monitor(eventSet, values);

            // retval = PAPI_read( eventSet, values[0] );
            // if ( retval != PAPI_OK )
            //     test_fail_exit( __FILE__, __LINE__, "PAPI_read", retval );

            // retval = PAPI_reset( eventSet );
            // if ( retval != PAPI_OK )
            //     test_fail_exit( __FILE__, __LINE__, "PAPI_reset", retval );

            // t2 = gettime();

            if (cfg.interval > 0)
                usleep(cfg.interval);

            // printf("Time to monitor: %lu\n", t2 - t1);
            buildCSVLine(monitorLine, values[0], cfg.numEvents);
            printf("%s\n", monitorLine);
            if (kill(pid, 0) < 0) {
                /* Process is not active anymore */
                if (errno == ESRCH) {
                    processAlive = false;
                }
            }
            // t2 = gettime();
            // printf("Time to end the loop: %lu\n", t2 - t1);
        }

        retval = PAPI_stop( eventSet, values[0]);
        if ( retval != PAPI_OK )
            test_fail_exit( __FILE__, __LINE__, "PAPI_stop", retval );
        
        cleanup(&eventSet);

        exit(0);
}

void initPapi(){
    int retval;
    const PAPI_hw_info_t *hw_info;
    const PAPI_component_info_t *cmpinfo;
    retval = PAPI_library_init( PAPI_VER_CURRENT );
    if ( retval != PAPI_VER_CURRENT )
            test_fail_exit( __FILE__, __LINE__, "PAPI_library_init", retval );

    if ( ( cmpinfo = PAPI_get_component_info( 0 ) ) == NULL )
            test_fail_exit( __FILE__, __LINE__, "PAPI_get_component_info", 0 );

    if ( cmpinfo->attach == 0 )
            test_skip( __FILE__, __LINE__, "Platform does not support attaching",
                               0 );

    hw_info = PAPI_get_hardware_info(  );
    if ( hw_info == NULL )
            test_fail_exit( __FILE__, __LINE__, "PAPI_get_hardware_info", 0 );
}

void initEventSet(int *eventSet) {
    int retval;
    /* add PAPI_TOT_CYC and one of the events in PAPI_FP_INS, PAPI_FP_OPS or
       PAPI_TOT_INS, depending on the availability of the event on the
       platform */
    retval = PAPI_create_eventset(eventSet);
    if ( retval != PAPI_OK )
            test_fail_exit( __FILE__, __LINE__, "PAPI_create_eventset", retval );

    /* Here we are testing that this does not cause a fail */

    retval = PAPI_assign_eventset_component( *eventSet, 0 );
    if ( retval != PAPI_OK )
            test_fail( __FILE__, __LINE__, "PAPI_assign_eventset_component",
            retval );

    /* Enable multiplexing to be able to probe more counters (less precision) */
    // retval = PAPI_set_multiplex(*eventSet);
    // if ( retval != PAPI_OK )
    //         test_fail( __FILE__, __LINE__, "PAPI_set_multiplex",
    //         retval );
}

void addEvent(int eventSet, char *eventName) {
    int eventCode = 0;
    int retval;
    char msg[PAPI_MAX_STR_LEN];
    PAPI_event_name_to_code(eventName, &eventCode );
    retval = PAPI_add_event(eventSet, eventCode);
    if ( retval != PAPI_OK ) {
        sprintf(msg, "PAPI_add_event (%s)", eventName);
        test_fail_exit( __FILE__, __LINE__, msg, retval );
    }
}

void monitor(int eventSet, long long **values) {
    int retval;

    retval = PAPI_read( eventSet, values[0] );
    if ( retval != PAPI_OK )
        test_fail_exit( __FILE__, __LINE__, "PAPI_read", retval );

    retval = PAPI_reset( eventSet );
    if ( retval != PAPI_OK )
        test_fail_exit( __FILE__, __LINE__, "PAPI_reset", retval );
}

void buildCSVLine(char *line, long long *values, int numItems) {
    int i = 0;
    if (values == NULL || numItems <= 0) {
        printf("Error: invalid CSV line! (%s, line %d)\n", __FILE__, __LINE__);
        exit(1);
    }
    for (; i < numItems; i++) {
        if (i == 0) {
            sprintf(line, "%lld", values[i]);
        } else {
            sprintf(line, "%s,%lld", line, values[i]);            
        }
    }
}

void cleanup(int *eventSet) {
    int retval;
    retval = PAPI_cleanup_eventset(*eventSet);
    if (retval != PAPI_OK)
      test_fail_exit( __FILE__, __LINE__, "PAPI_cleanup_eventset", retval );

    retval = PAPI_destroy_eventset(eventSet);
    if (retval != PAPI_OK)
      test_fail_exit( __FILE__, __LINE__, "PAPI_destroy_eventset", retval );
}

void usage(char *name) {
    printf("Usage: %s -a PID -c EVENTS_FILE [-i INTERVAL (usecs)] [-n SAMPLES]\n", name);
    printf("\t(If no interval is specified the sampling is performed as quickly as possible)\n");
    printf("\t(If no number of samples is specified the execution continues until either %s is killed or the attached process terminates)\n", name);
    exit(1);
}