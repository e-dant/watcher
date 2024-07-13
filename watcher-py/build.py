import os
import shutil
import glob
from os.path import dirname, realpath
from subprocess import check_call

HERE_DIR = realpath(dirname(__file__))
LIBCWATCHER_SRC_DIR = realpath(os.path.join(HERE_DIR, "..", "libcwatcher"))
BUILD_DIR = os.path.join(HERE_DIR, "build")
INSTALL_DIR = os.path.join(HERE_DIR, "watcher", "lib")


def build():
    def is_shared_lib(lib):
        return lib.endswith(".so") or lib.endswith(".dylib") or lib.endswith(".dll")

    print(f"{HERE_DIR=}")
    print(f"{LIBCWATCHER_SRC_DIR=}")
    print(f"{BUILD_DIR=}")
    print(f"{INSTALL_DIR=}")
    if not os.path.exists(BUILD_DIR):
        check_call(["meson", "setup", BUILD_DIR, LIBCWATCHER_SRC_DIR])
    check_call(["meson", "compile", "-C", BUILD_DIR])
    # Because Meson does not want to install the library in the right place,
    # regardless of the destdir, and I don't want hardcodes paths in the build file.
    os.makedirs(INSTALL_DIR, exist_ok=True)
    for lib in filter(is_shared_lib, glob.glob(os.path.join(BUILD_DIR, "lib*"))):
        print(f"Copying {lib} to {INSTALL_DIR}")
        shutil.copy(lib, INSTALL_DIR)
