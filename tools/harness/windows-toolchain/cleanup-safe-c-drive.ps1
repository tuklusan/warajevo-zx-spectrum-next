# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectDir,

    [int]$MinimumAgeDays = 14
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
Set-Location $ProjectDir

$activeInstaller = Get-Process -Name setup,vs_installer,vs_buildtools,msiexec -ErrorAction SilentlyContinue
if ($activeInstaller) {
    Write-Output 'cleanup-skipped=installer-active'
    return 2
}

$drive = Get-PSDrive -Name C
$before = $drive.Free
$cutoff = (Get-Date).AddDays(-1 * $MinimumAgeDays)
$targets = @(
    $env:TEMP
    (Join-Path $env:WINDIR 'Temp')
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -Unique

$removedBytes = [int64]0
$removedFiles = 0
foreach ($target in $targets) {
    $items = Get-ChildItem -LiteralPath $target -File -Recurse -Force -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -lt $cutoff }
    foreach ($item in $items) {
        try {
            $removedBytes += [int64]$item.Length
            Remove-Item -LiteralPath $item.FullName -Force -ErrorAction Stop
            $removedFiles++
        } catch {
            continue
        }
    }
}

$after = (Get-PSDrive -Name C).Free
Write-Output ('free-before-bytes={0}' -f $before)
Write-Output ('free-after-bytes={0}' -f $after)
Write-Output ('removed-files={0}' -f $removedFiles)
Write-Output ('removed-bytes={0}' -f $removedBytes)
return 0
