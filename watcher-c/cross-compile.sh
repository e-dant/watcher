#! /usr/bin/env bash
set -e
[ "${1:-}" = --clean ] && {
  rm -rf "$(dirname "$0")/out"
  exit
}
[ "${1:-}" = --show-artifacts ] && {
  for pattern in '*.a' '*.so' '*.dylib'
  do find "$(dirname "$0")/out" -type f -name "$pattern"
  done
  exit
}
[ "${1:---build}" = --build ] && {
  [ -d "$(dirname "$0")/out" ] || mkdir "$(dirname "$0")/out"
  cd "$(dirname "$0")"
  linux-cross-compilation-containers/build-containers.sh
  [ -f out/meson.build ] || cp meson.build out
  [ -d out/src ] || cp -r src out
  [ -d out/include ] || cp -r include out
  (
    cd out
    SRC=$PWD
    docker run --platform linux/amd64 --rm -v "$SRC:/src" meson-builder-x86_64-unknown-linux-gnu:latest
    docker run --platform linux/arm64 --rm -v "$SRC:/src" meson-builder-aarch64-unknown-linux-gnu:latest
    docker run --platform linux/arm/v7 --rm -v "$SRC:/src" meson-builder-armv7-unknown-linux-gnueabihf:latest
  )
  [ "$(uname)" = Darwin ] && {
    [ -d out/x86_64-apple-darwin ] || meson setup --cross-file cross-files/x86_64-apple-darwin.txt out/x86_64-apple-darwin
    [ -d out/aarch64-apple-darwin ] || meson setup --cross-file cross-files/aarch64-apple-darwin.txt out/aarch64-apple-darwin
    meson compile -C out/x86_64-apple-darwin
    meson compile -C out/aarch64-apple-darwin
  }
  echo '-------- Artifacts --------'
  for pattern in '*.a' '*.so' '*.dylib'
  do find out -type f -name "$pattern" -exec file {} \;
  done
  echo '~~~~~~~~~~~~~~~~~~~~~~~~~~~'
  for pattern in '*.a' '*.so' '*.dylib'
  do find out -type f -name "$pattern"
  done
  echo '---------------------------'
}
