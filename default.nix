with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "gridware-env";
  nativeBuildInputs = [ python3 python38Packages.setuptools python38Packages.protobuf python38Packages.grpcio-tools];
}
