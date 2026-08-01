#!/usr/bin/env pwsh
# build.ps1 - Build do firmware ESP32 com PlatformIO e gera binário de flash

# Nome do ambiente definido em platformio.ini, ex:
# [env:esp32dev]  -> $EnvName = "esp32dev"
$EnvName = "esp32dev"   # ajuste para o seu env

# Vai para o diretório do projeto (onde está o platformio.ini)
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

# Descobre qual comando do PlatformIO existe: `pio` ou `platformio`
if (Get-Command pio -ErrorAction SilentlyContinue) {
    $PioCmd = "pio"
}
elseif (Get-Command platformio -ErrorAction SilentlyContinue) {
    $PioCmd = "platformio"
}
else {
    Write-Error "ERRO: PlatformIO CLI não encontrado. Instale o PlatformIO Core (pio/platformio) e tente novamente."
    exit 1
}

Write-Host "==> Build do ambiente '$EnvName'..."
& $PioCmd run -e $EnvName

$BinSrc = ".pio/build/$EnvName/firmware.bin"
$BinDir = "build"
$BinDst = Join-Path $BinDir "$EnvName-firmware.bin"

if (-not (Test-Path $BinDir)) {
    New-Item -ItemType Directory -Path $BinDir | Out-Null
}

if (-not (Test-Path $BinSrc)) {
    Write-Error "ERRO: binário não encontrado em $BinSrc"
    exit 1
}

Copy-Item $BinSrc $BinDst -Force

Write-Host "==> Binário de flash gerado em: $BinDst"