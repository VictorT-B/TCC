/*
 * LOG: Exp.1 em C++. Mesma lista do C com nos proprios via new; a classe Lista e dona dos nos e o destrutor libera tudo (RAII). Imprime tempo por fase e checksum.
 *
 * Experimento 1 - Fragmentacao em Lista Duplamente Encadeada (C++, RAII)
 *
 * Uso: exp1_cpp [N] [REMOVER] [SEED]
 *      padrao: N=1000000, REMOVER=N/2, SEED=115
 *
 * Saida (ultima linha, CSV):
 *   t_construcao,t_remocao,t_reinsercao,t_liberacao,t_total,checksum
 *
 * A lista usa nos proprios (mesmo layout do C e do Java), e nao std::list,
 * para que a unica diferenca entre as linguagens seja o gerenciamento de
 * memoria: aqui a posse dos nos pertence ao objeto Lista e a liberacao
 * acontece no destrutor, ao sair do escopo.
 *
 * Compilar: g++ -O2 -static-libgcc -static-libstdc++ exp1_cpp.cpp -o exp1_cpp.exe
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

// ─── Lista duplamente encadeada com RAII ────────────────────────────────────
struct No {
    int valor;
    No* anterior;
    No* proximo;
    No(int v, No* ant) : valor(v), anterior(ant), proximo(nullptr) {}
};

class Lista {
public:
    No* cabeca = nullptr;
    No* cauda = nullptr;
    int tamanho = 0;

    Lista() = default;
    Lista(const Lista&) = delete;
    Lista& operator=(const Lista&) = delete;

    ~Lista() {                              // liberacao deterministica
        No* atual = cabeca;
        while (atual) {
            No* prox = atual->proximo;
            delete atual;
            atual = prox;
        }
    }

    void inserirFim(int valor) {
        No* novo = new No(valor, cauda);
        if (cauda) cauda->proximo = novo;
        else       cabeca = novo;
        cauda = novo;
        tamanho++;
    }

    void removerNo(No* alvo) {
        if (alvo->anterior) alvo->anterior->proximo = alvo->proximo;
        else                cabeca = alvo->proximo;
        if (alvo->proximo)  alvo->proximo->anterior = alvo->anterior;
        else                cauda = alvo->anterior;
        tamanho--;
        delete alvo;                        // liberacao imediata
    }

    uint64_t checksum() const {
        uint64_t h = (uint64_t)tamanho;
        for (No* a = cabeca; a; a = a->proximo)
            h = h * 31u + (uint64_t)(int64_t)a->valor;
        return h;
    }
};

int main(int argc, char** argv) {
    int N = argc > 1 ? std::atoi(argv[1]) : 1000000;
    int REMOVER = argc > 2 ? std::atoi(argv[2]) : N / 2;
    long long seed = argc > 3 ? std::atoll(argv[3]) : 115;
    if (N <= 0 || REMOVER < 0 || REMOVER > N) {
        std::fprintf(stderr, "Parametros invalidos\n");
        return 1;
    }
    Prng prng((uint64_t)seed);
    double tc, tr, ti, tl;
    uint64_t cs;

    try {
        // unique_ptr so para poder medir o destrutor isoladamente (reset()).
        auto lista = std::make_unique<Lista>();

        // Fase 1: construcao
        auto t0 = Relogio::now();
        for (int i = 0; i < N; i++) lista->inserirFim(i);
        auto t1 = Relogio::now();

        // Fase 2: remocao em posicoes pseudoaleatorias (Fisher-Yates)
        {
            std::unique_ptr<No*[]> ponteiros(new No*[N]);
            No* a = lista->cabeca;
            for (int i = 0; i < N; i++) { ponteiros[i] = a; a = a->proximo; }
            for (int i = N - 1; i > 0; i--) {
                int j = prng.limite(i + 1);
                No* tmp = ponteiros[i]; ponteiros[i] = ponteiros[j]; ponteiros[j] = tmp;
            }
            for (int i = 0; i < REMOVER; i++) lista->removerNo(ponteiros[i]);
        }   // array de ponteiros liberado aqui (RAII)
        auto t2 = Relogio::now();

        // Fase 3: reinsercao
        for (int i = 0; i < REMOVER; i++) lista->inserirFim(i);
        auto t3 = Relogio::now();

        // Verificacao (fora da contagem de tempo)
        cs = lista->checksum();

        // Fase 4: liberacao -> destrutor ~Lista()
        auto t3b = Relogio::now();
        lista.reset();
        auto t4 = Relogio::now();

        tc = segundos(t0, t1); tr = segundos(t1, t2);
        ti = segundos(t2, t3); tl = segundos(t3b, t4);
    } catch (const std::bad_alloc&) {
        std::fprintf(stderr, "ERRO: new falhou\n");
        return 2;
    }

    std::printf("=== Experimento 1 (C++) N=%d REMOVER=%d SEED=%lld ===\n", N, REMOVER, seed);
    std::printf("%.6f,%.6f,%.6f,%.6f,%.6f,%016llx\n",
                tc, tr, ti, tl, tc + tr + ti + tl, (unsigned long long)cs);
    return 0;
}
