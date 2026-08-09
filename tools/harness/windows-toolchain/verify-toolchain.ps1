# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

[CmdletBinding()]
param(
    [string]$ProjectDir,
    [string]$InstallPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

if ($ProjectDir) {
    Set-Location $ProjectDir
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
Write-Output ("computer-name={0}" -f $env:COMPUTERNAME)
Write-Output ("project-dir={0}" -f (Get-Location).Path)

cmd /c fsutil volume diskfree c:
if (Test-Path -LiteralPath 'D:\') {
    cmd /c fsutil volume diskfree d:
}

cmd /c where cmake
cmd /c where ninja
cmd /c where clang

if (Test-Path -LiteralPath $vswhere) {
    & $vswhere -products * -format json
}

if ($InstallPath) {
    $msvcRoot = Join-Path $InstallPath 'VC\Tools\MSVC'
    if (Test-Path -LiteralPath $msvcRoot) {
        Get-ChildItem -Path $msvcRoot -Filter cl.exe -Recurse -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty FullName
    }
}

return 0
