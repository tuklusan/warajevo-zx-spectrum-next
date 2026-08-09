# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectDir,

    [switch]$IncludeMsiexec
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

Set-Location $ProjectDir

$names = @('winget', 'setup', 'vs_installer', 'vs_buildtools')
if ($IncludeMsiexec) {
    $names += 'msiexec'
}

$processes = Get-Process -Name $names -ErrorAction SilentlyContinue |
    Select-Object Name, Id, StartTime, CPU, Path

Write-Output ("process-count={0}" -f @($processes).Count)
if ($processes) {
    $processes | Format-Table -AutoSize | Out-String -Width 4096 | Write-Output
    $processes | Stop-Process -Force
    Write-Output 'stopped-processes'
}

return 0
