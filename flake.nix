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
          # CMake to build; cacert and git for CMake's fetchcontent
          build_deps = (with pkgs; [ cmake cacert git ]);
          # Darwin needs some help to use FSEvents, dispatch, etc.
          maybe_sys_deps = (
            with pkgs;
            lib.optionals stdenv.isDarwin([darwin.apple_sdk.frameworks.CoreServices])
          );
          wtr-watcher = (
            with pkgs;
            stdenv.mkDerivation {
              pname = "wtr-watcher";
              version = "0.8.8";
              src = self;
              nativeBuildInputs = build_deps ++ maybe_sys_deps;
              buildPhase = ''
                cmake \
                  --build . \
                  --target wtr.watcher \
                  --config Release \
                  -j8
              '';
              installPhase = ''
                mkdir -p "$out/bin" \
                && mv wtr.watcher "$out/bin/wtr-watcher" \
                && mkdir -p "$out/include" \
                && mv ../include/wtr "$out/include/wtr" \
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
