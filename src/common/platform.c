/* OS boundary for clocks, sleep, memory, terminal, and CPU affinity. */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#if defined(__linux__)
#include <sched.h>
#endif
#endif

/* Read the monotonic clock so measurements do not follow wall-clock changes. */
uint64_t plat_now_ns(void)
{
#if defined(_WIN32)
    LARGE_INTEGER ticks;
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency) || !QueryPerformanceCounter(&ticks)) return 0;
    return (uint64_t)((ticks.QuadPart / frequency.QuadPart) * 1000000000ULL +
        (ticks.QuadPart % frequency.QuadPart) * 1000000000ULL / frequency.QuadPart);
#else
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) return 0;
    return (uint64_t)value.tv_sec * 1000000000ULL + (uint64_t)value.tv_nsec;
#endif
}

/* Sleep outside timed loops; nanosleep is available on POSIX systems. */
int plat_sleep_ms(unsigned milliseconds)
{
#if defined(_WIN32)
    Sleep(milliseconds);
    return 0;
#else
    struct timespec request;
    request.tv_sec = (time_t)(milliseconds / 1000U);
    request.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
    return nanosleep(&request, NULL);
#endif
}

/* Identify the build host for machine-readable benchmark labels. */
const char *plat_platform_name(void)
{
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

/* Query resident memory using the OS API while keeping callers portable. */
uint64_t plat_peak_memory_kb(void)
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters;
    counters.cb = sizeof counters;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters)) return 0;
    return (uint64_t)counters.PeakWorkingSetSize / 1024U;
#elif defined(__APPLE__) || defined(__linux__)
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0;
#if defined(__APPLE__)
    return (uint64_t)usage.ru_maxrss / 1024U;
#else
    return (uint64_t)usage.ru_maxrss;
#endif
#else
    return 0;
#endif
}

/* Enable Windows virtual-terminal output; POSIX terminals need no setup. */
int plat_enable_ansi(void)
{
#if defined(_WIN32)
    DWORD mode;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE || !GetConsoleMode(output, &mode)) return -1;
    return SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? 0 : -1;
#else
    return 0;
#endif
}

/* Detect Raspberry Pi only on Linux by reading its device-tree model. */
int plat_is_raspberry_pi(void)
{
#if defined(__linux__)
    char model[256];
    FILE *file = fopen("/proc/device-tree/model", "rb");
    if (file == NULL) return 0;
    size_t count = fread(model, 1, sizeof model - 1, file);
    (void)fclose(file);
    model[count] = '\0';
    return count > 0 && strstr(model, "Raspberry Pi") != NULL;
#else
    return 0;
#endif
}

/* Governor reporting is unavailable until a Linux sysfs probe is configured. */
const char *plat_cpu_governor(void)
{
#if defined(__linux__)
    static char governor[64];
    FILE *file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "rb");
    if (file == NULL) return "unknown";
    if (fgets(governor, sizeof governor, file) == NULL) {
        (void)fclose(file);
        return "unknown";
    }
    (void)fclose(file);
    governor[strcspn(governor, "\r\n")] = '\0';
    return governor;
#else
    return "n/a";
#endif
}

/* Pin the current thread on Linux; other systems report unsupported. */
int plat_pin_to_core(int core)
{
#if defined(__linux__)
    cpu_set_t set;
    if (core < 0 || core >= CPU_SETSIZE) return -1;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    return sched_setaffinity(0, sizeof set, &set);
#elif defined(_WIN32)
    DWORD_PTR mask;
    if (core < 0 || core >= (int)(sizeof(DWORD_PTR) * 8U)) return -1;
    mask = (DWORD_PTR)1U << (unsigned)core;
    return SetThreadAffinityMask(GetCurrentThread(), mask) == 0 ? -1 : 0;
#else
    (void)core;
    return -1;
#endif
}
