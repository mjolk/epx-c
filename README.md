### epx-c WIP

Distributed key/value store using Egalitarian paxos
This store will work with range subscriptions.
Clients can set watches on key ranges and receive updates via persistent (web)socket connection.
Egalitarian paxos will be better for embedding in mobile devices since it does not need a master
and should better cope with fast changing quorum configurations. (That's the idea anyway ¯\\_(ツ)_/¯)

#### to compile you need:
all dependencies need to be symlinked/installed/copied to /usr/local/{lib, include}
- c11 compiler (clang/gcc)
- [cmake](https://cmake.org/)
- [ninja](https://ninja-build.org/) optional
- [libdill](https://github.com/sustrik/libdill)
- [flatcc](https://github.com/dvidelabs/flatcc)
- [concurrencykit](https://github.com/concurrencykit/ck)
- [llrb](https://github.com/mjolk/llrb-interval.git)
- [rocksdb](https://github.com/facebook/rocksdb)
   

