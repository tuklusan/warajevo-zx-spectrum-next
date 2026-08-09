# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$projectRoot = (Get-Location).Path
$normalizedProjectRoot = [System.IO.Path]::GetFullPath($projectRoot)
$projectRootPrefix = if ($normalizedProjectRoot.EndsWith('\')) {
    $normalizedProjectRoot
} else {
    $normalizedProjectRoot + '\'
}
$resolvedOutput = [System.IO.Path]::GetFullPath(
    $(if ([System.IO.Path]::IsPathRooted($OutputPath)) {
        $OutputPath
    } else {
        Join-Path $projectRoot $OutputPath
    })
)

if (
    $resolvedOutput -ne $normalizedProjectRoot -and
    -not $resolvedOutput.StartsWith($projectRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)
) {
    throw "output path must remain within the project directory: $OutputPath"
}

$parentPath = Split-Path -Parent $resolvedOutput
if (-not (Test-Path -LiteralPath $parentPath)) {
    New-Item -ItemType Directory -Force -Path $parentPath | Out-Null
}

$bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
$bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)

try {
    $graphics.CopyFromScreen($bounds.X, $bounds.Y, 0, 0, $bitmap.Size)
    $bitmap.Save($resolvedOutput, [System.Drawing.Imaging.ImageFormat]::Png)
} catch {
    throw "interactive desktop screenshot capture failed in the current session: $($_.Exception.Message)"
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
}

Write-Output "SCREENSHOT_PATH=$resolvedOutput"
Write-Output "BOUNDS=$($bounds.ToString())"
