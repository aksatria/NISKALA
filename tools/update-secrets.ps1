# update-secrets.ps1
# Update kredensial NISKALA di src/secrets.h (dan opsional server/config.php).
# Cara pakai:
#   powershell -ExecutionPolicy Bypass -File tools\update-secrets.ps1
#   powershell -ExecutionPolicy Bypass -File tools\update-secrets.ps1 -SyncServer

param(
  [switch]$SyncServer
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$SecretsFile = Join-Path $Root 'src\secrets.h'
$ConfigFile  = Join-Path $Root 'server\config.php'

function Get-Define([string]$file, [string]$name) {
  if (-not (Test-Path -LiteralPath $file)) { return $null }
  $raw = [System.IO.File]::ReadAllText($file)
  $pat = '(?m)^#define\s+' + [regex]::Escape($name) + '\s+"([^"]*)"'
  $m = [regex]::Match($raw, $pat)
  if ($m.Success) { return $m.Groups[1].Value }
  return $null
}

function Prompt-Value([string]$label, [string]$current, [bool]$secure) {
  $shown = if ([string]::IsNullOrEmpty($current)) { '(kosong)' } else { $current }
  if ($secure) {
    $sec = Read-Host ("$label [saat ini: $shown] (Enter = biarkan)") -AsSecureString
    if ($null -eq $sec -or $sec.Length -eq 0) { return $current }
    $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($sec)
    try { return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr) }
    finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr) }
  } else {
    $v = Read-Host ("$label [saat ini: $shown] (Enter = biarkan)")
    if ([string]::IsNullOrEmpty($v)) { return $current }
    return $v
  }
}

$curSsid = Get-Define $SecretsFile 'NISKALA_WIFI_SSID'
$curPass = Get-Define $SecretsFile 'NISKALA_WIFI_PASS'
$curKey  = Get-Define $SecretsFile 'NISKALA_SYNC_API_KEY'

Write-Host ''
Write-Host '=== NISKALA: update kredensial ===' -ForegroundColor Cyan

$ssid = Prompt-Value 'WiFi SSID' $curSsid $false
$pass = Prompt-Value 'WiFi Password' $curPass $true
$key  = Prompt-Value 'API Key' $curKey $false

$content = @"
#ifndef NISKALA_SECRETS_H
#define NISKALA_SECRETS_H

// File ini bersifat LOKAL dan TIDAK di-commit ke Git (sudah di .gitignore).
// Nilai di bawah dihasilkan oleh tools/update-secrets.ps1.

#define NISKALA_WIFI_SSID    "$ssid"
#define NISKALA_WIFI_PASS    "$pass"
#define NISKALA_SYNC_API_KEY "$key"

#endif
"@

$content = $content.TrimEnd() + "`n"
[System.IO.File]::WriteAllText($SecretsFile, $content)

Write-Host ''
Write-Host "OK: $SecretsFile diperbarui." -ForegroundColor Green

if ($SyncServer -or $key -ne $curKey) {
  $do = $SyncServer
  if (-not $do) {
    $ans = Read-Host 'API key berubah. Sinkronkan ke server/config.php juga? (y/N)'
    $do = ($ans -match '^(y|Y)$')
  }
  if ($do) {
    $cfg = @"
<?php
// File ini bersifat LOKAL dan TIDAK di-commit ke Git (sudah di .gitignore).
define('API_KEY', '$key');
"@
    $cfg = $cfg.TrimEnd() + "`n"
    [System.IO.File]::WriteAllText($ConfigFile, $cfg)
    Write-Host "OK: $ConfigFile diperbarui." -ForegroundColor Green
  }
}

Write-Host ''
Write-Host 'Pengingat: flash ulang firmware, atau set ulang via Web Remote > Config.' -ForegroundColor DarkGray