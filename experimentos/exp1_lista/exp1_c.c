/*
 * LOG: Exp.1 em C. Cria lista duplamente encadeada (malloc), remove metade dos nos em ordem aleatoria com free imediato, reinsere e libera tudo. Imprime tempo por fase e checksum.
 *
 * Experimento 1 - Fragmentacao em Lista Duplamente Encadeada (C, malloc/free)
 *
 * Uso: exp1_c [N] [REMOVER] [SEED]
 *      padrao: N=1000000, REMOVER=N/2, SEED=115
 *
 * Saida (ultima linha, CSV):
 *   t_construcao,t_remocao,t_reinsercao,t_liberacao,t_total,checksum
 * Tempos em segundos (relogio monotonico). Memoria e tempo de CPU
 * sao medidos de fora pelo launcher, igual para as tres linguagens.
 *
 * Compilar: gcc -O2 -static-libgcc exp1_c.c -o exp1_c.exe
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
static double agora(void) {
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER t;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
}
#else // para linux, macOS, etc.
#include <time.h>
static double agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
#endif

/* ─── PRNG comum as tres linguagens: SplitMix64 ─────────────────────────── */
static uint64_t prng_estado;

static uint64_t prng_proximo(void) {
    uint64_t z = (prng_estado += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* inteiro em [0, limite) usando os 31 bits altos */
static int prng_limite(int limite) {
    return (int)((prng_proximo() >> 33) % (uint64_t)limite);
}

/* ─── Lista duplamente encadeada ────────────────────────────────────────── */
typedef struct No {
    int valor;
    struct No* anterior;
    struct No* proximo;
} No;

typedef struct {
    No* cabeca;
    No* cauda;
    int tamanho;
} Lista;

static void inserir_fim(Lista* l, int valor) {
    No* novo = (No*)malloc(sizeof(No));
    if (!novo) { fprintf(stderr, "ERRO: malloc falhou\n"); exit(2); }
    novo->valor = valor;
    novo->proximo = NULL;
    novo->anterior = l->cauda;
    if (l->cauda) l->cauda->proximo = novo;
    else          l->cabeca = novo;
    l->cauda = novo;
    l->tamanho++;
}

static void remover_no(Lista* l, No* alvo) {
    if (alvo->anterior) alvo->anterior->proximo = alvo->proximo;
    else                l->cabeca = alvo->proximo;
    if (alvo->proximo)  alvo->proximo->anterior = alvo->anterior;
    else                l->cauda = alvo->anterior;
    l->tamanho--;
    free(alvo);                       /* liberacao imediata */
}

static void liberar_lista(Lista* l) {
    No* atual = l->cabeca;
    while (atual) {
        No* prox = atual->proximo;
        free(atual);
        atual = prox;
    }
    l->cabeca = l->cauda = NULL;
    l->tamanho = 0;
}

/* Checksum da ordem final: h = h*31 + valor (mod 2^64), comeca no tamanho */
static uint64_t checksum(const Lista* l) {
    uint64_t h = (uint64_t)l->tamanho;
    for (No* a = l->cabeca; a; a = a->proximo)
        h = h * 31u + (uint64_t)(int64_t)a->valor;
    return h;
}

int main(int argc, char** argv) {
    int  N = argc > 1 ? atoi(argv[1]) : 1000000;
    int  REMOVER = argc > 2 ? atoi(argv[2]) : N / 2;
    long long seed = argc > 3 ? atoll(argv[3]) : 115;
    if (N <= 0 || REMOVER < 0 || REMOVER > N) {
        fprintf(stderr, "Parametros invalidos\n");
        return 1;
    }
    prng_estado = (uint64_t)seed;

    Lista lista = { NULL, NULL, 0 };
    double t0, t1, t2, t3, t4;

    /* Fase 1: construcao */
    t0 = agora();
    for (int i = 0; i < N; i++) inserir_fim(&lista, i);
    t1 = agora();

    /* Fase 2: remocao em posicoes pseudoaleatorias (Fisher-Yates) */
    No** ponteiros = (No**)malloc((size_t)N * sizeof(No*));
    if (!ponteiros) { fprintf(stderr, "ERRO: malloc falhou\n"); return 2; }
    {
        No* a = lista.cabeca;
        for (int i = 0; i < N; i++) { ponteiros[i] = a; a = a->proximo; }
    }
    for (int i = N - 1; i > 0; i--) {
        int j = prng_limite(i + 1);
        No* tmp = ponteiros[i]; ponteiros[i] = ponteiros[j]; ponteiros[j] = tmp;
    }
    for (int i = 0; i < REMOVER; i++) remover_no(&lista, ponteiros[i]);
    free(ponteiros);
    t2 = agora();

    /* Fase 3: reinsercao */
    for (int i = 0; i < REMOVER; i++) inserir_fim(&lista, i);
    t3 = agora();

    /* Verificacao (fora da contagem de tempo) */
    uint64_t cs = checksum(&lista);

    /* Fase 4: liberacao */
    double t3b = agora();
    liberar_lista(&lista);
    t4 = agora();

    double tc = t1 - t0, tr = t2 - t1, ti = t3 - t2, tl = t4 - t3b;
    printf("=== Experimento 1 (C) N=%d REMOVER=%d SEED=%lld ===\n", N, REMOVER, seed);
    printf("%.6f,%.6f,%.6f,%.6f,%.6f,%016llx\n",
           tc, tr, ti, tl, tc + tr + ti + tl, (unsigned long long)cs);
    return 0;
}
