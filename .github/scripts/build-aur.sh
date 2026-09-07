#!/usr/bin/env bash
set -euo pipefail

# Run as the unprivileged builder in a fresh Arch Linux container.
source_repo=$(realpath "${1:?Pass the bare repository containing the CI checkout}")
source_commit=$(git --git-dir="$source_repo" rev-parse HEAD)

export CMAKE_BUILD_PARALLEL_LEVEL=2
export VCPKG_MAX_CONCURRENCY=2
export MAKEFLAGS=-j2

mkdir -p "$HOME/aur"
cd "$HOME/aur"

# makepkg resolves official repository dependencies; these currently live in the AUR.
for dependency in miniaudio magic_enum sqlitecpp; do
  if pacman -Si "$dependency" > /dev/null 2>&1; then
    continue
  fi
  git clone --depth 1 "https://aur.archlinux.org/$dependency.git" "$dependency"
  (
    cd "$dependency"
    makepkg --syncdeps --install --noconfirm --needed --log
  )
done

git clone --depth 1 https://aur.archlinux.org/rhythmgame-git.git
cd rhythmgame-git
git log -1 --format='AUR recipe: %H %s'

# Keep the published recipe and its checksums, but build the commit under test.
# Fail if the recipe layout changes instead of silently replacing another source.
upstream_source=$(makepkg --printsrcinfo | awk '$1 == "source" && !found { print $3; found = 1 }')
if [[ "$upstream_source" != 'RhythmGame::git+https://github.com/Bobini1/RhythmGame.git' ]]; then
  printf 'Unexpected RhythmGame AUR source: %s\n' "$upstream_source" >&2
  exit 1
fi
printf '\n# Use the CI checkout instead of the upstream branch tip.\nsource[0]=%q\n' \
  "RhythmGame::git+file://$source_repo#commit=$source_commit" >> PKGBUILD

printf 'Building RhythmGame commit %s\n' "$source_commit"
makepkg --syncdeps --noconfirm --log
