# Resumo + ajustes da Introdução e Hipóteses — pronto para colar (13/11/2026)

## 1. RESUMO (novo — inserir antes da introdução)

**Resumo:** A abundância de recursos computacionais tem mascarado o custo das abstrações das linguagens modernas, custo que ressurge em sistemas embarcados, IoT e hardware legado. Este trabalho quantifica o impacto do paradigma de gerenciamento de memória — manual (C), determinístico via RAII (C++) e automático via Garbage Collection (Java, coletor Serial) — no desempenho de três estruturas de dados (lista duplamente encadeada, grafo com busca em largura e tabela hash com rehashing) sob cinco níveis de disponibilidade de hardware, do hospedeiro nativo a uma máquina virtual com 1 vCPU e 1,25 GB de RAM. Foram coletadas 1.350 observações válidas, com equivalência das implementações verificada por checksum, medição externa uniforme de tempo de CPU e pico de memória, e análise por estatística não paramétrica. Contrariando a hipótese inicial, o Java obteve as menores medianas de tempo em todos os cenários — favorecido pela alocação contígua em heap pré-dimensionado, que eliminou coletas de lixo durante os experimentos — ao custo de maior consumo de memória física. A degradação sob restrição foi aproximadamente linear para os três paradigmas, dominada pelo custo da virtualização; a escassez severa de RAM, porém, multiplicou por até 50 vezes a dispersão dos tempos do Java, enquanto C e C++ mantiveram comportamento determinístico. Conclui-se que o custo do gerenciamento automático, nas condições testadas, manifesta-se em memória e em previsibilidade, e não na tendência central do tempo de execução.

**Palavras-chave:** estruturas de dados; gerenciamento de memória; Garbage Collection; RAII; benchmark; virtualização.

> Ajuste fino: se o template da UNIARA limitar o resumo a 150–250 palavras, este tem ~230. Abstract em inglês: traduzir este mesmo texto.

## 2. INTRODUÇÃO — substituições pontuais

**(a) Objetivo específico "Configurar" — substituir o item inteiro por:**

- **Configurar** ambientes de execução parametrizados em cinco níveis de disponibilidade de hardware: execução nativa no hospedeiro (alto desempenho) e quatro configurações de máquina virtual que reduzem RAM e vCPU progressivamente à metade, até um cenário de severa restrição (1 vCPU, 1,25 GB de RAM) representativo de hardwares legados e sistemas embarcados.

**(b) Objetivo específico "Mensurar" — substituir o item inteiro por:**

- **Mensurar**, por instrumentação externa uniforme em nível de sistema operacional, o tempo de parede e o tempo de CPU (GetProcessTimes) e o pico de memória física residente (Peak Working Set) de cada processo, complementados pelo tempo do trecho algorítmico cronometrado internamente por relógio monotônico e pelos registros do coletor de lixo da JVM (-Xlog:gc).

**(c) Questão de pesquisa — manter como está** (ela pergunta a magnitude do impacto; a resposta encontrada, ainda que contrária à expectativa, responde à pergunta).

## 3. HIPÓTESES — substituir integralmente por:

- **Hipótese Principal (H1):**

A limitação progressiva de recursos computacionais (CPU e RAM) provocará, na linguagem executada sobre máquina virtual com coleta de lixo (Java), degradação de desempenho de caráter superlinear, originada pelas pausas de coleta (Stop-the-World) e pela pressão sobre a memória heap; em contrapartida, sob as mesmas restrições, o gerenciamento manual em C e o controle determinístico via RAII em C++ manterão degradação linear e previsível. A hipótese será testada pelo ajuste das medianas de tempo em função do nível de restrição, comparando-se um modelo linear e um exponencial pelo Critério de Informação de Akaike (AIC), e pela participação das pausas registradas nos logs do coletor no acréscimo total de tempo.

- **Hipótese Secundária (H2):**

Estruturas que exigem acesso não sequencial massivo (travessia de grafos) ou realocação de blocos contíguos (rehashing de tabelas hash) atuarão como os principais gatilhos de falhas de memória cache, expondo ineficiência das linguagens gerenciadas no nível do hardware físico em comparação às abstrações de custo zero do C++. Dada a indisponibilidade dos contadores de desempenho da CPU no ambiente virtualizado, a hipótese será avaliada de forma indireta, pelo tempo das fases de travessia e redimensionamento e pelo número de faltas de página por processo.

> Nota de coerência (para a defesa): a redação acima troca "exponencial... fragmentação da Heap" por "superlinear... pausas e pressão sobre a heap", porque o coletor Serial compacta a heap — fragmentação não é o mecanismo correto. O resultado (H1 refutada; degradação linear nas três linguagens, sem nenhuma coleta de lixo registrada) é reportado nas seções de Resultados e Discussão. Hipótese refutada com método sólido é resultado científico legítimo — não reescreva a H1 para "acertar": a banca pediu hipóteses testáveis, e é isso que esta redação entrega.

## 4. Frases de ligação úteis (opcional)

- Fim da introdução: "Os resultados, antecipando a discussão, contrariam a expectativa inicial: nas condições testadas, o custo do gerenciamento automático manifestou-se no consumo de memória e na previsibilidade do tempo de execução, e não na sua tendência central."
- Metodologia → Resultados: "Concluída a coleta nos cinco ambientes, as 1.350 observações válidas foram submetidas ao protocolo estatístico descrito, cujos resultados a seção seguinte apresenta por experimento."
