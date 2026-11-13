/*
 * LOG: Exp.2 em C. Gera grafo aleatorio (lista de adjacencia com nos via malloc), roda BFS a partir de origens sorteadas e libera com free. Imprime tempo por fase e checksum.
 *
 * Uso: exp2_c [VERTICES] [ARESTAS] [BUSCAS] [SEED]
 *      padrao: 200000 2000000 10 115
 *
 * Saida (ultima linha, CSV):
 *   t_construcao,t_bfs,t_liberacao,t_total,checksum
 *
 * Compilar: gcc -O2 -static-libgcc exp2_c.c -o exp2_c.exe
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

/* ─── PRNG comum as tres linguagens: SplitMix64 ─────────────────────────── */
static uint64_t prng_estado;

static uint64_t prng_proximo(void) {
    uint64_t z = (prng_estado += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static int prng_limite(int limite) {
    return (int)((prng_proximo() >> 33) % (uint64_t)limite);
}

static void* alocar(size_t n) {
    void* p = malloc(n);
    if (!p) { fprintf(stderr, "ERRO: malloc falhou\n"); exit(2); }
    return p;
}

/* ─── Grafo: lista de adjacencia com nos encadeados ─────────────────────── */
typedef struct Vizinho {
    int destino;
    struct Vizinho* proximo;
} Vizinho;

typedef struct {
    int num_vertices;
    Vizinho** adj;
} Grafo;

static Grafo* criar_grafo(int n) {
    Grafo* g = (Grafo*)alocar(sizeof(Grafo));
    g->num_vertices = n;
    g->adj = (Vizinho**)calloc((size_t)n, sizeof(Vizinho*));
    if (!g->adj) { fprintf(stderr, "ERRO: calloc falhou\n"); exit(2); }
    return g;
}

static void adicionar_aresta(Grafo* g, int u, int v) {
    Vizinho* a = (Vizinho*)alocar(sizeof(Vizinho));
    a->destino = v; a->proximo = g->adj[u]; g->adj[u] = a;
    Vizinho* b = (Vizinho*)alocar(sizeof(Vizinho));
    b->destino = u; b->proximo = g->adj[v]; g->adj[v] = b;
}

/* BFS: fila em vetor de int, visitado em vetor de bytes.
 * Ambos alocados e liberados a cada busca (igual nas tres linguagens).
 * Atualiza o checksum com a ordem de visita. */
static uint64_t bfs(const Grafo* g, int origem, uint64_t h) {
    unsigned char* visitado = (unsigned char*)calloc((size_t)g->num_vertices, 1);
    int* fila = (int*)alocar((size_t)g->num_vertices * sizeof(int));
    if (!visitado) { fprintf(stderr, "ERRO: calloc falhou\n"); exit(2); }
    int ini = 0, fim = 0;

    visitado[origem] = 1;
    fila[fim++] = origem;
    while (ini < fim) {
        int atual = fila[ini++];
        h = h * 31u + (uint64_t)atual;
        for (Vizinho* v = g->adj[atual]; v; v = v->proximo) {   /* pointer chasing */
            if (!visitado[v->destino]) {
                visitado[v->destino] = 1;
                fila[fim++] = v->destino;
            }
        }
    }
    h = h * 31u + (uint64_t)fim;   /* vertices alcancados */
    free(visitado);
    free(fila);
    return h;
}

static void liberar_grafo(Grafo* g) {
    for (int i = 0; i < g->num_vertices; i++) {
        Vizinho* a = g->adj[i];
        while (a) { Vizinho* p = a->proximo; free(a); a = p; }
    }
    free(g->adj);
    free(g);
}

int main(int argc, char** argv) {
    int V      = argc > 1 ? atoi(argv[1]) : 200000;
    int A      = argc > 2 ? atoi(argv[2]) : 2000000;
    int BUSCAS = argc > 3 ? atoi(argv[3]) : 10;
    long long seed = argc > 4 ? atoll(argv[4]) : 115;
    if (V < 2 || A < 0 || BUSCAS < 1) { fprintf(stderr, "Parametros invalidos\n"); return 1; }
    prng_estado = (uint64_t)seed;

    double t0, t1, t2, t3, t4;
    long long arestas = 0;

    /* Fase 1: construcao do grafo */
    t0 = agora();
    Grafo* g = criar_grafo(V);
    for (int i = 0; i < A; i++) {
        int u = prng_limite(V);
        int v = prng_limite(V);
        if (u != v) { adicionar_aresta(g, u, v); arestas++; }
    }
    t1 = agora();

    /* Fase 2: BFS a partir de origens sorteadas */
    uint64_t h = (uint64_t)arestas;
    for (int b = 0; b < BUSCAS; b++) h = bfs(g, prng_limite(V), h);
    t2 = agora();

    /* Fase 3: liberacao */
    t3 = agora();
    liberar_grafo(g);
    t4 = agora();

    double tc = t1 - t0, tb = t2 - t1, tl = t4 - t3;
    printf("=== Experimento 2 (C) V=%d A=%d BUSCAS=%d SEED=%lld ===\n", V, A, BUSCAS, seed);
    printf("%.6f,%.6f,%.6f,%.6f,%016llx\n", tc, tb, tl, tc + tb + tl, (unsigned long long)h);
    return 0;
}
