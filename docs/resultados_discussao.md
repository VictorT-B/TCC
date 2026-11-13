# Resultados e Discussão — rascunho para o artigo (dados de 13/11/2026)

> Para colar após a metodologia. Referências às figuras: exp1_tempo.png, exp2_tempo.png, exp3_tempo.png (tempo) e exp1–3_memoria.png (memória), geradas por `analise/analisar.py`. Tabelas completas em `analise/saida/resumo.md`.

## 4. RESULTADOS

A bateria completa produziu 1.485 execuções (5 ambientes × 9 combinações × 33 repetições), das quais 1.350 são observações válidas após o descarte das 3 execuções de aquecimento por combinação. Todas as 1.350 execuções terminaram com código de saída zero e com o checksum esperado, o que confirma a equivalência lógica das implementações nas três linguagens ao longo de toda a coleta. Os resultados a seguir reportam a mediana do tempo do trecho cronometrado e, entre colchetes, o intervalo interquartil (IQR).

### 4.1 Experimento 1 — Lista duplamente encadeada

No hospedeiro, o Java completou o ciclo de inserção, remoção e reinserção em 0,080 s [0,001], contra 0,146 s [0,001] do C e 0,149 s [0,002] do C++. A vantagem do Java manteve-se em todos os níveis de VM, encolhendo apenas no nível mais restrito: no VM_12, as medianas praticamente convergem (Java 0,176 s; C 0,178 s; C++ 0,180 s), mas o IQR do Java salta para 0,208 s — cinquenta vezes o do C (0,004 s). O pico de memória física do Java foi consistentemente maior (99,0 MB no hospedeiro, contra 41,1 MB do C), diminuindo com o tamanho do heap configurado (76,7 MB no VM_12).

### 4.2 Experimento 2 — Travessia de grafo (BFS)

O padrão repete-se na travessia por pointer chasing: no hospedeiro, Java 2,91 s [0,007], C++ 3,42 s [0,006] e C 3,49 s [0,011]. A transição do hospedeiro para a virtualização impôs o maior degrau da série — cerca de 40 a 45% de acréscimo para as três linguagens (C: 3,49 s → 5,01 s no VM_100). Entre os níveis de VM, as medianas formam um platô (C: 5,01; 4,86; 4,73; 4,77 s), sem tendência de piora com a redução de recursos. No VM_12, o IQR do Java chega a 1,15 s, contra 0,09 s do C.

### 4.3 Experimento 3 — Tabela hash com rehashing

A maior diferença entre paradigmas apareceu aqui: no hospedeiro, o Java inseriu as 5 milhões de chaves (13 rehashes) em 0,61 s [0,003], 2,5 vezes mais rápido que o C (1,55 s [0,013]) e o C++ (1,54 s [0,014]). A razão manteve-se em todos os níveis (VM_12: Java 0,88 s; C 2,23 s). Novamente o custo do Java apareceu na variabilidade sob restrição (IQR de 1,21 s no VM_12, contra 0,05 s do C) e no pico de memória, embora a diferença de memória tenha sido menor neste experimento (238,9 MB contra 220,5 MB no hospedeiro), pois os 5 milhões de nós dominam o consumo nas três linguagens.

### 4.4 Comportamento do coletor de lixo

Os 495 logs de GC coletados (um por execução Java) registram apenas a linha de inicialização do coletor Serial. **Nenhuma coleta de lixo ocorreu em nenhuma execução, em nenhum ambiente — inclusive no VM_12, com heap de 960 MB.** Com `-Xms` igual a `-Xmx` dimensionado em 75% da RAM, o conjunto vivo dos três experimentos (no máximo ~239 MB) nunca exerceu pressão suficiente sobre o heap para disparar o coletor.

### 4.5 Testes de hipótese e ajuste de curva

O teste de Kruskal-Wallis apontou diferença significativa entre as linguagens em todos os 15 pares ambiente×experimento, e as comparações par a par (Mann-Whitney com correção de Holm) confirmam a vantagem do Java sobre C e C++ em todos os ambientes, com delta de Cliff |δ| = 1,00 na quase totalidade dos casos (separação completa das distribuições). Para a H1, o ajuste da mediana em função do nível de restrição preferiu o modelo linear (menor AIC) em 8 das 9 séries linguagem×experimento; a única série em que o modelo exponencial venceu (Java, Experimento 1) tem taxa de crescimento de apenas 1,16 por nível, muito aquém de uma explosão exponencial.

## 5. DISCUSSÃO

### 5.1 H1: refutada nos termos propostos

A H1 previa degradação exponencial do Java sob restrição, causada por ciclos de bloqueio do coletor e pressão sobre o heap, contra degradação linear de C e C++. Os dados refutam a hipótese nos seus próprios termos: a degradação foi aproximadamente linear — e modesta — para as três linguagens, e o mecanismo causal previsto (pausas do GC) simplesmente não ocorreu. A razão é instrutiva do ponto de vista de engenharia: ao fixar o heap em 75% da RAM com pré-alocação (`-Xms = -Xmx`) e usar estruturas cujo conjunto vivo cabe com folga nesse heap, o experimento colocou a JVM na sua condição mais favorável — alocação por avanço de ponteiro sem nunca pagar o custo da coleta. O "custo da abstração" do gerenciamento automático não é um imposto fixo: ele só é cobrado quando a pressão de memória obriga o coletor a agir.

### 5.2 Por que o Java foi mais rápido

Três fatores explicam a vantagem consistente do Java nas medianas. Primeiro, a alocação na JVM avança um ponteiro sobre uma região contígua pré-reservada, de modo que nós criados em sequência ficam adjacentes na memória — melhor localidade espacial nas travessias do que a oferecida pelo `malloc`, que administra blocos livres e espalha as alocações (PATTERSON; HENNESSY, 2017). Segundo, a liberação é adiada: C e C++ pagam um `free`/`delete` por nó dentro do trecho cronometrado, enquanto no Java a devolução de memória ficaria a cargo de um coletor que nunca precisou executar. Terceiro, o compilador JIT especializa o laço quente em tempo de execução. O resultado inverte a expectativa ingênua — mas ao preço visível no pico de memória física (até 2,4× o do C no Experimento 1) e na reserva antecipada de heap: no commit de memória, o processo Java reservou o heap inteiro configurado, e quando o heap configurado não coube na RAM do ambiente a JVM sequer inicializou, falha observada nos testes preliminares deste trabalho.

### 5.3 O custo real da restrição: previsibilidade

O efeito mais claro da escassez de recursos sobre o runtime gerenciado não apareceu na tendência central, e sim na dispersão. No nível VM_12, o IQR do Java foi de 50 a 25 vezes o do C (0,208 s contra 0,004 s no Experimento 1; 1,21 s contra 0,05 s no Experimento 3), com execuções individuais até duas vezes mais lentas que a mediana, enquanto C e C++ mantiveram dispersão quase idêntica à dos ambientes folgados. Para sistemas embarcados e de tempo real, em que o requisito é previsibilidade e latência limitada no pior caso — não a média (TANENBAUM; BOS, 2015) —, essa perda de determinismo é exatamente o argumento a favor do gerenciamento manual e do RAII, e os dados a quantificam mesmo na ausência de coletas de lixo.

### 5.4 O degrau da virtualização e o platô entre níveis

A passagem do hospedeiro para a máquina virtual custou de 25 a 45% de tempo, o maior salto de toda a série — atribuível ao custo de tradução de endereços em dois níveis (paginação aninhada) e à mediação do hipervisor, que penalizam justamente cargas intensivas em memória como as deste estudo. Já a redução progressiva de vCPU e RAM entre os níveis de VM quase não moveu as medianas: como cada benchmark executa em um único núcleo (afinidade fixa) e o conjunto de trabalho coube na RAM de todos os níveis, cortar recursos excedentes não afeta a tendência central — afeta a cauda da distribuição, como mostra o IQR do VM_12. O nível VM_100 apresentou medianas levemente superiores às do VM_50 e VM_25 em alguns casos, com o maior IQR entre os níveis intermediários, compatível com a disputa de CPU com o hospedeiro ao ceder 7 das 8 vCPUs e com atividade leve registrada no hospedeiro durante essa bateria.

### 5.5 H2 e limitações

A H2 (falhas de cache como gatilho das diferenças) só pôde ser avaliada indiretamente, pois o hipervisor não expõe os contadores de desempenho da CPU ao convidado. Os indícios disponíveis são compatíveis com a hipótese na sua premissa — o Experimento 2, dominado por acesso não sequencial, foi o mais lento e o mais penalizado pela virtualização —, mas contrariam a conclusão esperada: a localidade produzida pela alocação contígua da JVM beneficiou o Java justamente no cenário de pointer chasing. Permanecem como limitações: ausência de medição direta de cache; um único hardware e um único hipervisor; restrição simulada por virtualização, e não por hardware embarcado real; heap fixo em 75% da RAM, condição favorável ao GC — cargas maiores que pressionem o heap, ou heaps menores, ficam como trabalho futuro para observar o coletor em ação; e a coleta piloto de setembro de 2026 foi descartada por não atender ao protocolo final.

## 6. CONCLUSÃO (esqueleto)

- A escolha do paradigma de gerenciamento de memória não impôs, nas condições testadas, o custo de tempo esperado ao Java; o custo real manifestou-se em memória (pico e reserva antecipada) e, sob restrição severa, em previsibilidade (dispersão até 50× maior).
- A virtualização, não a escassez incremental de recursos, foi o fator dominante de degradação para as três linguagens.
- Para o engenheiro: em cargas que cabem no heap, o Java com heap pré-alocado é competitivo ou superior; onde RAM é escassa ou a latência de pior caso importa, C e C++ (RAII) mantêm comportamento determinístico que o runtime gerenciado não garante.
- H1 refutada nos termos propostos; H2 inconclusiva por limitação instrumental, com indícios contrários à conclusão esperada.
