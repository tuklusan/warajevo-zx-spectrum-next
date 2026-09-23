param([Parameter(Mandatory=$true)][string]$Manifest)
$ErrorActionPreference = 'Stop'
winget install --id Kitware.CMake --exact --accept-source-agreements --accept-package-agreements
winget install --id Ninja-build.Ninja --exact --accept-source-agreements --accept-package-agreements
winget install --id Python.Python.3.12 --exact --accept-source-agreements --accept-package-agreements
if (-not (Test-Path -LiteralPath $Manifest)) { throw "Missing bootstrap manifest: $Manifest" }
