# LOG: Prepara o ambiente do TCC: checa gcc/java/git/python+libs, valida VM e snapshot do VirtualBox, calcula niveis RAM/vCPU e -Xmx (75%) e cria a pasta tcc-final com git.
#
# Uso (PowerShell, na pasta onde salvou o arquivo):
#   powershell -ExecutionPolicy Bypass -File .\preparar_ambiente.ps1
#   powershell -ExecutionPolicy Bypass -File .\preparar_ambiente.ps1 -VM "NomeDaVM" -TestarRestore
#
# -VM            nome da VM no VirtualBox (veja a lista que o script imprime)
# -Snapshot      nome do snapshot base (vazio = snapshot atual)
# -TestarRestore restaura o snapshot, aplica o nivel 1/2, confere e restaura de novo (VM precisa estar desligada)
# -MargemHostGB  RAM reservada ao Windows do host quando a VM esta "sem limite" (padrao 4 GB)

param(
    [string]$Raiz = "$HOME\tcc-final",
    [string]$VM = "",
    [string]$Snapshot = "",
    [int]$MargemHostGB = 4,
    [switch]$TestarRestore
)

$ErrorActionPreference = "Continue"
$script:pendencias = @()
function Ok($m)    { Write-Host "[OK]    $m" -ForegroundColor Green }
function Aviso($m) { Write-Host "[AVISO] $m" -ForegroundColor Yellow }
function Falha($m) { Write-Host "[FALTA] $m" -ForegroundColor Red; $script:pendencias += $m }
function Info($m)  { Write-Host "        $m" }

# ---------------------------------------------------------------- 1. Ferramentas
Write-Host "`n== 1. Ferramentas ==" -ForegroundColor Cyan
$ferramentas = @(
    @{ n = "gcc";   a = "--version" },
    @{ n = "g++";   a = "--version" },
    @{ n = "java";  a = "-version"  },
    @{ n = "javac"; a = "-version"  },
    @{ n = "git";   a = "--version" }
)
foreach ($t in $ferramentas) {
    if (Get-Command $t.n -ErrorAction SilentlyContinue) {
        $v = & $t.n $t.a 2>&1 | Select-Object -First 1
        Ok "$($t.n): $v"
    } else {
        Falha "$($t.n) nao encontrado no PATH"
    }
}

# ---------------------------------------------------------------- 2. Python + libs
Write-Host "`n== 2. Python e bibliotecas de analise ==" -ForegroundColor Cyan
$py = $null
if (Get-Command py -ErrorAction SilentlyContinue) { $py = "py" }
elseif (Get-Command python -ErrorAction SilentlyContinue) { $py = "python" }

if ($py) {
    $pyv = & $py --version 2>&1
    if ($LASTEXITCODE -ne 0 -or -not "$pyv") {
        Falha "Python parece ser so o atalho da Microsoft Store. Instale pelo python.org marcando 'Add to PATH'"
    } else {
        Ok "Python: $pyv"
        $libs = @("numpy", "pandas", "scipy", "matplotlib")
        $faltando = @()
        foreach ($lib in $libs) {
            & $py -c "import $lib" 2>$null
            if ($LASTEXITCODE -ne 0) { $faltando += $lib }
        }
        if ($faltando.Count -gt 0) {
            Info "Instalando: $($faltando -join ', ')"
            & $py -m pip install --upgrade $faltando
            foreach ($lib in $faltando) {
                & $py -c "import $lib" 2>$null
                if ($LASTEXITCODE -ne 0) { Falha "Falha ao instalar $lib" } else { Ok "$lib instalado" }
            }
        } else {
            Ok "numpy, pandas, scipy e matplotlib ja instalados"
        }
    }
} else {
    Falha "Python nao encontrado (instale pelo python.org marcando 'Add to PATH')"
}

# ---------------------------------------------------------------- 3. Hardware e niveis
Write-Host "`n== 3. Hardware do host e niveis de ambiente ==" -ForegroundColor Cyan
$cs  = Get-CimInstance Win32_ComputerSystem
$cpu = Get-CimInstance Win32_Processor
$hostRamMB = [int][math]::Floor($cs.TotalPhysicalMemory / 1MB)
$fisicos   = ($cpu | Measure-Object -Property NumberOfCores -Sum).Sum
$logicos   = ($cpu | Measure-Object -Property NumberOfLogicalProcessors -Sum).Sum
Info "CPU: $(($cpu | Select-Object -First 1).Name.Trim())"
Info "Nucleos fisicos: $fisicos | logicos: $logicos | RAM: $([math]::Round($hostRamMB/1024,1)) GB"

function Xmx($ramMB) { [int]([math]::Floor($ramMB * 0.75 / 64) * 64) }

$niveis = @()
$niveis += [pscustomobject]@{
    Ambiente = "Host"; RAM_MB = $hostRamMB; vCPU = $logicos; Xmx_MB = (Xmx $hostRamMB)
}

# VM "sem limite": RAM do host menos a margem, vCPU = nucleos fisicos (limite pratico do VirtualBox)
$vmBaseMB = [int]([math]::Floor(($hostRamMB - $MargemHostGB * 1024) / 512) * 512)
$nomes = @("VM_100", "VM_50", "VM_25", "VM_12")
for ($i = 0; $i -lt $nomes.Count; $i++) {
    $div = [math]::Pow(2, $i)
    $ram = [int]([math]::Floor($vmBaseMB / $div / 256) * 256)
    $cpus = [int][math]::Max(1, [math]::Floor($fisicos / $div))
    $niveis += [pscustomobject]@{
        Ambiente = $nomes[$i]; RAM_MB = $ram; vCPU = $cpus; Xmx_MB = (Xmx $ram)
    }
}
$niveis | Format-Table -AutoSize | Out-String | Write-Host

foreach ($n in $niveis | Where-Object { $_.Ambiente -ne "Host" }) {
    $sobra = $n.RAM_MB - $n.Xmx_MB
    if ($n.RAM_MB -lt 2048) {
        Aviso "$($n.Ambiente): $($n.RAM_MB) MB e pouco pra um Windows 10 convidado. Teste se ele chega a dar boot; se nao, descarte esse nivel."
    } elseif ($sobra -lt 1024) {
        Aviso "$($n.Ambiente): sobram so $sobra MB fora do heap pro Windows convidado. Pode haver paginacao (anote nas limitacoes)."
    }
}

New-Item -ItemType Directory -Force -Path (Join-Path $Raiz "scripts") | Out-Null
$csvNiveis = Join-Path $Raiz "scripts\niveis.csv"
$niveis | Export-Csv -Path $csvNiveis -NoTypeInformation -Encoding UTF8
Ok "Tabela de niveis salva em $csvNiveis"

# ---------------------------------------------------------------- 4. VirtualBox
Write-Host "`n== 4. VirtualBox, VM e snapshot ==" -ForegroundColor Cyan
$vbox = Join-Path $env:ProgramFiles "Oracle\VirtualBox\VBoxManage.exe"
if (-not (Test-Path $vbox)) {
    $c = Get-Command VBoxManage -ErrorAction SilentlyContinue
    if ($c) { $vbox = $c.Source } else { $vbox = $null }
}

function InfoVM($nome) {
    $h = @{}
    foreach ($l in (& $vbox showvminfo $nome --machinereadable)) {
        if ($l -match '^"?([^"=]+)"?="?([^"]*)"?$') { $h[$matches[1]] = $matches[2] }
    }
    return $h
}

if ($vbox) {
    Ok "VBoxManage $(& $vbox --version)"
    Info "VMs cadastradas:"
    & $vbox list vms | ForEach-Object { Info "  $_" }

    if ($VM) {
        $snaps = & $vbox snapshot $VM list 2>&1
        $temSnap = ($LASTEXITCODE -eq 0)
        if ($temSnap) {
            Ok "Snapshots de '$VM':"
            $snaps | ForEach-Object { Info "  $_" }
        } else {
            Falha "VM '$VM' sem snapshot ou nome errado. Com a VM desligada e limpa: & '$vbox' snapshot `"$VM`" take base"
        }

        $iv = InfoVM $VM
        Info "Estado: $($iv['VMState']) | RAM atual: $($iv['memory']) MB | vCPU atual: $($iv['cpus'])"

        if ($TestarRestore -and $temSnap) {
            if ($iv['VMState'] -notin @("poweroff", "aborted", "saved")) {
                Falha "Desligue a VM antes de testar o restore (estado atual: $($iv['VMState']))"
            } else {
                $alvo = $niveis | Where-Object { $_.Ambiente -eq "VM_50" }
                if ($Snapshot) { & $vbox snapshot $VM restore $Snapshot } else { & $vbox snapshot $VM restorecurrent }
                if ($LASTEXITCODE -ne 0) {
                    Falha "Restore do snapshot falhou"
                } else {
                    Ok "Restore do snapshot funcionou"
                    & $vbox modifyvm $VM --memory $alvo.RAM_MB --cpus $alvo.vCPU
                    $iv2 = InfoVM $VM
                    if ([int]$iv2['memory'] -eq $alvo.RAM_MB -and [int]$iv2['cpus'] -eq $alvo.vCPU) {
                        Ok "modifyvm aplicou o nivel VM_50 ($($alvo.RAM_MB) MB / $($alvo.vCPU) vCPU)"
                    } else {
                        Falha "modifyvm nao aplicou o nivel VM_50 (ficou $($iv2['memory']) MB / $($iv2['cpus']) vCPU)"
                    }
                    if ($Snapshot) { & $vbox snapshot $VM restore $Snapshot } else { & $vbox snapshot $VM restorecurrent }
                    Ok "Snapshot restaurado de novo (VM voltou ao estado base)"
                }
            }
        } elseif (-not $TestarRestore) {
            Info "Rode de novo com -TestarRestore (VM desligada) pra validar o fluxo de sexta."
        }
    } else {
        Info "Rode de novo com -VM `"NomeDaVM`" pra checar o snapshot."
    }
} else {
    Falha "VirtualBox (VBoxManage.exe) nao encontrado"
}

# ---------------------------------------------------------------- 5. Pasta + git
Write-Host "`n== 5. Pasta do repositorio ==" -ForegroundColor Cyan
$pastas = @("src\c", "src\cpp", "src\java", "scripts", "results\raw", "results\gc_logs", "analysis")
foreach ($p in $pastas) { New-Item -ItemType Directory -Force -Path (Join-Path $Raiz $p) | Out-Null }

$gi = Join-Path $Raiz ".gitignore"
if (-not (Test-Path $gi)) { "*.exe`n*.o`n*.class`n__pycache__/`n" | Set-Content -Path $gi -Encoding ASCII }

$rd = Join-Path $Raiz "README.md"
if (-not (Test-Path $rd)) {
@"
# TCC - Estruturas de dados em C, C++ e Java sob restricao de hardware

- src/      implementacoes (lista duplamente encadeada, grafo/BFS, tabela hash)
- scripts/  otimizacao do ambiente, orquestracao dos benchmarks, niveis.csv
- results/  CSVs brutos e logs de GC
- analysis/ estatistica e graficos
"@ | Set-Content -Path $rd -Encoding UTF8
}

if (Get-Command git -ErrorAction SilentlyContinue) {
    if (-not (Test-Path (Join-Path $Raiz ".git"))) { git -C $Raiz init | Out-Null }
    Ok "Repositorio git em $Raiz"
    $remoto = git -C $Raiz remote get-url origin 2>$null
    if ($remoto) {
        Ok "Remoto: $remoto"
    } elseif (Get-Command gh -ErrorAction SilentlyContinue) {
        Info "Pra criar o repo no GitHub: gh repo create tcc-estruturas-dados --public --source `"$Raiz`" --remote origin"
    } else {
        Info "Crie um repo VAZIO em https://github.com/new e rode:"
        Info "  git -C `"$Raiz`" remote add origin https://github.com/SEU_USUARIO/tcc-estruturas-dados.git"
    }
}

# ---------------------------------------------------------------- Resumo
Write-Host "`n== Resumo ==" -ForegroundColor Cyan
if ($script:pendencias.Count -eq 0) {
    Ok "Tudo pronto pra sexta."
} else {
    Write-Host "Pendencias:" -ForegroundColor Red
    $script:pendencias | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
}
