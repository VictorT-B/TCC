# Experimento 2 — Grafo e BFS

| Arquivo | Log |
|---|---|
| `exp2_c.c` | Exp.2 em C. Gera grafo aleatório (lista de adjacência com nós via malloc), roda BFS a partir de origens sorteadas e libera com free. Imprime tempo por fase e checksum. |
| `exp2_cpp.cpp` | Exp.2 em C++. Mesmo grafo e BFS do C, com nós via new e posse na classe Grafo: o destrutor libera tudo (RAII). Imprime tempo por fase e checksum. |
| `Experimento2.java` | Exp.2 em Java. Mesmo grafo e BFS do C, com nós como objetos na heap. Fila int[] sem autoboxing. Liberação só solta a referência; o GC recolhe. Imprime tempos e checksum. |

Argumentos: `[VERTICES] [ARESTAS] [BUSCAS] [SEED]` — padrão `200000 2000000 10 115` → checksum `f8d7b2fec15dd2cd`
