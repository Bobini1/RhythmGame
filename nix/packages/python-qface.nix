{
  lib,
  python3,
  fetchPypi,
}:
python3.pkgs.buildPythonPackage rec {
  pname = "qface";
  version = "2.0.13";

  src = fetchPypi {
    inherit pname version;
    sha256 = "e47be09989e3bf1c3201740501a07d9cd631fb29fb442445e343c94af7b480cb";
  };

  propagatedBuildInputs = with python3.pkgs; [
    antlr4-python3-runtime
    argh
    click
    coloredlogs
    jinja2
    markupsafe
    pyyaml
    six
    watchdog
  ];

  pyproject = true;
  build-system = with python3.pkgs; [
    setuptools
  ];

  nativeCheckInputs = with python3.pkgs; [
    pytest
    pytest-cov
  ];

  # Run tests (set to false if they fail or take too long)
  doCheck = true;

  pythonImportsCheck = ["qface"];

  meta = with lib; {
    description = "QFace is a generator framework based on a common modern IDL";
    homepage = "https://github.com/Pelagicore/qface";
    license = licenses.mit;
    maintainers = with maintainers; [Bobini1];
  };
}
