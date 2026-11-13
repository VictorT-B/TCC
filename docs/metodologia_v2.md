# Metodologia (versão pós-pré-banca) — rascunho para o artigo

> Texto para substituir as seções de metodologia e de arquitetura experimental do `tccMainDoc`. Os trechos marcados com **[conferir]** dependem de confirmação.

## Ambientes de execução

Os experimentos foram executados em uma única máquina física: Intel Core i7-9700F (8 núcleos e 8 threads, sem SMT; 3,0 GHz base; L3 de 12 MB), com 16 GB de RAM DDR4-2400 e Windows 10 22H2. Para variar os recursos de forma controlada, usou-se uma máquina virtual no Oracle VirtualBox 7.2.18, também com Windows 10, configurada em cinco níveis. O primeiro nível é o próprio hospedeiro. Nos níveis seguintes, a VM começa com quase todos os recursos e perde metade da RAM e das vCPUs a cada passo (Quadro X).

**Quadro X – Níveis de ambiente**

| Ambiente | RAM | vCPU | Heap Java (-Xms = -Xmx) |
|---|---|---|---|
| Host | 16 GB | 8 | 12.224 MB |
| VM_100 | 11.776 MB | 7 | 8.832 MB |
| VM_50 | 5.888 MB | 4 | 4.416 MB |
| VM_25 | 2.816 MB | 2 | 2.112 MB |
| VM_12 | 1.280 MB | 1 | 960 MB |

O nível VM_100 recebeu 7 vCPUs, e não 8, para que o hospedeiro e o processo do VirtualBox mantenham um núcleo livre. Todos os níveis de VM partem do mesmo snapshot, tirado com a máquina desligada. O snapshot já contém o JDK, os binários e o ambiente otimizado. A cada nível, apenas a RAM e o número de vCPUs são alterados (`VBoxManage modifyvm`). Assim, todo nível começa de um boot limpo e com o mesmo estado de disco. O nível VM_12 fica abaixo do mínimo oficial do Windows 10 64 bits (2 GB). Ele foi mantido por representar a restrição severa que o trabalho investiga, e se mostrou funcional em teste prévio.

Antes de cada bateria, o script `otimizacao_ambiente.ps1` ativa o plano de energia de alto desempenho com o processador fixo em 100% e desativa os serviços Windows Search, SysMain, Windows Update e Telemetria. Também exclui a pasta do experimento da verificação em tempo real do Windows Defender. Esse procedimento foi aplicado ao hospedeiro e ao convidado.

## Ferramentas e compilação

C e C++ foram compilados com o GCC/G++ 16.2.0 (MSYS2 mingw-w64), com `-O2` e bibliotecas de runtime estáticas (`-static-libgcc` e, no C++, `-static-libstdc++`). O Java foi executado no Oracle JDK 25.0.4.1, a mesma versão no hospedeiro e na VM. A JVM foi configurada com o coletor Serial (`-XX:+UseSerialGC`), para impedir que ela escolhesse o coletor de acordo com o hardware detectado. O heap foi fixado em cerca de 75% da RAM de cada ambiente, com `-Xms` igual a `-Xmx`, e os eventos de coleta foram registrados com `-Xlog:gc`. O script `compilar.ps1` registra as versões dos compiladores em cada compilação.

## Equivalência das implementações

As três linguagens usam o mesmo gerador pseudoaleatório, SplitMix64, com semente fixa (115). As estruturas foram escritas com nós próprios, sem `std::list` nem coleções da biblioteca Java, para que todas façam as mesmas alocações. Cada programa calcula um checksum de 64 bits sobre o estado final da estrutura, fora do trecho cronometrado. O script de coleta confere esse checksum em todas as execuções. Os valores esperados são idênticos nas três linguagens: `4a5df0a09d318eb1` (Exp. 1), `f8d7b2fec15dd2cd` (Exp. 2) e `79292f2b7b9ef235` (Exp. 3).

- **Experimento 1 (lista duplamente encadeada):** insere 1.000.000 de nós, remove 500.000 em ordem embaralhada (Fisher-Yates) e reinsere 500.000. Em C, cada nó removido é liberado imediatamente com `free`. Em C++, a classe da lista é dona dos nós e o destrutor libera tudo (RAII). Em Java, o nó removido apenas fica inalcançável.
- **Experimento 2 (grafo e BFS):** cria um grafo de 200.000 vértices a partir de 2.000.000 de sorteios de arestas, representado por lista de adjacência encadeada. Em seguida, executa 10 buscas em largura a partir de origens sorteadas. A fila e o vetor de visitados são vetores primitivos nas três linguagens, o que elimina o autoboxing de `ArrayDeque<Integer>` usado na versão anterior.
- **Experimento 3 (tabela hash):** insere 5.000.000 de chaves distintas, `chave = i × 2654435761 mod 2³²`, em uma tabela encadeada que começa com 1.024 baldes. A tabela dobra de tamanho sempre que o fator de carga passa de 0,75, o que gera 13 rehashes até 8.388.608 baldes. As chaves e os índices são tratados como inteiros sem sinal de 32 bits nas três linguagens.

## Medição

A memória e o tempo de CPU passaram a ser medidos fora do processo, pelo mesmo programa nas três linguagens: o `medidor.exe`, escrito em C. Ele cria o processo do benchmark suspenso, aplica prioridade alta e afinidade a um único núcleo (núcleo 1; no nível com 1 vCPU, núcleo 0) e só então o libera. Ao final, registra:

- o tempo de parede do processo, por `QueryPerformanceCounter`;
- o tempo de CPU em modo usuário e em modo núcleo, por `GetProcessTimes`;
- o pico de memória física (`PeakWorkingSetSize`), o pico de memória comprometida e o número de faltas de página, por `GetProcessMemoryInfo`.

Dentro de cada programa, apenas o trecho de interesse é cronometrado, por `QueryPerformanceCounter` (C), `steady_clock` (C++) ou `System.nanoTime` (Java), separado por fase. No Java, o processo medido é o `java.exe` do JDK, e não o atalho `javapath` instalado pela Oracle, que abre a JVM em outro processo. A métrica principal de memória é o pico de memória física. O pico de memória comprometida do Java reflete apenas a reserva feita pelo `-Xms`, e não o uso real.

## Protocolo de execução

Para cada ambiente, as 9 combinações de linguagem e experimento foram executadas 3 vezes como aquecimento, com resultados descartados, e depois 30 vezes como execuções válidas. As execuções de aquecimento e as válidas foram embaralhadas separadamente, com semente fixa, para que deriva térmica ou atividade residual do sistema não se concentrasse em uma linguagem. Cada execução é um processo novo. Entre execuções há uma pausa de 500 ms. No total, são 5 ambientes × 9 combinações × 33 execuções, o que dá 1.485 processos, dos quais 1.350 são observações válidas.

## Análise estatística

Os tempos de benchmark costumam ter distribuição assimétrica e valores extremos. Por isso, cada combinação é resumida por mediana e intervalo interquartil, com intervalo de confiança de 95% para a mediana obtido por bootstrap. As linguagens são comparadas dentro de cada ambiente pelo teste de Kruskal-Wallis. Quando há diferença, aplica-se Mann-Whitney par a par com correção de Holm, e o tamanho do efeito é medido pelo delta de Cliff.

- **H1:** para cada linguagem e experimento, a mediana do tempo é modelada em função do nível de restrição k. Um ajuste linear e um exponencial são comparados pelo critério de informação de Akaike (AIC). Para o Java, a soma das pausas registradas no log de GC mostra quanto do acréscimo de tempo se deve à coleta de lixo.
- **H2:** o VirtualBox não expõe os contadores de hardware (PMU) ao sistema convidado, e não houve ferramenta de contagem de falhas de cache disponível no hospedeiro. Por isso, a H2 é avaliada de forma indireta, pelo tempo da fase de BFS e do rehash e pelas faltas de página. Essa limitação é registrada adiante.

## Limitações

- Não há medição direta de falhas de cache. A H2 é inferida a partir do tempo e das faltas de página.
- Foi usado um único hardware físico e um único hipervisor. A restrição é simulada por virtualização, e não por hardware embarcado real.
- No nível VM_12, a RAM fica abaixo do mínimo do Windows 10 e a folga fora do heap Java é pequena. Parte da degradação pode vir do sistema convidado, e não apenas do programa.
- O pico de memória física inclui o runtime de cada linguagem (a JVM inteira, no caso do Java). Isso faz parte do custo real do paradigma, mas impede comparar só as estruturas.
- A coleta piloto de setembro de 2026 (540 observações, GCC 6.3, sem padronização) foi descartada e não é usada nas conclusões.
