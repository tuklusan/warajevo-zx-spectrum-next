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

$processes = Get-Process -Name winget,setup,vs_installer,vs_buildtools,msiexec -ErrorAction SilentlyContinue |
    Select-Object Name, Id, StartTime, CPU, Path
Write-Output ("process-count={0}" -f @($processes).Count)
if ($processes) {
    $processes | Format-Table -AutoSize | Out-String -Width 4096 | Write-Output
}

$logs = Get-ChildItem -Path $env:TEMP -Filter 'dd_*' -File -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending

Write-Output ("log-count={0}" -f @($logs).Count)
if ($logs) {
    $logs |
        Select-Object -First 10 Name, LastWriteTime, FullName |
        Format-Table -AutoSize | Out-String -Width 4096 | Write-Output
}

$errorLog = $logs | Where-Object { $_.Name -like '*_errors.log' } | Select-Object -First 1
if ($errorLog) {
    Write-Output ("error-log={0}" -f $errorLog.FullName)
    Get-Content -LiteralPath $errorLog.FullName -Tail 80
}

$latestLog = $logs | Select-Object -First 1
if ($latestLog) {
    Write-Output ("latest-log={0}" -f $latestLog.FullName)
    Get-Content -LiteralPath $latestLog.FullName -Tail 80
}

return 0
