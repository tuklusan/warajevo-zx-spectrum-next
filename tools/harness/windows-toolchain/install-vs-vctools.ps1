# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectDir,

    [Parameter(Mandatory = $true)]
    [string]$InstallPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

Set-Location $ProjectDir

$setup = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
if (-not (Test-Path -LiteralPath $setup)) {
    throw "Visual Studio installer setup.exe was not found at $setup"
}

$arguments = @(
    'modify'
    '--installPath'
    ('"{0}"' -f $InstallPath)
    '--add'
    'Microsoft.VisualStudio.Workload.NativeDesktop'
    '--includeRecommended'
    '--passive'
    '--norestart'
)

Write-Output ("setup-path={0}" -f $setup)
Write-Output ("install-path={0}" -f $InstallPath)

$process = Start-Process -FilePath $setup -ArgumentList $arguments -Wait -PassThru -WindowStyle Hidden
Write-Output ("setup-exit-code={0}" -f $process.ExitCode)
return $process.ExitCode
