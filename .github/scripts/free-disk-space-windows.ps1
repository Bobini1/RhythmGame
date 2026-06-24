$ErrorActionPreference = 'Continue'

Get-PSDrive -PSProvider FileSystem

$paths = @(
  $env:ANDROID_HOME,
  $env:ANDROID_NDK_HOME,
  "$env:ProgramFiles\Android",
  "${env:ProgramFiles(x86)}\Android",
  "$env:ProgramFiles\Microsoft SDKs\Azure",
  "${env:ProgramFiles(x86)}\Microsoft SDKs\Azure"
)

$paths |
  Where-Object { $_ -and (Test-Path -LiteralPath $_) } |
  ForEach-Object {
    Write-Host "Removing $_"
    Remove-Item -LiteralPath $_ -Recurse -Force -ErrorAction SilentlyContinue
  }

Get-PSDrive -PSProvider FileSystem
