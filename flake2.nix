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
          lib = pkgs.lib;
          darwin = import nixpkgs.nixosModules.config.darwin {
            config = {};
          };
          stdenv = pkgs.stdenv;
          wtr-watcher = (
            with pkgs;
            pkgs ++
            lib.optionals stdenv.isDarwin (with darwin.apple_sdk.frameworks; [
     Cocoa
     CoreServices
   ])
   ;
            stdenv.mkDerivation {
              pname = "wtr-watcher";
              version = "0.8.8";
              src = self;
              # cacert and git for cmake fetchcontent
              nativeBuildInputs = [ cmake cacert git ];
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
