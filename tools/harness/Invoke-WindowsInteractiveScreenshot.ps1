# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [string]$CaptureScriptPath,

    [int]$TimeoutSeconds = 30
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$projectRoot = [System.IO.Path]::GetFullPath((Get-Location).Path)
$projectRootPrefix = if ($projectRoot.EndsWith('\')) { $projectRoot } else { $projectRoot + '\' }
$resolvedOutput = [System.IO.Path]::GetFullPath(
    $(if ([System.IO.Path]::IsPathRooted($OutputPath)) { $OutputPath } else { Join-Path $projectRoot $OutputPath })
)

if (
    $resolvedOutput -eq $projectRoot -or
    -not $resolvedOutput.StartsWith($projectRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)
) {
    throw "output path must remain below the project directory"
}

$computer = Get-CimInstance -ClassName Win32_ComputerSystem
$interactiveUser = $computer.UserName
if ([string]::IsNullOrWhiteSpace($interactiveUser)) {
    throw "no active interactive desktop user was detected"
}

$captureScript = if ([string]::IsNullOrWhiteSpace($CaptureScriptPath)) {
    Join-Path $projectRoot 'tools\harness\Capture-WindowsDesktopScreenshot.ps1'
} else {
    [System.IO.Path]::GetFullPath($CaptureScriptPath)
}
if (
    $captureScript -eq $projectRoot -or
    -not $captureScript.StartsWith($projectRootPrefix, [System.StringComparison]::OrdinalIgnoreCase) -or
    -not (Test-Path -LiteralPath $captureScript)
) {
    throw "capture script must exist below the project directory"
}
$taskName = "WZSN-InteractiveScreenshot-$([guid]::NewGuid().ToString('N'))"
$outputParent = Split-Path -Parent $resolvedOutput
if (-not (Test-Path -LiteralPath $outputParent)) {
    New-Item -ItemType Directory -Force -Path $outputParent | Out-Null
}
Set-Content -LiteralPath (Join-Path $outputParent 'interactive-bridge-started.txt') -Value (Get-Date).ToString('o')

$action = New-ScheduledTaskAction `
    -Execute 'powershell.exe' `
    -Argument "-NoProfile -NonInteractive -ExecutionPolicy Bypass -File `"$captureScript`" -ProjectRoot `"$projectRoot`" -OutputPath `"$resolvedOutput`""
$principal = New-ScheduledTaskPrincipal -UserId $interactiveUser -LogonType Interactive -RunLevel Limited

try {
    Register-ScheduledTask -TaskName $taskName -Action $action -Principal $principal -Force | Out-Null
    Start-ScheduledTask -TaskName $taskName
    $task = Get-ScheduledTask -TaskName $taskName
    Set-Content -LiteralPath (Join-Path $outputParent 'interactive-bridge-task.txt') -Value ("State=$($task.State)")

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $resolvedOutput) {
            $item = Get-Item -LiteralPath $resolvedOutput
            if ($item.Length -gt 0) {
                Write-Output "SCREENSHOT_PATH=$resolvedOutput"
                Write-Output "INTERACTIVE_USER=$interactiveUser"
                Write-Output "TASK_NAME=$taskName"
                exit 0
            }
        }
        Start-Sleep -Milliseconds 250
    } while ((Get-Date) -lt $deadline)

    throw "interactive screenshot task did not produce output within $TimeoutSeconds seconds"
} finally {
    Unregister-ScheduledTask -TaskName $taskName -Confirm:$false -ErrorAction SilentlyContinue
}
