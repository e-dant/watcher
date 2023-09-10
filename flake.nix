{
  description =
    "Filesystem watcher. Works anywhere. Simple, efficient and friendly.";
  inputs =
  { nixpkgs = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    flake-utils = { url = "github:numtide/flake-utils"; };
  };
  outputs =
  { self
  , nixpkgs
  , flake-utils
  , ...
  }:
    flake-utils.lib.eachDefaultSystem
    ( system:
        let
          pkgs = import nixpkgs { inherit system; };
          deps = (
            with pkgs;
            with darwin.apple_sdk.frameworks;
            [ cacert  # For github.com
              clang   # For building
              cmake   # For building
              git     # For cmake fetchcontent
              python3 # Optional; For snitch testing framework
            ]
            # If on Darwin, we need this to use FSEvents, dispatch, etc.
            ++ lib.optionals stdenv.isDarwin([ CoreServices ])
          );
          watcher = (
            pkgs.stdenv.mkDerivation {
              pname = "wtr.watcher";
              version = "0.8.8"; # hook: tool/release
              src = self;
              nativeBuildInputs = deps;
              # build phase should maybe also run unit tests? no...
              # I think we should make a test target to this flake
              buildPhase = ''
                cmake --build . --target wtr.watcher --config Release
                cmake --build . --target wtr.hdr_watcher
              '';
              installPhase = ''
                cmake --install . --prefix "$out" --config Release --component bin
                cmake --install . --prefix "$out" --config Release --component include
              '';
            }
          );
        in rec {
          defaultApp = flake-utils.lib.mkApp { drv = defaultPackage; };
          defaultPackage = watcher;
          devShell = pkgs.mkShell {
            buildInputs = [ watcher ];
          };
        }
    );
}
