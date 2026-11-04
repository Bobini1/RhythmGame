$ErrorActionPreference = 'Stop'

if ($env:GITHUB_ACTIONS -ne 'true') {
  throw 'This script replaces the disposable vcpkg checkout on a CI runner.'
}

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
# Runner images can include changes outside their recorded Git revision.
git -C $env:VCPKG_ROOT checkout --force --detach $baseline
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($IsWindows) {
  & "$env:VCPKG_ROOT/bootstrap-vcpkg.bat" -disableMetrics
} else {
  & "$env:VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
