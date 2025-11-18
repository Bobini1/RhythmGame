{ qtModule,
  qtbase,
  pkg-config,
  lib,
  stdenv,
  fetchFromGitHub,
  qtremoteobjects,
  python3,
  qface
}:
let
  # Create a Python environment with qface
  pythonWithQface = python3.withPackages (ps: with ps; [
    qface
    # Add other Python packages if needed
    # jinja2  # qface might need this
    # pyyaml
  ]);
in
qtModule {
  pname = "qtinterfaceframework";
  version = "6.10.0";
  src = fetchFromGitHub {
    owner = "qt";
    repo = "qtinterfaceframework";
    rev = "v6.10.0";
    hash = "sha256-4baRx05ilLq62ZlIWcnCmwuD8QRZNileR3bjTUibC1s=";
    fetchSubmodules = true;
  };
  propagatedBuildInputs = [ qtbase qtremoteobjects pythonWithQface ];
  buildInputs = [
  ];
  nativeBuildInputs = [ pkg-config ];
}
