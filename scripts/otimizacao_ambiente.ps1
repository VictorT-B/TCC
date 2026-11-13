# LOG: Prepara host ou VM antes da coleta: plano Alto Desempenho com CPU min/max 100%, desativa WSearch/SysMain/wuauserv/DiagTrack e exclui a pasta do repositorio do Defender. Rodar como admin.
# Executar como Administrador ANTES de cada bateria de testes (Ambiente A ou B)
# Uso: .\otimizacao_ambiente.ps1 -PastaTestes "C:\TCC\benchmarks"

param(
    [string]$PastaTestes = (Split-Path $PSScriptRoot -Parent)
)

Write-Host "=== Aplicando otimizacoes para reduzir ruido nas medicoes ===" -ForegroundColor Cyan

# 1. Plano de energia - Alto Desempenho
powercfg /setactive SCHEME_MIN
Write-Host "[OK] Plano de energia: Alto Desempenho"

# 2. Trava o estado minimo/maximo do processador em 100%
#    Evita variacao de clock (throttling / turbo instavel) entre execucoes
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMIN 100
powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX 100
powercfg /setactive SCHEME_CURRENT
Write-Host "[OK] CPU travada em 100% de clock minimo/maximo"

# 3. Para e desativa servicos que geram I/O/CPU em background
$servicos = @("WSearch", "SysMain", "wuauserv", "DiagTrack")
foreach ($s in $servicos) {
    Stop-Service -Name $s -Force -ErrorAction SilentlyContinue
    Set-Service -Name $s -StartupType Disabled -ErrorAction SilentlyContinue
    Write-Host "[OK] Servico desativado: $s"
}

# 4. Exclui a pasta de testes do scan em tempo real do Defender
if (Test-Path $PastaTestes) {
    Add-MpPreference -ExclusionPath $PastaTestes -ErrorAction SilentlyContinue
    Write-Host "[OK] Pasta excluida do Windows Defender: $PastaTestes"
} else {
    Write-Host "[AVISO] Pasta nao encontrada, ajuste -PastaTestes: $PastaTestes" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Otimizacoes aplicadas. Recomendacoes adicionais (manuais):" -ForegroundColor Cyan
Write-Host " - Feche navegadores, IDEs e apps em segundo plano antes de rodar a bateria"
Write-Host " - Desconecte a rede/Wi-Fi durante os testes (reduz interrupcoes)"
Write-Host " - Rode a bateria completa sem tocar no mouse/teclado durante a execucao"
Write-Host ""
Write-Host "Para revertir apos os testes, reative os servicos manualmente via services.msc"
