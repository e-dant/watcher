from __future__ import annotations
import ctypes
import os
import sys
from typing import Callable
from dataclasses import dataclass
from enum import Enum
from datetime import datetime

_LIB: ctypes.CDLL | None = None


class _CEvent(ctypes.Structure):
    _fields_ = [
        ("path_name", ctypes.c_char_p),
        ("effect_type", ctypes.c_int8),
        ("path_type", ctypes.c_int8),
        ("effect_time", ctypes.c_int64),
        ("associated_path_name", ctypes.c_char_p),
    ]


_CCallback = ctypes.CFUNCTYPE(None, _CEvent, ctypes.c_void_p)


def _lazy_static_solib_handle() -> ctypes.CDLL:
    def so_file_ending():
        match os.uname().sysname:
            case "Darwin":
                return "dylib"
            case "Windows":
                return "dll"
            case _:
                return "so"

    def libcwatcher_path():
        version = "0.12.0" # hook: tool/release
        libname = f"libcwatcher-{version}.{so_file_ending()}"
        heredir = os.path.dirname(os.path.abspath(__file__))
        return os.path.join(heredir, "lib", libname)

    global _LIB
    if _LIB is None:
        _LIB = ctypes.CDLL(libcwatcher_path())
        _LIB.wtr_watcher_open.argtypes = [ctypes.c_char_p, _CCallback, ctypes.c_void_p]
        _LIB.wtr_watcher_open.restype = ctypes.c_void_p
        _LIB.wtr_watcher_close.argtypes = [ctypes.c_void_p]
        _LIB.wtr_watcher_close.restype = ctypes.c_bool
    return _LIB


def _as_utf8(s: str | bytes | memoryview | None) -> str:
    if s is None:
        return ""
    elif isinstance(s, str):
        return s
    elif isinstance(s, memoryview):
        return s.tobytes().decode("utf-8")
    else:
        return s.decode("utf-8")


def _c_event_to_event(c_event: _CEvent) -> Event:
    path_name = _as_utf8(c_event.path_name)
    associated_path_name = _as_utf8(c_event.associated_path_name)
    effect_type = EffectType(c_event.effect_type)
    path_type = PathType(c_event.path_type)
    effect_time = datetime.fromtimestamp(c_event.effect_time / 1e9)
    return Event(path_name, effect_type, path_type, effect_time, associated_path_name)


class EffectType(Enum):
    RENAME = 0
    MODIFY = 1
    CREATE = 2
    DESTROY = 3
    OWNER = 4
    OTHER = 5


class PathType(Enum):
    DIR = 0
    FILE = 1
    HARD_LINK = 2
    SYM_LINK = 3
    WATCHER = 4
    OTHER = 5


@dataclass
class Event:
    path_name: str
    effect_type: EffectType
    path_type: PathType
    effect_time: datetime
    associated_path_name: str


class Watch:
    def __init__(self, path: str, callback: Callable[[Event], None]):
        def callback_bridge(event, _):
            callback(_c_event_to_event(event))

        self._lib = _lazy_static_solib_handle()
        self._path = path.encode("utf-8")
        self._c_callback = _CCallback(callback_bridge)
        self._watcher = self._lib.wtr_watcher_open(self._path, self._c_callback, None)
        if not self._watcher:
            if os.path.exists(path):
                raise RuntimeError(f"No such path: {path}")
            else:
                raise RuntimeError("Internal error while opening a watcher")

    def close(self):
        if self._watcher:
            if not self._lib.wtr_watcher_close(self._watcher):
                raise RuntimeError("Internal error while closing a watcher")
            self._watcher = None

    def __del__(self):
        self.close()

    def __deinit__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "."
    with Watch(path, print) as watcher:
        input()
