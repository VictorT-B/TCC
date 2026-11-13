# Experimento 1 — Lista duplamente encadeada

| Arquivo | Log |
|---|---|
| `exp1_c.c` | Exp.1 em C. Cria lista duplamente encadeada (malloc), remove metade dos nós em ordem aleatória com free imediato, reinsere e libera tudo. Imprime tempo por fase e checksum. |
| `exp1_cpp.cpp` | Exp.1 em C++. Mesma lista do C com nós próprios via new; a classe Lista é dona dos nós e o destrutor libera tudo (RAII). Imprime tempo por fase e checksum. |
| `Experimento1.java` | Exp.1 em Java. Mesma lista do C com nós como objetos na heap; removidos ficam inalcançáveis e o GC recolhe quando decidir. Imprime tempo por fase e checksum. |

Argumentos: `[N] [REMOVER] [SEED]` — padrão `1000000 500000 115` → checksum `4a5df0a09d318eb1`
