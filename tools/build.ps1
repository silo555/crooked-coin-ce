param(
    [string]$Cedev = $env:CEDEV,
    [switch]$Clean
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

if (-not $Cedev) {
    $config = Get-Command cedev-config -ErrorAction SilentlyContinue
    if ($config) { $Cedev = & $config.Source --prefix }
}
if (-not $Cedev) {
    throw 'Set CEDEV to an existing CEdev v15 folder, or pass -Cedev <folder>. See docs/BUILDING.md.'
}
$Cedev = (Resolve-Path -LiteralPath $Cedev).Path
$make = Join-Path $Cedev 'bin/make.exe'
$compiler = Join-Path $Cedev 'bin/ez80-clang.exe'
if (-not (Test-Path -LiteralPath $make) -or -not (Test-Path -LiteralPath $compiler)) {
    throw "The selected folder does not contain CEdev's Windows build tools: $Cedev"
}
if ($Cedev.Contains(' ') -or $projectRoot.Contains(' ')) {
    throw 'CEdev make requires paths without spaces. Move the checkout/toolchain to paths without spaces.'
}
$previousPath = $env:PATH
$previousCedev = $env:CEDEV
try {
    $env:PATH = (Join-Path $Cedev 'bin') + ';' + $env:PATH
    $env:CEDEV = $Cedev
    Push-Location $projectRoot
    try {
        if ($Clean) {
            & $make clean
            if ($LASTEXITCODE -ne 0) { throw 'CEdev clean failed.' }
        }
        & $make
        if ($LASTEXITCODE -ne 0) { throw 'CEdev build failed.' }
        python tools/verify_8xp.py bin/CROOKED.8xp --icon-asm obj/icon.s
        if ($LASTEXITCODE -ne 0) { throw 'TI transfer validation failed.' }
    } finally { Pop-Location }
} finally {
    $env:PATH = $previousPath
    $env:CEDEV = $previousCedev
}
