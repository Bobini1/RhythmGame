$ErrorActionPreference = 'Stop'

$manifest = Get-Content "$PSScriptRoot/../../vcpkg.json" -Raw | ConvertFrom-Json
$baseline = $manifest.'builtin-baseline'
if ($baseline -notmatch '^[0-9a-f]{40}$') {
  throw 'vcpkg.json must specify a full builtin-baseline commit.'
}
if (-not $env:VCPKG_ROOT) {
  throw 'VCPKG_ROOT is not set.'
}

# Hosted runners can have an older checkout than the manifest requires.
git -C $env:VCPKG_ROOT fetch --no-tags origin $baseline
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
git -C $env:VCPKG_ROOT checkout --detach $baseline
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($IsWindows) {
  & "$env:VCPKG_ROOT/bootstrap-vcpkg.bat" -disableMetrics
} else {
  & "$env:VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
