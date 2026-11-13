/*
 * LOG: Exp.3 em C++. Mesma tabela hash do C com nos via new; a classe TabelaHash e dona dos nos e dos baldes (delete[] no rehash, destrutor libera tudo - RAII). Imprime tempos e checksum.
 *
 * Experimento 3 - Realocacao Massiva em Tabela Hash (C++, RAII)
 *
 * Uso: exp3_cpp [CHAVES] [CAP_INICIAL]
 *      padrao: 5000000 1024
 *
 * Saida (ultima linha, CSV):
 *   t_insercao,t_liberacao,t_total,checksum
 *
 * Compilar: g++ -O2 -static-libgcc -static-libstdc++ exp3_cpp.cpp -o exp3_cpp.exe
 */
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <chrono>

static double agora() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

struct NoHash {
    uint32_t chave;
    NoHash* proximo;
    NoHash(uint32_t c, NoHash* p) : chave(c), proximo(p) {}
};

class TabelaHash {
public:
    explicit TabelaHash(uint32_t capInicial)
        : baldes(new NoHash*[capInicial]()), capacidade(capInicial), tamanho(0), rehashes(0) {}

    ~TabelaHash() {                       // RAII: destruir o objeto libera tudo
        for (uint32_t i = 0; i < capacidade; i++) {
            NoHash* a = baldes[i];
            while (a) { NoHash* p = a->proximo; delete a; a = p; }
        }
        delete[] baldes;
    }

    TabelaHash(const TabelaHash&) = delete;
    TabelaHash& operator=(const TabelaHash&) = delete;

    void inserir(uint32_t chave) {
        if (static_cast<uint64_t>(tamanho) * 4u > static_cast<uint64_t>(capacidade) * 3u) redimensionar();
        uint32_t idx = chave % capacidade;
        baldes[idx] = new NoHash(chave, baldes[idx]);
        tamanho++;
    }

    uint64_t checksum() const {
        uint64_t h = tamanho;
        h = h * 31u + capacidade;
        h = h * 31u + rehashes;
        for (uint32_t i = 0; i < capacidade; i++)
            for (NoHash* a = baldes[i]; a; a = a->proximo)
                h = h * 31u + a->chave;
        return h;
    }

    uint32_t cap() const { return capacidade; }
    uint32_t numRehashes() const { return rehashes; }

private:
    void redimensionar() {
        uint32_t nova = capacidade * 2u;
        NoHash** novos = new NoHash*[nova]();
        for (uint32_t i = 0; i < capacidade; i++) {
            NoHash* a = baldes[i];
            while (a) {
                NoHash* prox = a->proximo;
                uint32_t idx = a->chave % nova;
                a->proximo = novos[idx];
                novos[idx] = a;
                a = prox;
            }
        }
        delete[] baldes;                  // vetor antigo liberado na hora
        baldes = novos;
        capacidade = nova;
        rehashes++;
    }

    NoHash** baldes;
    uint32_t capacidade;
    uint32_t tamanho;
    uint32_t rehashes;
};

int main(int argc, char** argv) {
    long long CHAVES = argc > 1 ? std::atoll(argv[1]) : 5000000;
    long long CAP0   = argc > 2 ? std::atoll(argv[2]) : 1024;
    if (CHAVES <= 0 || CHAVES > 1000000000LL || CAP0 < 1 || CAP0 > (1LL << 30)) {
        std::fprintf(stderr, "Parametros invalidos\n");
        return 1;
    }

    double t0, t1, t2, t3;
    uint64_t cs;
    uint32_t cap, rh;
    {
        TabelaHash* t = new TabelaHash(static_cast<uint32_t>(CAP0));

        // Fase 1: insercao (com rehashing)
        t0 = agora();
        for (uint32_t i = 0; i < static_cast<uint32_t>(CHAVES); i++)
            t->inserir(i * 2654435761u);
        t1 = agora();

        // Verificacao (fora da contagem de tempo)
        cs = t->checksum();
        cap = t->cap();
        rh = t->numRehashes();

        // Fase 2: liberacao (destrutor)
        t2 = agora();
        delete t;
        t3 = agora();
    }

    double ti = t1 - t0, tl = t3 - t2;
    std::printf("=== Experimento 3 (C++) CHAVES=%lld CAP_INICIAL=%lld CAP_FINAL=%u REHASHES=%u ===\n",
                CHAVES, CAP0, cap, rh);
    std::printf("%.6f,%.6f,%.6f,%016llx\n", ti, tl, ti + tl, static_cast<unsigned long long>(cs));
    return 0;
}
