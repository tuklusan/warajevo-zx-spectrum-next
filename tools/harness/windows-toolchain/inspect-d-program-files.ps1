# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
Set-Location $ProjectDir

$roots = @('D:\Program Files', 'D:\Program Files (x86)')
foreach ($root in $roots) {
    Write-Output ('root={0}' -f $root)
    if (-not (Test-Path -LiteralPath $root)) {
        Write-Output 'exists=no'
        continue
    }
    Write-Output 'exists=yes'
    Get-ChildItem -LiteralPath $root -Directory -Force -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match 'Visual Studio|Build Tools|LLVM|CMake|Windows Kits' } |
        Select-Object -ExpandProperty FullName
}

$libraryNames = @('msvcrtd.lib', 'oldnames.lib', 'msvcrt.lib', 'libcmt.lib')
foreach ($name in $libraryNames) {
    Write-Output ('library={0}' -f $name)
    Get-ChildItem -Path 'D:\Program Files', 'D:\Program Files (x86)' -Filter $name -File -Recurse -Force -ErrorAction SilentlyContinue |
        Select-Object -ExpandProperty FullName
}
return 0
