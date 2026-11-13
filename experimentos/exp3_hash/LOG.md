# Experimento 3 — Tabela hash com rehashing

| Arquivo | Log |
|---|---|
| `exp3_c.c` | Exp.3 em C. Insere chaves unicas espalhadas numa tabela hash encadeada (malloc), dobra os baldes a cada fator 0,75 com free imediato do vetor antigo e libera tudo. Imprime tempos e checksum. |
| `exp3_cpp.cpp` | Exp.3 em C++. Mesma tabela hash do C com nos via new; a classe TabelaHash e dona dos nos e dos baldes (delete[] no rehash, destrutor libera tudo - RAII). Imprime tempos e checksum. |
| `Experimento3.java` | Exp.3 em Java. Mesma tabela hash do C com nos como objetos na heap; no rehash o vetor antigo vira lixo e o GC recolhe quando decidir. Imprime tempos e checksum. |

Argumentos: `[CHAVES] [CAP_INICIAL]` — padrao `5000000 1024`
