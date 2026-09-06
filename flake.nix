{
  description = "A customizable BMS player for Windows and Linux";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nur.url = "github:nix-community/NUR";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    nur,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
        overlays = [
          nur.overlays.default
          (import ./nix/overlays/stb.nix)
        ];
      };

      stdenv = pkgs.gcc15Stdenv;

      libremidi = pkgs.libremidi.overrideAttrs (_: rec {
        version = "5.4.3";
        src = pkgs.fetchFromGitHub {
          owner = "jcelerier";
          repo = "libremidi";
          rev = "v${version}";
          hash = "sha256-p1abtxCJBwOt2VqKh85sF3yP4ekmwzTsden4XoMKDos=";
        };
      });

      ned14-llfio = pkgs.callPackage ./nix/packages/ned14-llfio.nix {
        inherit (nur-foolnotion) ned14-quickcpplib ned14-outcome ned14-status-code byte-lite span-lite;
        inherit stdenv;
      };

      nur-foolnotion = pkgs.nur.repos.foolnotion;
    in {
      packages = {
        default = self.packages.${system}.rhythmgame;
        rhythmgame = pkgs.kdePackages.callPackage ./nix/packages/rhythmgame.nix {
          inherit libremidi ned14-llfio;
          lexy = nur-foolnotion.foonathan-lexy;
          inherit stdenv;
        };
        inherit ned14-llfio;
      };

      devShells.default = pkgs.kdePackages.callPackage ./nix/shells/default.nix {
        inherit libremidi ned14-llfio;
        lexy = nur-foolnotion.foonathan-lexy;
        inherit (pkgs.kdePackages) qtdeclarative qtwebsockets qtsvg qtshadertools qtwayland qtmultimedia qttools qtkeychain;
        mkShell = pkgs.mkShell.override {inherit stdenv;};
      };

      # For nix build
      defaultPackage = self.packages.${system}.default;

      # For nix develop
      devShell = self.devShells.${system}.default;
    });
}
