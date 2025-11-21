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

      qface = pkgs.callPackage ./nix/packages/python-qface.nix {};

      qtinterfaceframework = pkgs.kdePackages.callPackage ./nix/packages/qtinterfaceframework.nix {
        inherit qface;
        inherit stdenv;
      };
      ned14-llfio = pkgs.callPackage ./nix/packages/ned14-llfio.nix {
        inherit (nur-foolnotion) ned14-quickcpplib ned14-outcome ned14-status-code byte-lite span-lite;
        inherit stdenv;
      };

      nur-foolnotion = pkgs.nur.repos.foolnotion;
    in {
      packages = {
        default = self.packages.${system}.rhythmgame;
        rhythmgame = pkgs.kdePackages.callPackage ./nix/packages/rhythmgame.nix {
          inherit qtinterfaceframework ned14-llfio;
          inherit (nur-foolnotion) lexy;
          inherit stdenv;
        };
        inherit qtinterfaceframework ned14-llfio;
      };

      devShells.default = pkgs.kdePackages.callPackage ./nix/shells/default.nix {
        inherit qtinterfaceframework ned14-llfio;
        inherit (nur-foolnotion) lexy;
        inherit (pkgs.kdePackages) qtdeclarative qtsvg qtshadertools qtwayland qtmultimedia qttools;
        mkShell = pkgs.mkShell.override {inherit stdenv;};
      };

      # For nix build
      defaultPackage = self.packages.${system}.default;

      # For nix develop
      devShell = self.devShells.${system}.default;
    });
}
