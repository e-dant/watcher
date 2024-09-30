# Using the watcher library from C

The watcher can also be used from C by including the `watcher-c.h` file on compilation and in your project.

You can cross compile the library via the `watcher-c/cross-compile.sh` script or download the precompiled library from the releases.

### Compiling the library from source

If you want to compile the library yourself you can follow the following example:

First check out the `next` branch, compile a shared `libwatcher-c.so` library with `gcc` and install
it in the `/usr/local/lib` directory:

```bash
curl -L https://github.com/e-dant/watcher/archive/refs/heads/next.tar.gz | tar xz
cd watcher-next/watcher-c
gcc -o libwatcher-c.so ./src/watcher-c.cpp -I ./include -I ../include -std=c++17 -O3 -Wall -Wextra -fPIC -shared
cp libwatcher-c.so /usr/local/lib/libwatcher-c.so
ldconfig
```

### Using the library in your C code

After installing the library, copy the `watcher-c.h` file to your project and include it in your C code:

```c
#include "watcher-c.h"
```

You will then be able to use the `wtr_watcher_open` and `wtr_watcher_close` functions by linking against the library (`-lwatcher-c`).

`wtr_watcher_close` will always return `false` if the watcher is closed before the first `@live` event has been sent.

Note that the watcher depends on the c++ standard library.