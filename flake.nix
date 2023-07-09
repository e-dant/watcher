{
  description =
    "Filesystem watcher. Works anywhere. Simple, efficient and friendly.";
  inputs = {
    nixpkgs = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    flake-utils = { url = "github:numtide/flake-utils"; };
  };
  outputs = {
    self,
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachDefaultSystem(
      system:
        let
          pkgs = import nixpkgs { inherit system; };
          wtr-watcher = (
            with pkgs;
            stdenv.mkDerivation {
              pname = "wtr-watcher";
              version = "0.8.8";
              src = self;
              nativeBuildInputs = [ cmake cacert git ]; # cacert and git for cmake fetchcontent
              buildPhase = ''
                cmake \
                  --build . \
                  --target wtr.watcher \
                  --config Release \
                  -j8
              '';
              installPhase = ''
                mkdir -p "$out/bin" \
                && mv wtr.watcher "$out/bin/wtr-watcher"
              '';
            }
          );
        in rec {
          defaultApp = flake-utils.lib.mkApp { drv = defaultPackage; };
          defaultPackage = wtr-watcher;
          devShell = pkgs.mkShell {
            buildInputs = [ wtr-watcher ];
          };
        }
    );
}
