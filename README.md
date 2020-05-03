### epx-c WIP

distributed(epaxos) real-time key/value store
clients set watches on key ranges and receive updates via persistent (web)socket connection.
¯\\_(ツ)_/¯

#### to compile you need:
all dependencies need to be symlinked/installed/copied to /usr/local/{lib, include} (also timeout)
- c11 & c++17 compiler (clang/gcc)
- [cmake](https://cmake.org/)
- [ninja](https://ninja-build.org/) optional
- [libdill](https://github.com/sustrik/libdill)
- [flatcc](https://github.com/dvidelabs/flatcc)
- [concurrencykit](https://github.com/concurrencykit/ck)
- [llrb](https://github.com/mjolk/llrb-interval.git)
- [rocksdb](https://github.com/facebook/rocksdb)
- [klib](https://github.com/attractivechaos/klib)
- [timeout](https://github.com/wahern/timeout)
   

