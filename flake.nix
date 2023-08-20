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
          build_deps = (
            with pkgs;
            [ cacert  # For github.com
              clang   # For building
              cmake   # For building
              git     # For cmake fetchcontent
              python3 # Optional; For snitch testing framework
            ]
          );
          # Darwin needs some help to use FSEvents, dispatch, etc.
          maybe_sys_deps = (
            with pkgs;
            lib.optionals stdenv.isDarwin([darwin.apple_sdk.frameworks.CoreServices])
          );
          watcher = (
            with pkgs;
            stdenv.mkDerivation {
              pname = "watcher";
              version = "0.8.8"; # hook: tool/release
              src = self;
              nativeBuildInputs = build_deps ++ maybe_sys_deps;
              # build phase should maybe also run unit tests? no...
              # I think we should make a test target to this flake
              buildPhase = "cmake --build . --target wtr.watcher --config Release -j3";
              installPhase = ''
                cmake --install . --prefix "$out" --config Release --component bin
                cmake --install . --prefix "$out" --config Release --component include
                echo 'checking installation paths...'
                ls "$out/bin"
                ls "$out/include/wtr"
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
