#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>

static Display *dpy;
long cpu0_work = 0;
long cpu0_total = 0;
long cpu1_work = 0;
long cpu1_total = 0;

long cpu2_work = 0;
long cpu2_total = 0;

long cpu3_work = 0;
long cpu3_total = 0;

void setstatus(char *str) {
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

int getcpu(char *status, size_t size) {
    FILE *fd;
    long jif1, jif2, jif3, jif4, jif5, jif6, jif7;
    long work0, total0, work1, total1, work2, total2, work3, total3;
    int load0, load1, load2, load3;

    fd = fopen("/proc/stat", "r");
    char c;
    while (c != '\n') c = fgetc(fd);
    fscanf(fd, "cpu0 %ld %ld %ld %ld %ld %ld %ld", &jif1, &jif2, &jif3, &jif4, &jif5, &jif6, &jif7);
    work0 = jif1 + jif2 + jif3 + jif6 + jif7;
    total0 = work0 + jif4 + jif5;

    c = 0;
    while (c != '\n') c = fgetc(fd);
    fscanf(fd, "cpu1 %ld %ld %ld %ld %ld %ld %ld", &jif1, &jif2, &jif3, &jif4, &jif5, &jif6, &jif7);
    work1 = jif1 + jif2 + jif3 + jif6 + jif7;
    total1 = work1 + jif4 + jif5;

    c = 0;
    while (c != '\n') c = fgetc(fd);
    fscanf(fd, "cpu2 %ld %ld %ld %ld %ld %ld %ld", &jif1, &jif2, &jif3, &jif4, &jif5, &jif6, &jif7);
    work2 = jif1 + jif2 + jif3 + jif6 + jif7;
    total2 = work2 + jif4 + jif5;

    c = 0;
    while (c != '\n') c = fgetc(fd);
    fscanf(fd, "cpu3 %ld %ld %ld %ld %ld %ld %ld", &jif1, &jif2, &jif3, &jif4, &jif5, &jif6, &jif7);
    work3 = jif1 + jif2 + jif3 + jif6 + jif7;
    total3 = work3 + jif4 + jif5;

    fclose(fd);

    load0 = 100 * (work0 - cpu0_work) / (total0 - cpu0_total);
    load1 = 100 * (work1 - cpu1_work) / (total1 - cpu1_total);
    load2 = 100 * (work2 - cpu2_work) / (total2 - cpu2_total);
    load3 = 100 * (work3 - cpu3_work) / (total3 - cpu3_total);

    cpu0_work = work0;
    cpu0_total = total0;
    cpu1_work = work1;
    cpu1_total = total1;
    cpu2_work = work2;
    cpu2_total = total2;
    cpu3_work = work3;
    cpu3_total = total3;

    return snprintf(status, size, "CPU %02d%% %02d%% %02d%% %02d%%", load0, load1, load2, load3);
}

int getmem(char *status, size_t size) {
    int total,
        used;
    FILE *freeCommandResult = popen("free | grep Mem | cut -d':' -f2", "r");
    if (!freeCommandResult)
    {
        return -1;
    }
    fscanf(freeCommandResult, "%d %d", &total, &used);
    pclose(freeCommandResult);
    return snprintf(status, size, "| MEM %01d%%", 100 * used / total);
}

int getdatetime(char *status, size_t size) {
    time_t result;
    struct tm *resulttm;

    result = time(NULL);
    resulttm = localtime(&result);

    return strftime(status, size, "| %Y.%m.%d %a | %H:%M:%S", resulttm);
}

int getbattery(char *status, size_t size) {
    FILE *fd;
    int now, full, bat;
    char stat[12];

    fd = fopen("/sys/class/power_supply/BAT0/energy_now", "r");
    fscanf(fd, "%d", &now);
    fclose(fd);

    fd = fopen("/sys/class/power_supply/BAT0/energy_full", "r");
    fscanf(fd, "%d", &full);
    fclose(fd);

    fd = fopen("/sys/class/power_supply/BAT0/status", "r");
    fscanf(fd, "%s", stat);
    fclose(fd);

    bat = 100 * (float)now / full;

    if(strncmp(stat, "Discharging", 11) == 0) {
        return snprintf(status, size, "| BAT %d%% ", bat);
    } else {
        return snprintf(status, size, "| BAT AC %d%% ", bat);
    }
}

int main(void) {
    char status[100];
    int l = 0;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display.\n");
        return 1;
    }

    for (;;sleep(2)) {
        l = 0;
        l = getcpu(status, sizeof(status));
        l += getmem(status + l, sizeof(status) - l);
        l += getbattery(status + l, sizeof(status) - l);
        l += getdatetime(status + l, sizeof(status) - l);

        setstatus(status);
    }

    free(status);
    XCloseDisplay(dpy);

    return 0;
}