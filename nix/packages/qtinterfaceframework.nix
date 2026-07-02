{
  qtModule,
  qtbase,
  pkg-config,
  lib,
  stdenv,
  fetchFromGitHub,
  qtremoteobjects,
  python3,
  qface,
}: let
  # Create a Python environment with qface
  pythonWithQface = python3.withPackages (ps:
    with ps; [
      qface
      # Add other Python packages if needed
      # jinja2  # qface might need this
      # pyyaml
    ]);
in
  qtModule {
    pname = "qtinterfaceframework";
    version = "6.11.1";
    src = fetchFromGitHub {
      owner = "qt";
      repo = "qtinterfaceframework";
      rev = "v6.11.1";
      hash = "sha256-uenOlKvIfJHsZon+5uUU9ifMPW5J85wr1/5KsYfeNXM=";
      fetchSubmodules = true;
    };
    propagatedBuildInputs = [qtbase qtremoteobjects pythonWithQface];
    buildInputs = [
    ];
    nativeBuildInputs = [pkg-config];
  }
