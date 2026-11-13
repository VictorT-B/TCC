/*
 * LOG: Exp.3 em C. Insere chaves unicas espalhadas numa tabela hash encadeada (malloc), dobra os baldes a cada fator 0,75 com free imediato do vetor antigo e libera tudo. Imprime tempos e checksum.
 *
 * Experimento 3 - Realocacao Massiva em Tabela Hash (C, malloc/free)
 *
 * Uso: exp3_c [CHAVES] [CAP_INICIAL]
 *      padrao: 5000000 1024
 *
 * Chave i = (i * 2654435761) mod 2^32 (sem sinal): todas distintas e
 * espalhadas. Indice = chave % capacidade (sem sinal). Rehash quando
 * tamanho/capacidade > 0,75 (conta inteira: tamanho*4 > capacidade*3).
 *
 * Saida (ultima linha, CSV):
 *   t_insercao,t_liberacao,t_total,checksum
 *
 * Compilar: gcc -O2 -static-libgcc exp3_c.c -o exp3_c.exe
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
#else
#include <time.h>
static double agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
#endif

typedef struct NoHash {
    uint32_t chave;
    struct NoHash* proximo;
} NoHash;

typedef struct {
    NoHash** baldes;
    uint32_t capacidade;
    uint32_t tamanho;
    uint32_t rehashes;
} TabelaHash;

static void* zerado(size_t n, size_t tam) {
    void* p = calloc(n, tam);
    if (!p) { fprintf(stderr, "ERRO: calloc falhou\n"); exit(2); }
    return p;
}

static void redimensionar(TabelaHash* t) {
    uint32_t nova = t->capacidade * 2u;
    NoHash** novos = (NoHash**)zerado(nova, sizeof(NoHash*));
    for (uint32_t i = 0; i < t->capacidade; i++) {
        NoHash* a = t->baldes[i];
        while (a) {
            NoHash* prox = a->proximo;
            uint32_t idx = a->chave % nova;
            a->proximo = novos[idx];
            novos[idx] = a;
            a = prox;
        }
    }
    free(t->baldes);                  /* vetor antigo liberado na hora */
    t->baldes = novos;
    t->capacidade = nova;
    t->rehashes++;
}

static void inserir(TabelaHash* t, uint32_t chave) {
    if ((uint64_t)t->tamanho * 4u > (uint64_t)t->capacidade * 3u) redimensionar(t);
    uint32_t idx = chave % t->capacidade;
    NoHash* novo = (NoHash*)malloc(sizeof(NoHash));
    if (!novo) { fprintf(stderr, "ERRO: malloc falhou\n"); exit(2); }
    novo->chave = chave;
    novo->proximo = t->baldes[idx];
    t->baldes[idx] = novo;
    t->tamanho++;
}

/* h comeca em tamanho, capacidade e rehashes; depois h = h*31 + chave
 * percorrendo baldes e cadeias em ordem (depende da ordem exata das cadeias). */
static uint64_t checksum(const TabelaHash* t) {
    uint64_t h = t->tamanho;
    h = h * 31u + t->capacidade;
    h = h * 31u + t->rehashes;
    for (uint32_t i = 0; i < t->capacidade; i++)
        for (NoHash* a = t->baldes[i]; a; a = a->proximo)
            h = h * 31u + a->chave;
    return h;
}

static void liberar(TabelaHash* t) {
    for (uint32_t i = 0; i < t->capacidade; i++) {
        NoHash* a = t->baldes[i];
        while (a) { NoHash* p = a->proximo; free(a); a = p; }
    }
    free(t->baldes);
    t->baldes = NULL;
    t->capacidade = t->tamanho = 0;
}

int main(int argc, char** argv) {
    long long CHAVES = argc > 1 ? atoll(argv[1]) : 5000000;
    long long CAP0   = argc > 2 ? atoll(argv[2]) : 1024;
    if (CHAVES <= 0 || CHAVES > 1000000000LL || CAP0 < 1 || CAP0 > (1LL << 30)) {
        fprintf(stderr, "Parametros invalidos\n");
        return 1;
    }

    TabelaHash t;
    t.capacidade = (uint32_t)CAP0;
    t.tamanho = 0;
    t.rehashes = 0;
    t.baldes = (NoHash**)zerado(t.capacidade, sizeof(NoHash*));

    /* Fase 1: insercao (com rehashing) */
    double t0 = agora();
    for (uint32_t i = 0; i < (uint32_t)CHAVES; i++)
        inserir(&t, i * 2654435761u);
    double t1 = agora();

    /* Verificacao (fora da contagem de tempo) */
    uint64_t cs = checksum(&t);
    uint32_t cap = t.capacidade, rh = t.rehashes;

    /* Fase 2: liberacao */
    double t2 = agora();
    liberar(&t);
    double t3 = agora();

    double ti = t1 - t0, tl = t3 - t2;
    printf("=== Experimento 3 (C) CHAVES=%lld CAP_INICIAL=%lld CAP_FINAL=%u REHASHES=%u ===\n",
           CHAVES, CAP0, cap, rh);
    printf("%.6f,%.6f,%.6f,%016llx\n", ti, tl, ti + tl, (unsigned long long)cs);
    return 0;
}
