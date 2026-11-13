# Ambiente de testes

Versões congeladas até o fim da coleta: Windows Update pausado e sem
`pacman -Syu` no MSYS2.

## Hardware (host)

| Item | Valor |
|---|---|
| CPU | Intel Core i7-9700F — 8 núcleos / 8 threads (sem HT), base 3,0 GHz |
| Cache | L1 512 KB · L2 2 MB (256 KB/núcleo) · L3 12 MB |
| RAM | 16 GB DDR4-2400 (2 de 4 slots) |
| Placa-mãe | Gigabyte Z390 M GAMING, BIOS F8 |
| Disco da VM | SSD |

## Software (host e VM idênticos)

| Item | Versão |
|---|---|
| Windows | 10 22H2, build 19045.6466 (host Pro, VM Home) |
| GCC / G++ | 16.2.0 (Rev6, MSYS2 mingw-w64) |
| JDK | Oracle 25.0.4.1 LTS |
| VirtualBox | _anotar (Ajuda → Sobre)_ |

## Máquina virtual

- Execution Engine VT-x/AMD-V, paginação aninhada ativa, Execution Cap 100%
- Paravirtualização Hyper-V; Hyper-V do host desativado (`hypervisorlaunchtype off`)
- Disco VDI dinâmico no SSD, marcado como SSD, sem cache de I/O do hospedeiro
- Áudio, área de transferência, drag-and-drop, pastas compartilhadas e desktop remoto desativados
- Rede desconectada durante a coleta
- Uma única VM; cada nível parte do snapshot "VM-0 limpa"

## Matriz de ambientes

| Config | vCPU | RAM | Heap Java (`-Xms`/`-Xmx`) |
|---|---|---|---|
| Host | 8 | 16 GB | 12g |
| VM-0 | 4 | 8 GB | 6g |
| VM-1 | 2 | 4 GB | 3g |
| VM-2 | 1 | 2 GB | 1536m |
| VM-3 | 1 | 1 GB | 768m |

Na VM-3 só a RAM diminui (não há menos de 1 vCPU). 1 GB fica abaixo do
mínimo oficial do Windows 10 64-bit.

## JVM

- Coletor: `-XX:+UseSerialGC` em todos os ambientes (sem a escolha automática da JVM)
- Heap fixo em ~75% da RAM do ambiente
- Log de GC: `-Xlog:gc*:file=<arquivo>`
