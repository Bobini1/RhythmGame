$ErrorActionPreference = 'Continue'

function Escape-WorkflowCommandData {
  param([string]$Value)

  return $Value.
    Replace('%', '%25').
    Replace("`r", '%0D').
    Replace("`n", '%0A')
}

function Write-LogAnnotation {
  param(
    [string]$Title,
    [string]$Path
  )

  if (-not (Test-Path -LiteralPath $Path)) {
    return
  }

  $lines = Get-Content -LiteralPath $Path -Tail 80 -ErrorAction SilentlyContinue
  if (-not $lines) {
    return
  }

  $message = $lines -join "`n"
  if ($message.Length -gt 6000) {
    $message = $message.Substring($message.Length - 6000)
  }

  $escapedTitle = Escape-WorkflowCommandData $Title
  $escapedMessage = Escape-WorkflowCommandData $message
  Write-Output "::error title=$escapedTitle::$escapedMessage"
}

$highPriorityLogs = @()
$vcpkgLogs = @()

$cmakeLogs = @(
  'configure.log',
  'build/CMakeFiles/CMakeConfigureLog.yaml',
  'build/CMakeFiles/CMakeError.log',
  'build/CMakeFiles/CMakeOutput.log',
  'build/coverage/CMakeFiles/CMakeConfigureLog.yaml',
  'build/coverage/CMakeFiles/CMakeError.log',
  'build/coverage/CMakeFiles/CMakeOutput.log',
  'build/sanitize/CMakeFiles/CMakeConfigureLog.yaml',
  'build/sanitize/CMakeFiles/CMakeError.log',
  'build/sanitize/CMakeFiles/CMakeOutput.log'
)

foreach ($path in $cmakeLogs) {
  if (Test-Path -LiteralPath $path) {
    $highPriorityLogs += Get-Item -LiteralPath $path
  }
}

$manifestLogs = @(
  'build/vcpkg-manifest-install.log',
  'build/coverage/vcpkg-manifest-install.log',
  'build/sanitize/vcpkg-manifest-install.log'
)

foreach ($path in $manifestLogs) {
  if (Test-Path -LiteralPath $path) {
    $vcpkgLogs += Get-Item -LiteralPath $path
  }
}

$buildtreeRoots = @(
  (Join-Path $PWD.Path '.vcpkg-buildtrees')
)

if ($env:VCPKG_ROOT) {
  $buildtreeRoots += Join-Path $env:VCPKG_ROOT 'buildtrees'
}

foreach ($buildtrees in $buildtreeRoots) {
  if (-not (Test-Path -LiteralPath $buildtrees)) {
    continue
  }

  $recentBuildtrees = Get-ChildItem -LiteralPath $buildtrees -Directory -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 8

  foreach ($directory in $recentBuildtrees) {
    $vcpkgLogs += Get-ChildItem -LiteralPath $directory.FullName -File -Include *.log,*.err,*.out -ErrorAction SilentlyContinue
  }
}

$candidateLogs = @(
  $highPriorityLogs |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 8
  $vcpkgLogs |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 4
) | Select-Object -Unique

if (-not $candidateLogs) {
  Write-Output '::error title=Configure failed::No vcpkg or CMake configure logs were found.'
  exit 0
}

foreach ($log in $candidateLogs) {
  Write-LogAnnotation -Title "Configure log: $($log.Name)" -Path $log.FullName
}
