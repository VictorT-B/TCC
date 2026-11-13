/*
 * LOG: Exp.2 em Java. Mesmo grafo e BFS do C, com nos como objetos na heap. Fila int[] sem autoboxing. Liberacao so solta a referencia; o GC recolhe. Imprime tempos e checksum.
 *
 * Uso: java -XX:+UseSerialGC -Xms<X> -Xmx<X> Experimento2 [VERTICES] [ARESTAS] [BUSCAS] [SEED]
 *      padrao: 200000 2000000 10 115
 *
 * Saida (ultima linha, CSV):
 *   t_construcao,t_bfs,t_liberacao,t_total,checksum
 *
 * Fila em int[] e visitado em boolean[] (1 byte por posicao), iguais ao
 * C e ao C++. A versao antiga usava ArrayDeque<Integer>, que criava um
 * objeto Integer por vertice enfileirado (autoboxing) e distorcia a
 * comparacao. Nao ha System.gc() em nenhum ponto.
 *
 * Compilar: javac Experimento2.java
 */
public class Experimento2 {

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

    // ─── Grafo: lista de adjacencia com nos encadeados ──────────────────────
    static final class Vizinho {
        final int destino;
        final Vizinho proximo;
        Vizinho(int destino, Vizinho proximo) { this.destino = destino; this.proximo = proximo; }
    }

    static final class Grafo {
        final int numVertices;
        final Vizinho[] adj;

        Grafo(int n) { numVertices = n; adj = new Vizinho[n]; }

        void adicionarAresta(int u, int v) {
            adj[u] = new Vizinho(v, adj[u]);
            adj[v] = new Vizinho(u, adj[v]);
        }

        long bfs(int origem, long h) {
            boolean[] visitado = new boolean[numVertices];
            int[] fila = new int[numVertices];
            int ini = 0, fim = 0;

            visitado[origem] = true;
            fila[fim++] = origem;
            while (ini < fim) {
                int atual = fila[ini++];
                h = h * 31L + atual;
                for (Vizinho v = adj[atual]; v != null; v = v.proximo) {   // pointer chasing
                    if (!visitado[v.destino]) {
                        visitado[v.destino] = true;
                        fila[fim++] = v.destino;
                    }
                }
            }
            return h * 31L + fim;
        }   // visitado e fila viram lixo; o GC decide quando recolher
    }

    public static void main(String[] args) {
        int V      = args.length > 0 ? Integer.parseInt(args[0]) : 200000;
        int A      = args.length > 1 ? Integer.parseInt(args[1]) : 2000000;
        int BUSCAS = args.length > 2 ? Integer.parseInt(args[2]) : 10;
        long seed  = args.length > 3 ? Long.parseLong(args[3]) : 115L;
        if (V < 2 || A < 0 || BUSCAS < 1) {
            System.err.println("Parametros invalidos");
            System.exit(1);
        }
        prngEstado = seed;

        // Fase 1: construcao
        long t0 = System.nanoTime();
        Grafo g = new Grafo(V);
        long arestas = 0;
        for (int i = 0; i < A; i++) {
            int u = prngLimite(V);
            int v = prngLimite(V);
            if (u != v) { g.adicionarAresta(u, v); arestas++; }
        }
        long t1 = System.nanoTime();

        // Fase 2: BFS a partir de origens sorteadas
        long h = arestas;
        for (int b = 0; b < BUSCAS; b++) h = g.bfs(prngLimite(V), h);
        long t2 = System.nanoTime();

        // Fase 3: liberacao (apenas solta a referencia)
        long t3 = System.nanoTime();
        g = null;
        long t4 = System.nanoTime();

        double tc = (t1 - t0) / 1e9, tb = (t2 - t1) / 1e9, tl = (t4 - t3) / 1e9;
        System.out.println("=== Experimento 2 (Java) V=" + V + " A=" + A + " BUSCAS=" + BUSCAS + " SEED=" + seed + " ===");
        System.out.println(String.format(java.util.Locale.ROOT,
                "%.6f,%.6f,%.6f,%.6f,%016x", tc, tb, tl, tc + tb + tl, h));
    }
}
