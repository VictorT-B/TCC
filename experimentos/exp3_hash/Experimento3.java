/*
 * LOG: Exp.3 em Java. Mesma tabela hash do C com nos como objetos na heap; no rehash o vetor antigo vira lixo e o GC recolhe quando decidir. Imprime tempos e checksum.
 *
 * Experimento 3 - Realocacao Massiva em Tabela Hash (Java, GC)
 *
 * Uso: java -XX:+UseSerialGC -Xms<X> -Xmx<X> Experimento3 [CHAVES] [CAP_INICIAL]
 *      padrao: 5000000 1024
 *
 * Chaves e indices tratados como inteiros SEM sinal de 32 bits
 * (Integer.remainderUnsigned), iguais ao uint32_t do C e do C++.
 * Nao ha System.gc() em nenhum ponto.
 *
 * Saida (ultima linha, CSV):
 *   t_insercao,t_liberacao,t_total,checksum
 *
 * Compilar: javac Experimento3.java
 */
public class Experimento3 {

    static final class NoHash {
        final int chave;          // 32 bits, interpretado sem sinal
        NoHash proximo;
        NoHash(int chave, NoHash proximo) { this.chave = chave; this.proximo = proximo; }
    }

    static final class TabelaHash {
        NoHash[] baldes;
        int capacidade, tamanho, rehashes;

        TabelaHash(int capInicial) { capacidade = capInicial; baldes = new NoHash[capInicial]; }

        void redimensionar() {
            int nova = capacidade * 2;
            NoHash[] novos = new NoHash[nova];
            for (int i = 0; i < capacidade; i++) {
                NoHash a = baldes[i];
                while (a != null) {
                    NoHash prox = a.proximo;
                    int idx = Integer.remainderUnsigned(a.chave, nova);
                    a.proximo = novos[idx];
                    novos[idx] = a;
                    a = prox;
                }
            }
            baldes = novos;           // vetor antigo inalcancavel; o GC decide quando recolher
            capacidade = nova;
            rehashes++;
        }

        void inserir(int chave) {
            if ((long) tamanho * 4L > (long) capacidade * 3L) redimensionar();
            int idx = Integer.remainderUnsigned(chave, capacidade);
            baldes[idx] = new NoHash(chave, baldes[idx]);
            tamanho++;
        }

        long checksum() {
            long h = tamanho;
            h = h * 31L + capacidade;
            h = h * 31L + rehashes;
            for (int i = 0; i < capacidade; i++)
                for (NoHash a = baldes[i]; a != null; a = a.proximo)
                    h = h * 31L + Integer.toUnsignedLong(a.chave);
            return h;
        }
    }

    public static void main(String[] args) {
        long CHAVES = args.length > 0 ? Long.parseLong(args[0]) : 5000000L;
        long CAP0   = args.length > 1 ? Long.parseLong(args[1]) : 1024L;
        if (CHAVES <= 0 || CHAVES > 1000000000L || CAP0 < 1 || CAP0 > (1L << 30)) {
            System.err.println("Parametros invalidos");
            System.exit(1);
        }

        TabelaHash t = new TabelaHash((int) CAP0);

        // Fase 1: insercao (com rehashing)
        long t0 = System.nanoTime();
        int n = (int) CHAVES;
        for (int i = 0; i < n; i++)
            t.inserir(i * (int) 2654435761L);   // mesmo produto mod 2^32 do C
        long t1 = System.nanoTime();

        // Verificacao (fora da contagem de tempo)
        long cs = t.checksum();
        int cap = t.capacidade, rh = t.rehashes;

        // Fase 2: liberacao (apenas solta a referencia)
        long t2 = System.nanoTime();
        t = null;
        long t3 = System.nanoTime();

        double ti = (t1 - t0) / 1e9, tl = (t3 - t2) / 1e9;
        System.out.println("=== Experimento 3 (Java) CHAVES=" + CHAVES + " CAP_INICIAL=" + CAP0
                + " CAP_FINAL=" + Integer.toUnsignedString(cap) + " REHASHES=" + rh + " ===");
        System.out.println(String.format(java.util.Locale.ROOT,
                "%.6f,%.6f,%.6f,%016x", ti, tl, ti + tl, cs));
    }
}
