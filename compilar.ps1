# LOG: Compila experimentos (gcc/g++ -O2 estatico, javac) e o medidor externo para a pasta bin e grava as versoes dos compiladores em bin\versoes.txt.
# compilar.ps1 - compila todos os experimentos com as flags fixas do estudo
# Uso (PowerShell, na raiz do repositorio):  .\compilar.ps1
# Saida: bin\*.exe (C e C++) e bin\java\*.class (Java)

$ErrorActionPreference = "Stop"
$raiz = $PSScriptRoot
$bin  = Join-Path $raiz "bin"
$binJava = Join-Path $bin "java"
$exps = Join-Path $raiz "experimentos"

New-Item -ItemType Directory -Force $bin, $binJava | Out-Null

$CFLAGS   = @("-O2", "-static-libgcc")
$CXXFLAGS = @("-O2", "-static-libgcc", "-static-libstdc++")

Get-ChildItem $exps -Recurse -Filter *.c | ForEach-Object {
    $saida = Join-Path $bin ($_.BaseName + ".exe")
    Write-Host "gcc  $($_.Name)"
    & gcc @CFLAGS $_.FullName -o $saida
    if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar $($_.Name)" }
}

Get-ChildItem $exps -Recurse -Filter *.cpp | ForEach-Object {
    $saida = Join-Path $bin ($_.BaseName + ".exe")
    Write-Host "g++  $($_.Name)"
    & g++ @CXXFLAGS $_.FullName -o $saida
    if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar $($_.Name)" }
}

# Launcher (medidor externo de CPU e pico de memoria)
$medidor = Join-Path $raiz "launcher\medidor.c"
if (Test-Path $medidor) {
    Write-Host "gcc  medidor.c"
    & gcc -O2 $medidor -o (Join-Path $bin "medidor.exe") -lpsapi
    if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar medidor.c" }
}

$fontesJava = Get-ChildItem $exps -Recurse -Filter *.java | ForEach-Object { $_.FullName }
if ($fontesJava) {
    Write-Host "javac $($fontesJava.Count) arquivo(s)"
    & javac -d $binJava @fontesJava
    if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar Java" }
}

# Registra as versoes usadas nesta compilacao
$versoes = @(
    "Data: $(Get-Date -Format 'yyyy-MM-dd HH:mm')",
    "Maquina: $env:COMPUTERNAME",
    (& gcc --version | Select-Object -First 1),
    (& g++ --version | Select-Object -First 1),
    (& javac -version 2>&1 | Out-String).Trim()
)
$versoes | Set-Content (Join-Path $bin "versoes.txt")
Write-Host ""
$versoes | ForEach-Object { Write-Host $_ }
Write-Host "`nConcluido. Binarios em: $bin" -ForegroundColor Green
