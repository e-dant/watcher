{
  description =
    "Filesystem watcher. Works anywhere. Simple, efficient and friendly.";

  inputs =
    { nixpkgs     = { url = "github:nixos/nixpkgs/nixos-unstable"; };
      flake-utils = { url = "github:numtide/flake-utils"; };
      snitch-repo = { url = "github:cschreib/snitch"; flake = false; };
    };

  outputs =
    { self
    , nixpkgs
    , flake-utils
    , snitch-repo
    , ...
    }:
    (
      flake-utils.lib.eachDefaultSystem(system:
        let
          pkgs = import nixpkgs { inherit system; };

          watcher_targets = {
            cli = "wtr.watcher";
            hdr = "wtr.hdr_watcher";
            test = "wtr.test_watcher";
            bench = "wtr.bench_watcher";
          };
          watcher_components = {
            bin = "bin";
            include = "include";
          };
          watcher_buildcfgs = {
            rel = "Release";
            deb = "Debug";
            rwd = "RelWithDebInfo";
            msr = "MinSizeRel";
          };

          build_deps = [ pkgs.clang pkgs.cmake ];
          # If on Darwin, we need this to use FSEvents, dispatch, etc.
          maybe_sys_deps = pkgs.lib.optionals pkgs.stdenv.isDarwin [ pkgs.darwin.apple_sdk.frameworks.CoreServices ];

          snitch = pkgs.stdenv.mkDerivation {
            pname = "snitch";
            version = "0.1.0";
            src = snitch-repo;
            nativeBuildInputs = build_deps;
            buildPhase = ''cmake --build .'';
            installPhase = ''cmake --install . --prefix "$out"'';
          };

          mkWatcherDerivation =
            { src
            , buildcfg
            , targets
            , components
            }: pkgs.stdenv.mkDerivation {
              inherit src buildcfg targets components;
              pname = "wtr.watcher";
              version = "0.9.2"; # hook: tool/release
              nativeBuildInputs = build_deps ++ maybe_sys_deps ++ [ snitch ];
              env.WTR_WATCHER_USE_SYSTEM_SNITCH = 1;
              buildPhase = ''for target in $targets; do cmake --build . --target "$target" --config "$buildcfg"; done'';
              installPhase = ''for component in $components; do cmake --install . --prefix "$out" --config "$buildcfg" --component "$component"; done'';
            };

          watcher = mkWatcherDerivation {
            src = self;
            buildcfg = watcher_buildcfgs.rel;
            targets = [ watcher_targets.cli watcher_targets.hdr ];
            components = [ watcher_components.bin watcher_components.include ];
          };

          watcher-cli = mkWatcherDerivation {
            src = self;
            buildcfg = watcher_buildcfgs.rel;
            targets = [ watcher_targets.cli ];
            components = [ watcher_components.bin ];
          };

          watcher-hdr = mkWatcherDerivation {
            src = self;
            buildcfg = watcher_buildcfgs.rel;
            targets = [ watcher_targets.hdr ];
            components = [ watcher_components.include ];
          };

          watcher-test = mkWatcherDerivation {
            src = self;
            buildcfg = watcher_buildcfgs.deb;
            targets = [ watcher_targets.test ];
            components = [ watcher_components.bin ];
          };

          watcher-bench = mkWatcherDerivation {
            src = self;
            buildcfg = watcher_buildcfgs.rel;
            targets = [ watcher_targets.bench ];
            components = [ watcher_components.bin ];
          };

          # Bring watcher's tree and all of its dependencies in via inputsFrom,
          # and pre-build watcher via buildInputs so that we can use it right away.
          watcher-devshell = pkgs.mkShell {
            #inputsFrom = [ watcher ];
            #buildInputs = [ watcher ];
            inputsFrom = [ watcher-cli watcher-hdr watcher-test watcher-bench ];
            buildInputs = [ watcher ];
          };

        in {
          defaultApp = flake-utils.lib.mkApp { drv = watcher; };
          defaultPackage = watcher;
          devShell = watcher-devshell;
        }
    )
  );
}
