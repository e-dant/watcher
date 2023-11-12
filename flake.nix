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
            , pname
            , buildcfg
            , targets
            , components
            # TODO Just make this a regular cmake target
            , installBashScriptName ? ""
            , installBashScript ? ""
            }: pkgs.stdenv.mkDerivation {
              inherit src pname buildcfg targets components;
              version = "0.9.5"; # hook: tool/release
              nativeBuildInputs = build_deps ++ maybe_sys_deps ++ [ snitch ];
              env.WTR_WATCHER_USE_SYSTEM_SNITCH = 1;
              buildPhase = ''
                for target in $targets
                do cmake --build . --target "$target" --config "$buildcfg" &
                done
                wait
              '';
              installPhase = ''
                for component in $components
                do cmake --install . --prefix "$out" --config "$buildcfg" --component "$component"
                done
                if [ -n "${installBashScript}" ]
                then
                  echo "${installBashScript}" | tee -a "$out/bin/${installBashScriptName}"
                  chmod +x "$out/bin/${installBashScriptName}"
                fi
              '';
            };

          watcher = mkWatcherDerivation {
            src        = self;
            pname      = "wtr.watcher";
            buildcfg   = "Release";
            targets    = [ "wtr.watcher" "wtr.hdr_watcher" ];
            components = [ "bin" "include" ];
          };
          watcher-cli = mkWatcherDerivation {
            src        = self;
            pname      = "wtr.watcher";
            buildcfg   = "Release";
            targets    = [ "wtr.watcher" ];
            components = [ "bin" ];
          };
          watcher-hdr = mkWatcherDerivation {
            src        = self;
            pname      = "exit";
            buildcfg   = "Release";
            targets    = [ "wtr.hdr_watcher" ];
            components = [ "include" ];
          };
          watcher-test = mkWatcherDerivation {
            src        = self;
            pname      = "wtr.test_watcher.allsan";
            buildcfg   = "Debug";
            targets    = [ "wtr.test_watcher" "wtr.test_watcher.asan" "wtr.test_watcher.msan" "wtr.test_watcher.tsan" "wtr.test_watcher.ubsan" ];
            components = [ "test-bin" ];
            installBashScriptName = "wtr.test_watcher.allsan";
            installBashScript = ''
              #!/usr/bin/env bash
              PATH="$PATH:$out/bin"
              mkdir -p /tmp/watcher-test
              cd /tmp/watcher-test
              if command -v wtr.test_watcher      ; then wtr.test_watcher      ; fi
              if command -v wtr.test_watcher.asan ; then wtr.test_watcher.asan ; fi
              if command -v wtr.test_watcher.msan ; then wtr.test_watcher.msan ; fi
              if command -v wtr.test_watcher.tsan ; then wtr.test_watcher.tsan ; fi
              if command -v wtr.test_watcher.ubsan; then wtr.test_watcher.ubsan; fi
              rm -rf /tmp/watcher-test
            '';
          };

          # Bring watcher's tree and all of its dependencies in via inputsFrom,
          # and pre-build watcher via buildInputs so that we can use it right away.
          watcher-devshell = pkgs.mkShell {
            inputsFrom = [ watcher-cli watcher-hdr watcher-test ];
            buildInputs = [ watcher ];
          };

        in {
          packages = { inherit watcher watcher-cli watcher-hdr watcher-test; };
          defaultApp = flake-utils.lib.mkApp { drv = watcher-cli; };
          defaultPackage = watcher;
          devShell = watcher-devshell;
        }
    )
  );
}
