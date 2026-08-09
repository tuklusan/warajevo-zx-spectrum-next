# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

[CmdletBinding()]
param(
    [string]$ProjectDir = 'D:\WarajevoSpectrum.Next',
    [string]$ExistingInstallPath = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools',
    [string]$ToolRoot = 'D:\WarajevoSpectrum.Next\.toolchains\vs17',
    [string]$BootstrapperUrl = 'https://aka.ms/vs/17/release/vs_BuildTools.exe'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

Set-Location $ProjectDir

Get-Process -Name winget,setup,vs_installer,vs_buildtools -ErrorAction SilentlyContinue | Stop-Process -Force

$setup = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
if (-not (Test-Path -LiteralPath $setup)) {
    throw "Visual Studio installer setup.exe was not found at $setup"
}

if (Test-Path -LiteralPath $ExistingInstallPath) {
    $uninstallArguments = @(
        'uninstall'
        '--installPath'
        ('"{0}"' -f $ExistingInstallPath)
        '--passive'
        '--norestart'
    )

    Write-Output ("uninstall-path={0}" -f $ExistingInstallPath)
    $uninstall = Start-Process -FilePath $setup -ArgumentList $uninstallArguments -Wait -PassThru -WindowStyle Hidden
    Write-Output ("uninstall-exit-code={0}" -f $uninstall.ExitCode)
    if ($uninstall.ExitCode -ne 0) {
        return $uninstall.ExitCode
    }
}

$cacheDir = Join-Path $ToolRoot 'cache'
$sharedDir = Join-Path $ToolRoot 'shared'
$installDir = Join-Path $ToolRoot 'BuildTools'
$bootstrapper = Join-Path $ToolRoot 'vs_BuildTools.exe'

$null = New-Item -ItemType Directory -Force -Path $ToolRoot, $cacheDir, $sharedDir
Invoke-WebRequest -Uri $BootstrapperUrl -OutFile $bootstrapper

$installArguments = @(
    '--path'
    ('install="{0}"' -f $installDir)
    '--path'
    ('cache="{0}"' -f $cacheDir)
    '--path'
    ('shared="{0}"' -f $sharedDir)
    '--add'
    'Microsoft.Component.MSBuild'
    '--add'
    'Microsoft.VisualStudio.Component.VC.CoreBuildTools'
    '--add'
    'Microsoft.VisualStudio.Component.VC.Redist.14.Latest'
    '--add'
    'Microsoft.VisualStudio.Component.Windows10SDK'
    '--add'
    'Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Core'
    '--add'
    'Microsoft.VisualStudio.Component.VC.Tools.x86.x64'
    '--add'
    'Microsoft.VisualStudio.Component.Windows10SDK.19041'
    '--passive'
    '--wait'
    '--norestart'
)

Write-Output ("bootstrapper-path={0}" -f $bootstrapper)
Write-Output ("buildtools-install-path={0}" -f $installDir)

$install = Start-Process -FilePath $bootstrapper -ArgumentList $installArguments -Wait -PassThru -WindowStyle Hidden
Write-Output ("install-exit-code={0}" -f $install.ExitCode)
return $install.ExitCode
