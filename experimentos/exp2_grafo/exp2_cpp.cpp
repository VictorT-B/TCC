/*
 * LOG: Exp.2 em C++. Mesmo grafo e BFS do C, com nos via new e posse na classe Grafo: o destrutor libera tudo (RAII). Imprime tempo por fase e checksum.
 *
 * Uso: exp2_cpp [VERTICES] [ARESTAS] [BUSCAS] [SEED]
 *      padrao: 200000 2000000 10 115
 *
 * Saida (ultima linha, CSV):
 *   t_construcao,t_bfs,t_liberacao,t_total,checksum
 *
 * Fila e visitado sao vetores simples (int e unsigned char), e nao
 * std::queue / std::vector<bool>, para ficarem iguais ao C e ao Java.
 *
 * Compilar: g++ -O2 -static-libgcc -static-libstdc++ exp2_cpp.cpp -o exp2_cpp.exe
 */
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>

using Relogio = std::chrono::steady_clock;
static double segundos(Relogio::time_point a, Relogio::time_point b) {
    return std::chrono::duration<double>(b - a).count();
}

// ─── PRNG comum as tres linguagens: SplitMix64 ──────────────────────────────
class Prng {
    uint64_t estado;
public:
    explicit Prng(uint64_t seed) : estado(seed) {}
    uint64_t proximo() {
        uint64_t z = (estado += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    int limite(int lim) { return (int)((proximo() >> 33) % (uint64_t)lim); }
};

// ─── Grafo com RAII ─────────────────────────────────────────────────────────
struct Vizinho {
    int destino;
    Vizinho* proximo;
    Vizinho(int d, Vizinho* p) : destino(d), proximo(p) {}
};

class Grafo {
public:
    const int numVertices;
    std::unique_ptr<Vizinho*[]> adj;

    explicit Grafo(int n) : numVertices(n), adj(new Vizinho*[n]()) {}
    Grafo(const Grafo&) = delete;
    Grafo& operator=(const Grafo&) = delete;

    ~Grafo() {                                  // liberacao deterministica
        for (int i = 0; i < numVertices; i++) {
            Vizinho* a = adj[i];
            while (a) { Vizinho* p = a->proximo; delete a; a = p; }
        }
    }                                           // adj[] liberado pelo unique_ptr

    void adicionarAresta(int u, int v) {
        adj[u] = new Vizinho(v, adj[u]);
        adj[v] = new Vizinho(u, adj[v]);
    }

    uint64_t bfs(int origem, uint64_t h) const {
        std::unique_ptr<unsigned char[]> visitado(new unsigned char[numVertices]());
        std::unique_ptr<int[]> fila(new int[numVertices]);
        int ini = 0, fim = 0;

        visitado[origem] = 1;
        fila[fim++] = origem;
        while (ini < fim) {
            int atual = fila[ini++];
            h = h * 31u + (uint64_t)atual;
            for (Vizinho* v = adj[atual]; v; v = v->proximo) {   // pointer chasing
                if (!visitado[v->destino]) {
                    visitado[v->destino] = 1;
                    fila[fim++] = v->destino;
                }
            }
        }
        return h * 31u + (uint64_t)fim;
    }                                           // visitado e fila liberados aqui
};

int main(int argc, char** argv) {
    int V      = argc > 1 ? std::atoi(argv[1]) : 200000;
    int A      = argc > 2 ? std::atoi(argv[2]) : 2000000;
    int BUSCAS = argc > 3 ? std::atoi(argv[3]) : 10;
    long long seed = argc > 4 ? std::atoll(argv[4]) : 115;
    if (V < 2 || A < 0 || BUSCAS < 1) { std::fprintf(stderr, "Parametros invalidos\n"); return 1; }
    Prng prng((uint64_t)seed);

    double tc, tb, tl;
    uint64_t h;
    try {
        // Fase 1: construcao
        auto t0 = Relogio::now();
        auto g = std::make_unique<Grafo>(V);
        long long arestas = 0;
        for (int i = 0; i < A; i++) {
            int u = prng.limite(V);
            int v = prng.limite(V);
            if (u != v) { g->adicionarAresta(u, v); arestas++; }
        }
        auto t1 = Relogio::now();

        // Fase 2: BFS a partir de origens sorteadas
        h = (uint64_t)arestas;
        for (int b = 0; b < BUSCAS; b++) h = g->bfs(prng.limite(V), h);
        auto t2 = Relogio::now();

        // Fase 3: liberacao -> destrutor ~Grafo()
        auto t3 = Relogio::now();
        g.reset();
        auto t4 = Relogio::now();

        tc = segundos(t0, t1); tb = segundos(t1, t2); tl = segundos(t3, t4);
    } catch (const std::bad_alloc&) {
        std::fprintf(stderr, "ERRO: new falhou\n");
        return 2;
    }

    std::printf("=== Experimento 2 (C++) V=%d A=%d BUSCAS=%d SEED=%lld ===\n", V, A, BUSCAS, seed);
    std::printf("%.6f,%.6f,%.6f,%.6f,%016llx\n", tc, tb, tl, tc + tb + tl, (unsigned long long)h);
    return 0;
}
