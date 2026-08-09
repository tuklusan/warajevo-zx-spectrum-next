# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

[CmdletBinding(DefaultParameterSetName = 'CommandText')]
param(
    [Parameter(Mandatory = $true, ParameterSetName = 'CommandText')]
    [string]$CommandText,

    [Parameter(Mandatory = $true, ParameterSetName = 'FilePath')]
    [string]$FilePath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$tokens = $null
$errors = $null

if ($PSCmdlet.ParameterSetName -eq 'CommandText') {
    [System.Management.Automation.Language.Parser]::ParseInput(
        $CommandText,
        [ref]$tokens,
        [ref]$errors
    ) | Out-Null
} else {
    $resolvedPath = (Resolve-Path -LiteralPath $FilePath).Path
    [System.Management.Automation.Language.Parser]::ParseFile(
        $resolvedPath,
        [ref]$tokens,
        [ref]$errors
    ) | Out-Null
}

if ($errors.Count -gt 0) {
    foreach ($parseError in $errors) {
        Write-Error $parseError.Message
    }

    exit 1
}

Write-Output 'PowerShell syntax parse passed'
