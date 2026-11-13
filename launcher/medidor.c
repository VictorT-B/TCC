/* LOG: Lanca o benchmark suspenso, fixa prioridade alta e afinidade, mede tempo de parede, CPU user/kernel (GetProcessTimes) e pico de RAM (PeakWorkingSet) igual nas 3 linguagens. */
/*
 * Uso:   medidor.exe <mascara_afinidade_hex> <programa> [args...]
 * Ex.:   medidor.exe 0x2 bin\exp1_c.exe
 *        medidor.exe 0x2 java -XX:+UseSerialGC -Xms4416m -Xmx4416m -cp bin Exp1
 *
 * O benchmark herda stdout e imprime a propria linha (trecho cronometrado + checksum).
 * Ao final, o medidor imprime UMA linha:
 *   MEDIDOR;parede_s;cpu_user_s;cpu_kernel_s;pico_ws_kb;page_faults;pico_commit_kb;exit_code
 *
 * Compilar: gcc -O2 -o medidor.exe medidor.c -lpsapi
 */
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double ft_seg(FILETIME ft) {
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return (double)u.QuadPart / 1e7; /* unidades de 100 ns */
}

/* Acrescenta um argumento a linha de comando, com aspas quando necessario. */
static void anexa_arg(char *dst, size_t cap, const char *arg) {
    size_t n = strlen(dst);
    int aspas = (strchr(arg, ' ') != NULL || strchr(arg, '\t') != NULL || arg[0] == '\0');
    if (n > 0 && n + 1 < cap) { dst[n++] = ' '; dst[n] = '\0'; }
    if (aspas && n + 1 < cap) { dst[n++] = '"'; dst[n] = '\0'; }
    strncat(dst, arg, cap - strlen(dst) - 1);
    n = strlen(dst);
    if (aspas && n + 1 < cap) { dst[n++] = '"'; dst[n] = '\0'; }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "uso: %s <mascara_afinidade_hex> <programa> [args...]\n", argv[0]);
        return 2;
    }

    DWORD_PTR mascara = (DWORD_PTR)strtoull(argv[1], NULL, 16);
    if (mascara == 0) {
        fprintf(stderr, "mascara de afinidade invalida: %s\n", argv[1]);
        return 2;
    }

    static char cmd[32768];
    cmd[0] = '\0';
    for (int i = 2; i < argc; i++) anexa_arg(cmd, sizeof cmd, argv[i]);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    ZeroMemory(&pi, sizeof pi);

    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);

    /* Criado suspenso: prioridade e afinidade valem desde a primeira instrucao do benchmark. */
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE,
                        CREATE_SUSPENDED | HIGH_PRIORITY_CLASS,
                        NULL, NULL, &si, &pi)) {
        fprintf(stderr, "CreateProcess falhou (%lu): %s\n", GetLastError(), cmd);
        return 3;
    }

    if (!SetProcessAffinityMask(pi.hProcess, mascara)) {
        fprintf(stderr, "SetProcessAffinityMask falhou (%lu)\n", GetLastError());
        TerminateProcess(pi.hProcess, 4);
        return 4;
    }

    QueryPerformanceCounter(&t0);
    ResumeThread(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    QueryPerformanceCounter(&t1);

    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    FILETIME c, e, k, u;
    double user_s = -1, kernel_s = -1;
    if (GetProcessTimes(pi.hProcess, &c, &e, &k, &u)) {
        user_s = ft_seg(u);
        kernel_s = ft_seg(k);
    }

    PROCESS_MEMORY_COUNTERS pmc;
    ZeroMemory(&pmc, sizeof pmc);
    pmc.cb = sizeof pmc;
    GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof pmc);

    double parede_s = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

    fflush(stdout);
    printf("MEDIDOR;%.6f;%.6f;%.6f;%llu;%lu;%llu;%lu\n",
           parede_s, user_s, kernel_s,
           (unsigned long long)(pmc.PeakWorkingSetSize / 1024),
           (unsigned long)pmc.PageFaultCount,
           (unsigned long long)(pmc.PeakPagefileUsage / 1024),
           (unsigned long)exit_code);
    fflush(stdout);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
