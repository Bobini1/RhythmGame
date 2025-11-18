{ lib
, stdenv
, fetchFromGitHub
, ned14-outcome
, ned14-quickcpplib
, ned14-status-code
, byte-lite
, span-lite
, cmake
}:

stdenv.mkDerivation rec {
  pname = "ned14-llfio";
  version = "master";

  src = fetchFromGitHub {
    owner = "ned14";
    repo = "llfio";
    rev = "198be38faa0b05c09b8d75a95fbbc6e81dcb4e49";
    hash = "sha256-p6n+cQmtBe0GSO66I3TZQySSvcyS85kweTixjwllnLY=";
    fetchSubmodules = true;
  };

  nativeBuildInputs = [
    cmake
  ];

  propagatedBuildInputs = [
    ned14-outcome
    ned14-quickcpplib
    ned14-status-code
    byte-lite
    span-lite
  ];

  cmakeFlags = [
    "-DLLFIO_USE_EXPERIMENTAL_SG14_STATUS_CODE=ON"
  ];

  meta = with lib; {
    description = "Low-level file I/O library from ned14";
    homepage = "https://github.com/ned14/llfio";
    license = licenses.asl20;
    platforms = platforms.all;
    maintainers = [ maintainers.Bobini1 ];
  };
}
