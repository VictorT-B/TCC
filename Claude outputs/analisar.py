# LOG: Le os CSVs de results/raw (+ raw da VM), valida checksums, calcula mediana/IQR/IC bootstrap, Kruskal-Wallis, Mann-Whitney/Holm, delta de Cliff, ajuste linear x exponencial (AIC) e gera graficos e tabelas em analise/saida.
#
# Uso (na raiz do repositorio):
#   py analise\analisar.py
# Le todos os resultados_*.csv de results\raw e results\vm\raw.
# Saida: analise\saida\ (tabelas .csv/.md e graficos .png)

import glob
import os
import sys

import numpy as np
import pandas as pd
from scipy import stats
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SAIDA = os.path.join(RAIZ, "analise", "saida")
os.makedirs(SAIDA, exist_ok=True)

ORDEM_AMB = ["Host", "VM_100", "VM_50", "VM_25", "VM_12"]
ORDEM_LING = ["C", "C++", "Java"]
NOME_EXP = {1: "Exp1 Lista", 2: "Exp2 Grafo/BFS", 3: "Exp3 Tabela Hash"}
RNG = np.random.default_rng(115)

# ---------------------------------------------------------------- carga
arquivos = sorted(
    glob.glob(os.path.join(RAIZ, "results", "raw", "resultados_*.csv"))
    + glob.glob(os.path.join(RAIZ, "results", "vm", "raw", "resultados_*.csv"))
)
if not arquivos:
    sys.exit("Nenhum resultados_*.csv em results/raw ou results/vm/raw")

frames = []
for a in arquivos:
    d = pd.read_csv(a)
    d["arquivo"] = os.path.basename(a)
    frames.append(d)
dados = pd.concat(frames, ignore_index=True)

# Se um ambiente tiver mais de um CSV (rodada refeita), fica o arquivo mais novo
ultimos = dados.groupby("ambiente")["arquivo"].max()
dados = dados[dados.apply(lambda r: r["arquivo"] == ultimos[r["ambiente"]], axis=1)]

print("Arquivos usados:")
for amb, arq in ultimos.items():
    print(f"  {amb}: {arq}")

# ---------------------------------------------------------------- validacao
prob = dados[(dados.checksum_ok != 1) | (dados.exit_code != 0)]
if len(prob):
    print(f"\n[AVISO] {len(prob)} execucoes invalidas (checksum/exit) DESCARTADAS:")
    print(prob[["ambiente", "linguagem", "experimento", "repeticao", "aquecimento"]].to_string())
dados = dados[(dados.checksum_ok == 1) & (dados.exit_code == 0)]

v = dados[dados.aquecimento == 0].copy()
v["pico_mb"] = v.pico_ws_kb / 1024.0
cont = v.groupby(["ambiente", "linguagem", "experimento"]).size()
if (cont != 30).any():
    print("\n[AVISO] combinacoes sem 30 execucoes validas:")
    print(cont[cont != 30].to_string())
print(f"\nObservacoes validas: {len(v)}")


def ic_mediana(x, n=10000):
    med = RNG.choice(x, size=(n, len(x)), replace=True)
    m = np.median(med, axis=1)
    return np.percentile(m, 2.5), np.percentile(m, 97.5)


def cliff_delta(a, b):
    a, b = np.asarray(a), np.asarray(b)
    maior = sum((x > b).sum() for x in a)
    menor = sum((x < b).sum() for x in a)
    return (maior - menor) / (len(a) * len(b))


# ---------------------------------------------------------------- tabela resumo
linhas = []
for (amb, ling, exp), g in v.groupby(["ambiente", "linguagem", "experimento"]):
    t = g.t_trecho_s.values
    lo, hi = ic_mediana(t)
    linhas.append({
        "ambiente": amb, "linguagem": ling, "experimento": exp, "n": len(t),
        "t_mediana_s": np.median(t), "t_iqr_s": np.subtract(*np.percentile(t, [75, 25])),
        "t_ic95_inf": lo, "t_ic95_sup": hi,
        "cpu_user_med_s": g.cpu_user_s.median(), "cpu_kernel_med_s": g.cpu_kernel_s.median(),
        "parede_med_s": g.parede_s.median(),
        "pico_mb_med": g.pico_mb.median(),
        "page_faults_med": g.page_faults.median(),
    })
resumo = pd.DataFrame(linhas).sort_values(["experimento", "ambiente", "linguagem"])
resumo["ambiente"] = pd.Categorical(resumo.ambiente, ORDEM_AMB)
resumo = resumo.sort_values(["experimento", "ambiente", "linguagem"])
resumo.to_csv(os.path.join(SAIDA, "resumo.csv"), index=False, float_format="%.6f")

# ---------------------------------------------------------------- testes por ambiente
tst = []
for (amb, exp), g in v.groupby(["ambiente", "experimento"]):
    grupos = {l: gg.t_trecho_s.values for l, gg in g.groupby("linguagem")}
    if len(grupos) < 3:
        continue
    H, p_kw = stats.kruskal(*grupos.values())
    pares, ps = [], []
    for l1, l2 in [("C", "C++"), ("C", "Java"), ("C++", "Java")]:
        _, p = stats.mannwhitneyu(grupos[l1], grupos[l2], alternative="two-sided")
        pares.append((l1, l2, cliff_delta(grupos[l1], grupos[l2])))
        ps.append(p)
    # correcao de Holm
    ordem = np.argsort(ps)
    p_adj = [None] * 3
    maior = 0.0
    for rank, idx in enumerate(ordem):
        padj = min(1.0, ps[idx] * (3 - rank))
        maior = max(maior, padj)
        p_adj[idx] = maior
    for (l1, l2, d), p, padj in zip(pares, ps, p_adj):
        tst.append({"ambiente": amb, "experimento": exp, "KW_H": H, "KW_p": p_kw,
                    "par": f"{l1} vs {l2}", "MW_p": p, "MW_p_holm": padj, "cliff_delta": d})
pd.DataFrame(tst).to_csv(os.path.join(SAIDA, "testes.csv"), index=False, float_format="%.3e")

# ---------------------------------------------------------------- H1: linear x exponencial (AIC)
def aic(res, n, k):
    sse = np.sum(res ** 2)
    return n * np.log(sse / n) + 2 * k


h1 = []
med = v.groupby(["experimento", "linguagem", "ambiente"]).t_trecho_s.median().reset_index()
med["k"] = med.ambiente.map({a: i for i, a in enumerate(ORDEM_AMB)})
med = med.dropna(subset=["k"])
for (exp, ling), g in med.groupby(["experimento", "linguagem"]):
    g = g.sort_values("k")
    if len(g) < 4:
        continue
    x, y = g.k.values.astype(float), g.t_trecho_s.values
    b_lin = np.polyfit(x, y, 1)
    res_lin = y - np.polyval(b_lin, x)
    b_exp = np.polyfit(x, np.log(y), 1)
    res_exp = y - np.exp(np.polyval(b_exp, x))
    a_lin, a_exp = aic(res_lin, len(y), 2), aic(res_exp, len(y), 2)
    h1.append({"experimento": exp, "linguagem": ling,
               "AIC_linear": a_lin, "AIC_exponencial": a_exp,
               "melhor": "exponencial" if a_exp < a_lin else "linear",
               "taxa_exp": np.exp(b_exp[0])})
pd.DataFrame(h1).to_csv(os.path.join(SAIDA, "h1_ajuste.csv"), index=False, float_format="%.4f")

# ---------------------------------------------------------------- graficos
CORES = {"C": "#1f77b4", "C++": "#ff7f0e", "Java": "#2ca02c"}
for exp in sorted(v.experimento.unique()):
    for metrica, rotulo, arq in [("t_trecho_s", "Tempo do trecho (s)", "tempo"),
                                 ("pico_mb", "Pico de memoria fisica (MB)", "memoria")]:
        fig, ax = plt.subplots(figsize=(7, 4.2))
        for ling in ORDEM_LING:
            ys, lo, hi = [], [], []
            ambs = [a for a in ORDEM_AMB if ((v.experimento == exp) & (v.ambiente == a)).any()]
            for amb in ambs:
                va = v[(v.experimento == exp) & (v.linguagem == ling) & (v.ambiente == amb)][metrica].values
                ys.append(np.median(va))
                l, h = ic_mediana(va)
                lo.append(ys[-1] - l)
                hi.append(h - ys[-1])
            ax.errorbar(range(len(ambs)), ys, yerr=[lo, hi], marker="o", capsize=3,
                        label=ling, color=CORES[ling])
        ax.set_xticks(range(len(ambs)))
        ax.set_xticklabels(ambs)
        ax.set_yscale("log")
        ax.set_xlabel("Ambiente (restricao crescente)")
        ax.set_ylabel(rotulo + " — escala log")
        ax.set_title(f"{NOME_EXP[exp]}: mediana e IC95% (bootstrap)")
        ax.legend()
        ax.grid(True, alpha=.3)
        fig.tight_layout()
        fig.savefig(os.path.join(SAIDA, f"exp{exp}_{arq}.png"), dpi=150)
        plt.close(fig)

# ---------------------------------------------------------------- tabela em markdown
with open(os.path.join(SAIDA, "resumo.md"), "w", encoding="utf-8") as f:
    for exp in sorted(resumo.experimento.unique()):
        f.write(f"\n## {NOME_EXP[exp]}\n\n")
        f.write("| Ambiente | Linguagem | Mediana (s) | IQR (s) | IC95% | Pico RAM (MB) |\n")
        f.write("|---|---|---|---|---|---|\n")
        for _, r in resumo[resumo.experimento == exp].iterrows():
            f.write(f"| {r.ambiente} | {r.linguagem} | {r.t_mediana_s:.4f} | {r.t_iqr_s:.4f} | "
                    f"[{r.t_ic95_inf:.4f}; {r.t_ic95_sup:.4f}] | {r.pico_mb_med:.1f} |\n")

print(f"\nSaida em: {SAIDA}")
print("  resumo.csv / resumo.md  - medianas, IQR, IC95%")
print("  testes.csv              - Kruskal-Wallis, Mann-Whitney/Holm, delta de Cliff")
print("  h1_ajuste.csv           - AIC linear x exponencial por linguagem")
print("  exp*_tempo.png / exp*_memoria.png")
