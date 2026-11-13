/*
 * LOG: Exp.1 em Java. Mesma lista do C com nos como objetos na heap; removidos ficam inalcancaveis e o GC recolhe quando decidir. Imprime tempo por fase e checksum.
 *
 * Experimento 1 - Fragmentacao em Lista Duplamente Encadeada (Java, GC)
 *
 * Uso: java -XX:+UseSerialGC -Xms<X> -Xmx<X> Experimento1 [N] [REMOVER] [SEED]
 *      padrao: N=1000000, REMOVER=N/2, SEED=115
 *
 * Saida (ultima linha, CSV):
 *   t_construcao,t_remocao,t_reinsercao,t_liberacao,t_total,checksum
 *
 * Fase de liberacao: em Java ela consiste apenas em soltar a referencia
 * da lista (lista = null), que e o que um programa real faz. O custo de
 * recolher os nos fica com o GC, e aparece no tempo total do processo
 * (medido pelo launcher) e no log -Xlog:gc*, nao nesta fase.
 * Nao ha System.gc() em nenhum ponto.
 *
 * Compilar: javac Experimento1.java
 */
public class Experimento1 {

    // ─── PRNG comum as tres linguagens: SplitMix64 ──────────────────────────
    static long prngEstado;

    static long prngProximo() {
        long z = (prngEstado += 0x9E3779B97F4A7C15L);
        z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
        z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
        return z ^ (z >>> 31);
    }

    static int prngLimite(int limite) {
        return (int) ((prngProximo() >>> 33) % limite);
    }

    // ─── Lista duplamente encadeada ─────────────────────────────────────────
    static final class No {
        int valor;
        No anterior;
        No proximo;
        No(int valor, No anterior) { this.valor = valor; this.anterior = anterior; }
    }

    static final class Lista {
        No cabeca, cauda;
        int tamanho;

        void inserirFim(int valor) {
            No novo = new No(valor, cauda);
            if (cauda != null) cauda.proximo = novo;
            else               cabeca = novo;
            cauda = novo;
            tamanho++;
        }

        void removerNo(No alvo) {
            if (alvo.anterior != null) alvo.anterior.proximo = alvo.proximo;
            else                       cabeca = alvo.proximo;
            if (alvo.proximo != null)  alvo.proximo.anterior = alvo.anterior;
            else                       cauda = alvo.anterior;
            tamanho--;
            // sem free(): o no fica inalcancavel e o GC decide quando recolher
        }

        long checksum() {
            long h = tamanho;
            for (No a = cabeca; a != null; a = a.proximo)
                h = h * 31L + a.valor;
            return h;
        }
    }

    public static void main(String[] args) {
        int N       = args.length > 0 ? Integer.parseInt(args[0]) : 1000000;
        int REMOVER = args.length > 1 ? Integer.parseInt(args[1]) : N / 2;
        long seed   = args.length > 2 ? Long.parseLong(args[2]) : 115L;
        if (N <= 0 || REMOVER < 0 || REMOVER > N) {
            System.err.println("Parametros invalidos");
            System.exit(1);
        }
        prngEstado = seed;

        Lista lista = new Lista();

        // Fase 1: construcao
        long t0 = System.nanoTime();
        for (int i = 0; i < N; i++) lista.inserirFim(i);
        long t1 = System.nanoTime();

        // Fase 2: remocao em posicoes pseudoaleatorias (Fisher-Yates)
        No[] ponteiros = new No[N];
        No a = lista.cabeca;
        for (int i = 0; i < N; i++) { ponteiros[i] = a; a = a.proximo; }
        for (int i = N - 1; i > 0; i--) {
            int j = prngLimite(i + 1);
            No tmp = ponteiros[i]; ponteiros[i] = ponteiros[j]; ponteiros[j] = tmp;
        }
        for (int i = 0; i < REMOVER; i++) lista.removerNo(ponteiros[i]);
        ponteiros = null;
        a = null;
        long t2 = System.nanoTime();

        // Fase 3: reinsercao
        for (int i = 0; i < REMOVER; i++) lista.inserirFim(i);
        long t3 = System.nanoTime();

        // Verificacao (fora da contagem de tempo)
        long cs = lista.checksum();

        // Fase 4: liberacao (apenas solta a referencia)
        long t3b = System.nanoTime();
        lista = null;
        long t4 = System.nanoTime();

        double tc = (t1 - t0) / 1e9, tr = (t2 - t1) / 1e9;
        double ti = (t3 - t2) / 1e9, tl = (t4 - t3b) / 1e9;
        System.out.println("=== Experimento 1 (Java) N=" + N + " REMOVER=" + REMOVER + " SEED=" + seed + " ===");
        // Locale.ROOT garante ponto decimal mesmo em Windows pt-BR
        System.out.println(String.format(java.util.Locale.ROOT,
                "%.6f,%.6f,%.6f,%.6f,%.6f,%016x", tc, tr, ti, tl, tc + tr + ti + tl, cs));
    }
}
