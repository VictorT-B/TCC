# Gerenciamento de memória sob escassez de recursos: C, C++ e Java

Trabalho de Conclusão de Curso — Engenharia de Computação, Universidade de Araraquara.

Compara o desempenho de três paradigmas de gerenciamento de memória — manual
(`malloc`/`free` em C), RAII (C++) e coleta de lixo (Java, Serial GC) — em três
estruturas de dados, sob níveis decrescentes de vCPU e RAM alocados em
máquina virtual.

| Experimento | Estrutura | Operação estressada |
|---|---|---|
| 1 | Lista duplamente encadeada | Fragmentação: remoções aleatórias e reinserção |
| 2 | Grafo (lista de adjacência) | Travessia BFS (*pointer chasing*) |
| 3 | Tabela hash encadeada | Realocação em massa (*rehashing*) |

## Estrutura

```
experimentos/      código dos experimentos, uma pasta por experimento
  exp1_lista/      exp1_c.c, exp1_cpp.cpp, Experimento1.java
  exp2_grafo/      exp2_c.c, exp2_cpp.cpp, Experimento2.java
  exp3_hash/       exp3_c.c, exp3_cpp.cpp, Experimento3.java
launcher/          medidor externo: pico de memória e tempo de CPU do processo
scripts/           automação da coleta e preparação do ambiente (PowerShell)
dados/
  brutos/          CSVs da coleta, um por ambiente
  piloto_2026-09_OBSOLETO/  coleta piloto (GCC 6.3, sem padronização) — não usar para conclusões
analise/           scripts de análise estatística e gráficos
docs/              ambiente de testes e decisões de método
compilar.ps1       compila tudo com as flags fixas
```

## Compilar

Requer GCC/G++ 16.2.0 (MSYS2 mingw-w64) e JDK 25.0.4.1 no `PATH`.

```powershell
.\compilar.ps1
```

Flags fixas: C `-O2 -static-libgcc`; C++ `-O2 -static-libgcc -static-libstdc++`.
As versões usadas ficam registradas em `bin\versoes.txt`.

## Executar

Cada pasta de experimento tem um `LOG.md` descrevendo os arquivos. Argumentos:

| Experimento | Argumentos | Padrão |
|---|---|---|
| 1 | `[N] [REMOVER] [SEED]` | `1000000 500000 115` |
| 2 | `[VERTICES] [ARESTAS] [BUSCAS] [SEED]` | `200000 2000000 10 115` |
| 3 | `[CHAVES] [CAP_INICIAL]` | `5000000 1024` |

```powershell
bin\exp1_c.exe
bin\exp1_cpp.exe
java -XX:+UseSerialGC -Xms<heap> -Xmx<heap> -cp bin\java Experimento1
```

Última linha da saída (CSV):

```
Exp. 1: t_construcao,t_remocao,t_reinsercao,t_liberacao,t_total,checksum
Exp. 2: t_construcao,t_bfs,t_liberacao,t_total,checksum
Exp. 3: t_insercao,t_liberacao,t_total,checksum
```

Tempos em segundos, por relógio monotônico. O checksum confirma que as três
linguagens fizeram exatamente o mesmo trabalho.

| Experimento | Parâmetros padrão | Checksum esperado |
|---|---|---|
| 1 | `1000000 500000 115` | `4a5df0a09d318eb1` |
| 2 | `200000 2000000 10 115` | `f8d7b2fec15dd2cd` |
| 3 | `5000000 1024` | `79292f2b7b9ef235` |

## Método (resumo)

- Mesmo gerador pseudoaleatório nas três linguagens (SplitMix64).
- Mesmas estruturas: nós próprios nas três linguagens, sem `std::list` ou coleções do Java.
- Memória e tempo de CPU medidos de fora do processo, igual para as três linguagens.
- JVM: `-XX:+UseSerialGC`, `-Xms` = `-Xmx` = ~75% da RAM do ambiente, `-Xlog:gc*` em arquivo.
- 5 execuções de aquecimento descartadas + 30 válidas por combinação, em ordem embaralhada.

Detalhes do ambiente em [`docs/ambiente.md`](docs/ambiente.md).
