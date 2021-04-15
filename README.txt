1. Requirements
Python 3 and the following python packages:
  - setuptools
  - protobuf
  - grpcio-tools

make and g++ need to be installed as well.

Alternatively if you have nix, the following will set up an environment for you:
`` nix-shell

2. Setting up for the first build
Clone nanopb:
`` git submodule update --init --recursive

3. Configuring
Tunables are stored in common.h

4. Building
(optional) Compile pb headers, replace those provided by the repo:
`` make build-pb

To compile, simply:
`` make

5. Running
`` ./fw_update_sim