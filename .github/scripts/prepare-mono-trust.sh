#!/usr/bin/env bash
set -euo pipefail

# Mono 6.14 in Ubuntu 26.04 omits cert-sync and its automatic trust-store setup.
# https://bugs.debian.org/1151503
trust_dir="${XDG_CONFIG_HOME:-$HOME/.config}/.mono/new-certs"
mkdir -p "$trust_dir"
if [[ ! -e "$trust_dir/Trust" ]]; then
  ln -s /etc/ssl/certs "$trust_dir/Trust"
fi
