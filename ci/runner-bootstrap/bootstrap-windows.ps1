param([Parameter(Mandatory=$true)][string]$Manifest)
$ErrorActionPreference = 'Stop'
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { winget install --id Kitware.CMake --exact --accept-source-agreements --accept-package-agreements }
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) { winget install --id Ninja-build.Ninja --exact --accept-source-agreements --accept-package-agreements }
if (-not (Get-Command python -ErrorAction SilentlyContinue)) { winget install --id Python.Python.3.12 --exact --accept-source-agreements --accept-package-agreements }
if (-not (Test-Path -LiteralPath $Manifest)) { throw "Missing bootstrap manifest: $Manifest" }
