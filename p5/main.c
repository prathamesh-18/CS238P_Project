/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * main.c
 */

#include <signal.h>
#include "system.h"
#include <unistd.h>


/**
 * Needs:
 *   signal()
 */

static volatile int done;

static void
_signal_(int signum) {
    assert(SIGINT == signum);

    done = 1;
}

double
cpu_util(const char *s) {
    static unsigned sum_, vector_[7];
    unsigned sum, vector[7];
    const char *p;
    double util;
    uint64_t i;

    /*
      user
      nice
      system
      idle
      iowait
      irq
      softirq
    */

    if (!(p = strstr(s, " ")) ||
        (7 != sscanf(p,
                     "%u %u %u %u %u %u %u",
                     &vector[0],
                     &vector[1],
                     &vector[2],
                     &vector[3],
                     &vector[4],
                     &vector[5],
                     &vector[6]))) {
        return 0;
    }
    sum = 0.0;
    for (i = 0; i < ARRAY_SIZE(vector); ++i) {
        sum += vector[i];
    }
    util = (1.0 - (vector[3] - vector_[3]) / (double) (sum - sum_)) * 100.0;
    sum_ = sum;
    for (i = 0; i < ARRAY_SIZE(vector); ++i) {
        vector_[i] = vector[i];
    }
    return util;
}

void memory_stats(void) {
    const char *const PROC_MEMINFO = "/proc/meminfo";
    FILE *mem_file;
    char line[1024];
    unsigned long mem_total = 0, mem_free = 0;

    if (!(mem_file = fopen(PROC_MEMINFO, "r"))) {
        TRACE("fopen() - Memory");
        return;
    }

    while (fgets(line, sizeof(line), mem_file)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%lu", &mem_total);
        } else if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line + 8, "%lu", &mem_free);
        }
    }

    fclose(mem_file);

    printf(" | Memory Used Percentage: %5.1f%%", (double) (mem_total - mem_free) / (double) mem_total * 100);
}

void network_stats(char *interface) {
    const char *const PROC_NET_DEV = "/proc/net/dev";
    FILE *net_file;
    char line[1024];
    int i;

    if (!(net_file = fopen(PROC_NET_DEV, "r"))) {
        TRACE("fopen() - Network");
        return;
    }

    for (i = 0; i < 2; ++i) {
        if (!fgets(line, sizeof(line), net_file)) {
            TRACE("fgets() - Network");
            fclose(net_file);
            return;
        }
    }
	printf("Lasan");
    while (fgets(line, sizeof(line), net_file)) {
        char iface[32];
        unsigned long recv, send;

        if (sscanf(line, "%s %lu %*u %*u %*u %*u %*u %*u %*u %lu", iface, &recv, &send) == 3) {
			printf("1\n");
            if (strcmp(iface, interface) == 0) {
				printf("2\n");
                printf(" | %s Receive: %lu bytes | %s Send: %lu bytes", interface, recv, interface, send);
                break;
            }
        }
    }

    fclose(net_file);
}

void disk_stats(char *disk) {
    const char *const PROC_DISK_STATS = "/proc/diskstats";
    FILE *disk_file;
    char line[1024];

    if (!(disk_file = fopen(PROC_DISK_STATS, "r"))) {
        TRACE("fopen() - Disk");
        return;
    }

    while (fgets(line, sizeof(line), disk_file)) {
        char dev_name[32];
        unsigned long rd_ios, wr_ios;

        if (sscanf(line, "%*u %*u %s %*u %*u %*u %lu %*u %*u %*u %lu", dev_name, &rd_ios, &wr_ios) != 3) {
            continue;
        }

        if (strcmp(dev_name, disk) == 0) {
            printf(" | %s Read: %lu ms | %s Write: %lu ms", disk, rd_ios, disk, wr_ios);
            break;
        }
    }

    fclose(disk_file);
}

void time_plus_stats(void) {
    const char *const PROC_STAT = "/proc/stat";
    FILE *stat_file;
    char line[1024];
    unsigned long  user_time, system_time, idle_time;
    unsigned long  total_time;
    double cpu_time;

    if (!(stat_file = fopen(PROC_STAT, "r"))) {
        TRACE("fopen() - CPU Time+");
        return;
    }

    if (fgets(line, sizeof(line), stat_file)) {

        sscanf(line, "cpu %lu %*u %lu %lu", &user_time, &system_time, &idle_time);

   
        total_time = user_time + system_time + idle_time;
        cpu_time = (double) total_time / (double) sscanf(_SC_CLK_TCK); 

        printf(" | Total CPU Time (TIME+): %5.2f seconds", cpu_time);
    }

    fclose(stat_file);
}

int main(int argc, char *argv[]) {
    const char *const PROC_STAT = "/proc/stat";

    char line[1024];
    FILE *file;

    UNUSED(argc);
    UNUSED(argv);

    if (SIG_ERR == signal(SIGINT, _signal_)) {
        TRACE("signal()");
        return -1;
    }

    while (!done) {
        if (!(file = fopen(PROC_STAT, "r"))) {
            TRACE("fopen() - CPU");
            return -1;
        }
        if (fgets(line, sizeof(line), file)) {
            printf("\rCPU Utilization: %5.1f%%", cpu_util(line));
            fflush(stdout);
        }
        fclose(file);

        memory_stats();
        network_stats("eth0:");
        disk_stats("sda");
		time_plus_stats();

        us_sleep(500000);
    }

    printf("\rDone!                                                                                            \n");
    return 0;
}
