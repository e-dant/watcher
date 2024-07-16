"""
Tool for writing and computing sha256sums on files.
Particularly useful for verifying the integrity of build artifacts.
"""

import glob
import hashlib
import os
import sys


def sha256sum(file_path) -> str:
    """
    The sha256sum of a file
    """
    with open(file_path, "rb") as f:
        return hashlib.file_digest(f, "sha256").hexdigest()


def sha256sum_tree(path) -> dict:
    """
    Similar to mk_dot_sha256sums, but returns a mapping of
    file paths to their shasums, not file path.sha256sum to shasums.
    """
    if os.path.isfile(path):
        return {path: sha256sum(path)}
    tree = {}
    for root, dirs, files in os.walk(path):
        for p in dirs + files:
            tree.update(sha256sum_tree(os.path.join(root, p)))
    return tree


def mk_dot_sha256sums(path) -> None:
    """
    Adds the shasums of all the files in a directory to new files of
    the same name, but with a .sha256sum file extension.
    If the path given is a file, just writes that file's shasum to a
    file of the same name, but with a .sha256sum file extension.
    You get the drift.
    """
    for file, shasum in sha256sum_tree(path).items():
        with open(f"{file}.sha256sum", "w", encoding="utf-8") as f:
            f.write(shasum)


def rm_dot_sha256sums(dir_path) -> None:
    """
    Convenience function to remove all .sha256sum files in a directory,
    because if we don't we'll make a bunch of .sha256sum.sha256sum... files.
    """
    for file in glob.glob(f"{dir_path}/**/*.sha256sum", recursive=True):
        os.remove(file)


def _main() -> None:
    if len(sys.argv) < 2:
        print("Usage: python shasum.py <file or directory> [mk | rm | show]")
        sys.exit(1)
    path = sys.argv[1]
    op = sys.argv[2] if len(sys.argv) > 2 else "show"
    match op:
        case "mk":
            mk_dot_sha256sums(path)
        case "rm":
            rm_dot_sha256sums(path)
        case "show":
            for file, shasum in sha256sum_tree(path).items():
                print(f"{file}: {shasum}")
        case _:
            print("Invalid operation. Use mk, rm, or show.")
            sys.exit(1)


if __name__ == "__main__":
    _main()
