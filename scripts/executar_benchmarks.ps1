# LOG: Roda os 9 benchmarks via medidor.exe (afinidade fixa, prioridade alta) com aquecimento descartado e ordem embaralhada por semente; grava 1 linha por execucao em results\raw e logs de GC.
#
# Uso (na raiz do repositorio, depois do .\compilar.ps1):
#   powershell -ExecutionPolicy Bypass -File .\scripts\executar_benchmarks.ps1 -Ambiente Host
#   powershell -ExecutionPolicy Bypass -File .\scripts\executar_benchmarks.ps1 -Ambiente VM_50
#   Teste rapido:  ... -Ambiente Host -Repeticoes 1 -Aquecimento 0
#
# -Ambiente     nome do nivel (Host, VM_100, VM_50, VM_25, VM_12) - usado no CSV e pra achar o -Xmx em scripts\niveis.csv
# -XmxMB        sobrescreve o heap Java (padrao: niveis.csv; se nao achar, 75% da RAM detectada)
# -Mascara      afinidade em hex (padrao: 0x2 = nucleo 1; com 1 vCPU usa 0x1)
# -Semente      semente do embaralhamento da ordem

param(
    [Parameter(Mandatory = $true)][string]$Ambiente,
    [int]$Repeticoes = 30,
    [int]$Aquecimento = 3,
    [int]$PausaMs = 500,
    [int]$XmxMB = 0,
    [string]$Mascara = "",
    [int]$Semente = 115
)

$ErrorActionPreference = "Continue"
$raiz    = Split-Path $PSScriptRoot -Parent
$bin     = Join-Path $raiz "bin"
$medidor = Join-Path $bin "medidor.exe"
$binJava = Join-Path $bin "java"
$dirRaw  = Join-Path $raiz "results\raw"
$dirGc   = Join-Path $raiz "results\gc_logs\$Ambiente"
New-Item -ItemType Directory -Force $dirRaw, $dirGc | Out-Null

if (-not (Test-Path $medidor)) { throw "bin\medidor.exe nao encontrado. Rode .\compilar.ps1 antes." }
# Roda a partir da raiz: o -Xlog da JVM usa caminho relativo (o ':' de 'C:\' conflita com a sintaxe do -Xlog)
Set-Location $raiz

# ---------------------------------------------------------------- Ambiente
$cs = Get-CimInstance Win32_ComputerSystem
$ramMB = [int][math]::Floor($cs.TotalPhysicalMemory / 1MB)
$vcpu  = [int]$cs.NumberOfLogicalProcessors

if ($XmxMB -le 0) {
    $csvNiveis = Join-Path $PSScriptRoot "niveis.csv"
    if (Test-Path $csvNiveis) {
        $linha = Import-Csv $csvNiveis | Where-Object { $_.Ambiente -eq $Ambiente } | Select-Object -First 1
        if ($linha) { $XmxMB = [int]$linha.Xmx_MB }
    }
    if ($XmxMB -le 0) { $XmxMB = [int]([math]::Floor($ramMB * 0.75 / 64) * 64) }
}
if (-not $Mascara) { $Mascara = if ($vcpu -ge 2) { "0x2" } else { "0x1" } }

Write-Host "Ambiente=$Ambiente  RAM=$ramMB MB  vCPU=$vcpu  Xmx=$XmxMB MB  afinidade=$Mascara  rep=$Repeticoes  aquec=$Aquecimento" -ForegroundColor Cyan

# ---------------------------------------------------------------- Combinacoes
$jvm = @("-XX:+UseSerialGC", "-Xms${XmxMB}m", "-Xmx${XmxMB}m", "-cp", $binJava)
$combos = @(
    @{ ling = "C";    exp = 1; prog = (Join-Path $bin "exp1_c.exe");   args = @() },
    @{ ling = "C++";  exp = 1; prog = (Join-Path $bin "exp1_cpp.exe"); args = @() },
    @{ ling = "Java"; exp = 1; prog = "java"; args = $jvm + "Experimento1" },
    @{ ling = "C";    exp = 2; prog = (Join-Path $bin "exp2_c.exe");   args = @() },
    @{ ling = "C++";  exp = 2; prog = (Join-Path $bin "exp2_cpp.exe"); args = @() },
    @{ ling = "Java"; exp = 2; prog = "java"; args = $jvm + "Experimento2" },
    @{ ling = "C";    exp = 3; prog = (Join-Path $bin "exp3_c.exe");   args = @() },
    @{ ling = "C++";  exp = 3; prog = (Join-Path $bin "exp3_cpp.exe"); args = @() },
    @{ ling = "Java"; exp = 3; prog = "java"; args = $jvm + "Experimento3" }
)
$esperado = @{ 1 = "4a5df0a09d318eb1"; 2 = "f8d7b2fec15dd2cd"; 3 = "79292f2b7b9ef235" }

foreach ($c in $combos) {
    if ($c.prog -ne "java" -and -not (Test-Path $c.prog)) { throw "Binario nao encontrado: $($c.prog)" }
}

# ---------------------------------------------------------------- Ordem: aquecimento embaralhado, depois validas embaralhadas
$rng = New-Object System.Random $Semente
function Embaralhar($lista) {
    $a = @($lista)
    for ($i = $a.Count - 1; $i -gt 0; $i--) {
        $j = $rng.Next($i + 1)
        $t = $a[$i]; $a[$i] = $a[$j]; $a[$j] = $t
    }
    return $a
}

$aquec = @(); $validas = @()
foreach ($c in $combos) {
    for ($r = 1; $r -le $Aquecimento; $r++) { $aquec   += , @{ c = $c; rep = $r; aq = 1 } }
    for ($r = 1; $r -le $Repeticoes; $r++)  { $validas += , @{ c = $c; rep = $r; aq = 0 } }
}
$fila = @()
if ($aquec.Count -gt 0) { $fila += Embaralhar $aquec }
$fila += Embaralhar $validas

# ---------------------------------------------------------------- CSV
$carimbo = Get-Date -Format "yyyyMMdd_HHmm"
$csv = Join-Path $dirRaw "resultados_${Ambiente}_$carimbo.csv"
"ambiente,ram_mb,vcpu,xmx_mb,afinidade,linguagem,experimento,ordem,repeticao,aquecimento,t_trecho_s,checksum,checksum_ok,parede_s,cpu_user_s,cpu_kernel_s,pico_ws_kb,page_faults,pico_commit_kb,exit_code,fases,gc_log" |
    Set-Content -Path $csv -Encoding ASCII

# ---------------------------------------------------------------- Execucao
$total = $fila.Count
$falhas = 0
$inicio = Get-Date
for ($k = 0; $k -lt $total; $k++) {
    $item = $fila[$k]; $c = $item.c
    $args2 = @($c.args)
    $gcLog = ""
    if ($c.ling -eq "Java") {
        $gcLog = "results\gc_logs\$Ambiente\" + ("exp{0}_r{1:D2}_aq{2}.log" -f $c.exp, $item.rep, $item.aq)
        # -Xlog vai antes do nome da classe
        $args2 = @("-Xlog:gc:file=$gcLog") + $args2
    }

    $saida = & $medidor $Mascara $c.prog @args2 2>&1 | ForEach-Object { "$_" }

    $med = $saida | Where-Object { $_ -like "MEDIDOR;*" } | Select-Object -Last 1
    $res = $saida | Where-Object { $_ -match '^[0-9.]+(,[0-9.]+)+,[0-9a-f]{16}$' } | Select-Object -Last 1

    $tTrecho = ""; $chk = ""; $ok = 0; $fases = ""
    if ($res) {
        $p = $res -split ","
        $chk = $p[-1]
        $tTrecho = $p[-2]
        $fases = ($p[0..($p.Count - 2)] -join "|")
        if ($chk -eq $esperado[$c.exp]) { $ok = 1 }
    }
    $m = @("", "", "", "", "", "", "")
    if ($med) { $m = ($med -split ";")[1..7] }

    if (-not $res -or -not $med -or $ok -ne 1 -or $m[6] -ne "0") {
        $falhas++
        Write-Host "  [FALHA] $($c.ling) exp$($c.exp) rep $($item.rep): $($saida -join ' | ')" -ForegroundColor Red
    }

    ($Ambiente, $ramMB, $vcpu, $XmxMB, $Mascara, $c.ling, $c.exp, ($k + 1), $item.rep, $item.aq,
     $tTrecho, $chk, $ok, $m[0], $m[1], $m[2], $m[3], $m[4], $m[5], $m[6], $fases, $gcLog) -join "," |
        Add-Content -Path $csv -Encoding ASCII

    $decorrido = (Get-Date) - $inicio
    $restante = [TimeSpan]::FromSeconds($decorrido.TotalSeconds / ($k + 1) * ($total - $k - 1))
    $tag = if ($item.aq) { "aquec" } else { "     " }
    Write-Host ("[{0,4}/{1}] {2} {3,-4} exp{4} rep{5,2}  trecho={6,-9} pico={7,8} KB  chk={8}  falta ~{9:hh\:mm\:ss}" -f
        ($k + 1), $total, $tag, $c.ling, $c.exp, $item.rep, $tTrecho, $m[3], $ok, $restante)

    Start-Sleep -Milliseconds $PausaMs
}

Write-Host ""
Write-Host "Concluido em $([int]((Get-Date) - $inicio).TotalMinutes) min. Falhas: $falhas" -ForegroundColor $(if ($falhas) { "Yellow" } else { "Green" })
Write-Host "CSV: $csv"
