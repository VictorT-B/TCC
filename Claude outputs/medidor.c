/* LOG: Lanca o benchmark suspenso num Job Object (pega processos filhos, ex. JVM), fixa prioridade/afinidade e mede parede, CPU user/kernel somada e maior pico de RAM entre os processos. */
/*
 * Uso:   medidor.exe <mascara_afinidade_hex> <programa> [args...]
 * Ex.:   medidor.exe 0x2 bin\exp1_c.exe
 *        medidor.exe 0x2 "C:\...\java.exe" -XX:+UseSerialGC -Xms4416m -Xmx4416m -cp bin\java Experimento1
 *
 * Por que Job Object: o launcher do Java pode abrir a JVM como processo filho. Medindo so o
 * processo lancado, o resultado seria o do launcher (~8 MB, CPU ~0). O job agrupa a arvore
 * inteira: a CPU vem da contabilidade do job (soma de todos) e o pico de RAM e o maior
 * PeakWorkingSet entre os processos. Afinidade e prioridade do job valem para os filhos.
 *
 * Ao final, imprime UMA linha:
 *   MEDIDOR;parede_s;cpu_user_s;cpu_kernel_s;pico_ws_kb;page_faults;pico_commit_kb;exit_code;n_processos
 *
 * Compilar: gcc -O2 -o medidor.exe medidor.c -lpsapi
 */
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROC 64

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
    ULONG_PTR mascara = (ULONG_PTR)strtoull(argv[1], NULL, 16);
    if (mascara == 0) { fprintf(stderr, "mascara invalida: %s\n", argv[1]); return 2; }

    static char cmd[32768];
    cmd[0] = '\0';
    for (int i = 2; i < argc; i++) anexa_arg(cmd, sizeof cmd, argv[i]);

    /* Job com afinidade e prioridade (herdadas por todos os processos do job) */
    HANDLE job = CreateJobObjectA(NULL, NULL);
    HANDLE porta = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if (!job || !porta) { fprintf(stderr, "CreateJobObject/IoCompletionPort falhou (%lu)\n", GetLastError()); return 3; }

    JOBOBJECT_BASIC_LIMIT_INFORMATION lim;
    ZeroMemory(&lim, sizeof lim);
    lim.LimitFlags = JOB_OBJECT_LIMIT_AFFINITY | JOB_OBJECT_LIMIT_PRIORITY_CLASS;
    lim.Affinity = mascara;
    lim.PriorityClass = HIGH_PRIORITY_CLASS;
    if (!SetInformationJobObject(job, JobObjectBasicLimitInformation, &lim, sizeof lim)) {
        fprintf(stderr, "SetInformationJobObject (limites) falhou (%lu)\n", GetLastError());
        return 3;
    }
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT acp;
    acp.CompletionKey = job;
    acp.CompletionPort = porta;
    if (!SetInformationJobObject(job, JobObjectAssociateCompletionPortInformation, &acp, sizeof acp)) {
        fprintf(stderr, "SetInformationJobObject (porta) falhou (%lu)\n", GetLastError());
        return 3;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof si); si.cb = sizeof si;
    ZeroMemory(&pi, sizeof pi);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_SUSPENDED | HIGH_PRIORITY_CLASS,
                        NULL, NULL, &si, &pi)) {
        fprintf(stderr, "CreateProcess falhou (%lu): %s\n", GetLastError(), cmd);
        return 3;
    }
    if (!AssignProcessToJobObject(job, pi.hProcess)) {
        fprintf(stderr, "AssignProcessToJobObject falhou (%lu)\n", GetLastError());
        TerminateProcess(pi.hProcess, 4);
        return 4;
    }

    HANDLE procs[MAX_PROC];
    DWORD pids[MAX_PROC];
    int nproc = 0;
    procs[nproc] = pi.hProcess; pids[nproc] = pi.dwProcessId; nproc++;

    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    ResumeThread(pi.hThread);

    /* Segura um handle de cada processo novo (pra ler o pico depois que ele sair)
       e espera o job esvaziar. */
    for (;;) {
        DWORD msg; ULONG_PTR chave; LPOVERLAPPED ov;
        if (!GetQueuedCompletionStatus(porta, &msg, &chave, &ov, INFINITE)) break;
        if (chave != (ULONG_PTR)job) continue;
        if (msg == JOB_OBJECT_MSG_NEW_PROCESS) {
            DWORD pid = (DWORD)(ULONG_PTR)ov;
            int ja = 0;
            for (int i = 0; i < nproc; i++) if (pids[i] == pid) { ja = 1; break; }
            if (!ja && nproc < MAX_PROC) {
                HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
                if (h) { procs[nproc] = h; pids[nproc] = pid; nproc++; }
            }
        } else if (msg == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO) {
            break;
        }
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    QueryPerformanceCounter(&t1);

    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION acc;
    ZeroMemory(&acc, sizeof acc);
    QueryInformationJobObject(job, JobObjectBasicAccountingInformation, &acc, sizeof acc, NULL);

    SIZE_T pico_ws = 0, pico_commit = 0;
    unsigned long long faults = 0;
    for (int i = 0; i < nproc; i++) {
        PROCESS_MEMORY_COUNTERS pmc;
        ZeroMemory(&pmc, sizeof pmc); pmc.cb = sizeof pmc;
        if (GetProcessMemoryInfo(procs[i], &pmc, sizeof pmc)) {
            if (pmc.PeakWorkingSetSize > pico_ws) pico_ws = pmc.PeakWorkingSetSize;
            if (pmc.PeakPagefileUsage > pico_commit) pico_commit = pmc.PeakPagefileUsage;
            faults += pmc.PageFaultCount;
        }
    }

    double parede_s = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
    double user_s = (double)acc.TotalUserTime.QuadPart / 1e7;
    double kernel_s = (double)acc.TotalKernelTime.QuadPart / 1e7;

    fflush(stdout);
    printf("MEDIDOR;%.6f;%.6f;%.6f;%llu;%llu;%llu;%lu;%lu\n",
           parede_s, user_s, kernel_s,
           (unsigned long long)(pico_ws / 1024), faults,
           (unsigned long long)(pico_commit / 1024),
           (unsigned long)exit_code, (unsigned long)acc.TotalProcesses);
    fflush(stdout);

    for (int i = 1; i < nproc; i++) CloseHandle(procs[i]);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(porta);
    CloseHandle(job);
    return 0;
}
