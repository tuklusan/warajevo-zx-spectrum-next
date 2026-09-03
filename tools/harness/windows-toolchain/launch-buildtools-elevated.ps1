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
    [string]$BootstrapperPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

Set-Location $ProjectDir

$toolRoot = Join-Path $ProjectDir '.toolchains\vs17'
$installDir = Join-Path $toolRoot 'BuildTools'
$cacheDir = Join-Path $toolRoot 'cache'
$sharedDir = Join-Path $toolRoot 'shared'

$arguments = @(
    '--installPath'
    ('"{0}"' -f $installDir)
    '--path'
    ('cache="{0}"' -f $cacheDir)
    '--path'
    ('shared="{0}"' -f $sharedDir)
    '--add'
    'Microsoft.VisualStudio.Workload.NativeDesktop'
    '--add'
    'Microsoft.VisualStudio.Component.VC.Tools.x86.x64'
    '--add'
    'Microsoft.VisualStudio.Component.Windows10SDK.19041'
    '--includeRecommended'
    '--passive'
    '--wait'
    '--norestart'
)

Write-Output ('bootstrapper-path={0}' -f $BootstrapperPath)
Write-Output ('install-path={0}' -f $installDir)
Write-Output 'elevation-requested=yes'

$process = Start-Process -FilePath $BootstrapperPath -ArgumentList $arguments -Verb RunAs -Wait -PassThru
Write-Output ('installer-exit-code={0}' -f $process.ExitCode)
return $process.ExitCode
