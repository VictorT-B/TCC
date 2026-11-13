# LEIA-ME — o que falta pra fechar o TCC (estado de 13/11/2026, 13h40)

## O arquivo que você vai editar
`docs/tccMainDoc_revisado_13_11_26.docx` — versão completa montada sobre o tccMainDoc 27_09_26.
Convenção: PRETO = original mantido · VERMELHO = inserido em 13/11 · CINZA RISCADO = removido.
Pra aceitar uma mudança: apague o trecho riscado e mude a cor do vermelho pra automática (Ctrl+T pra selecionar tudo no fim, se quiser aceitar tudo de uma vez — mas revise antes).

## Checklist de correção (em ordem)
1. [ ] Ler o documento inteiro aceitando/ajustando as partes vermelhas (28 páginas).
2. [ ] Citações no texto na REVISÃO BIBLIOGRÁFICA (parte preta): o parecer pediu. As fontes de cada subseção:
   - Gerenciamento manual/C → (KERNIGHAN; RITCHIE, 1986)
   - RAII/C++ → (STROUSTRUP, 2013)
   - GC/Java → (BLOCH, 2018) e (JONES; HOSKING; MOSS, 2011)
   - Estruturas e complexidade, listas, grafos/BFS, hash → (CORMEN et al., 2012)
   - Localidade/cache/memória virtual → (PATTERSON; HENNESSY, 2017) e (TANENBAUM; BOS, 2015)
3. [ ] Inserir na metodologia o link do repositório e o hash do commit final:
   "O código-fonte, os scripts e os dados brutos estão disponíveis em https://github.com/VictorT-B/TCC (commit <hash>)."
   O hash sai de: git log -1 --format=%h
4. [ ] Conferir o template da UNIARA (Template explicativo artigo UNIARA 2026.docx): resumo 150–250 palavras, abstract em inglês (traduzir o resumo), formatação de títulos/quadros/figuras.
5. [ ] Numerar seções e figuras conforme o template (hoje: Figura 1–4, Quadro A + Quadro 1).
6. [ ] Revisão final de português e remoção das marcas de cor.
7. [ ] Enviar ao orientador.

## Onde está cada coisa (tudo dentro desta pasta tcc-final)
- `docs/tccMainDoc_revisado_13_11_26.docx` — o documento a corrigir
- `docs/metodologia_v2.md`, `docs/resultados_discussao.md`, `docs/resumo_intro_hipoteses.md` — textos-fonte das partes vermelhas
- `analise/saida/` — resumo.md (tabelas), testes.csv (estatística), h1_ajuste.csv (AIC) e os 6 gráficos PNG
- `results/raw/` e `results/vm/raw/` — CSVs brutos dos 5 ambientes (Host, VM_100, VM_50, VM_25, VM_12)
- `results/gc_logs/` — logs de GC (todos com 33 bytes = nenhuma coleta ocorreu; é um RESULTADO, citado na seção 4.4)
- `experimentos/`, `launcher/`, `scripts/` — código final dos benchmarks

## Números-chave (se precisar conferir algo no texto)
- 1.485 execuções, 1.350 válidas, 0 falhas de checksum
- Checksums: Exp1 4a5df0a09d318eb1 · Exp2 f8d7b2fec15dd2cd · Exp3 79292f2b7b9ef235
- Java mais rápido em TODOS os cenários; nenhuma coleta de GC em nenhum ambiente
- AIC: linear vence em 8 de 9 séries → H1 refutada
- IQR do Java no VM_12: 50× o do C (Exp1), ~25× (Exp3) → perda de previsibilidade
- Degrau Host→VM: 25–45%; entre níveis de VM: platô
- Ambientes: Host 16GB/8vCPU · VM_100 11776MB/7 · VM_50 5888/4 · VM_25 2816/2 · VM_12 1280/1; -Xmx = 75% da RAM; Serial GC; JDK 25.0.4.1; GCC 16.2.0; VirtualBox 7.2.18
