### epx-c WIP

Distributed key/value store using Egalitarian paxos using rocksdb as backing store (planned)
This store will work with range subscriptions.
Clients can set watches on key ranges and receive updates via persistent (web)socket connection.
Egalitarian paxos will be better for embedding in mobile devices since it does not need a master
and should better cope with fast changing quorum configurations. (That's the idea anyway)

using [libdill](http://libdill.org) for extra concurrency
and flatbuffers [flatcc](https://github.com/dvidelabs/flatcc) for fast transport  
flatbuffers need no decoding so hopefully offset a little the extra cycles burned by the
consensus algo.

#### to compile you need:
- c11 compiler (clang/gcc)
- [cmake](https://cmake.org/)
- [ninja](https://ninja-build.org/) optional for faster builds. makes a difference for me allthough this is not a large project.
- [libdill](http://libdill.org)
- [flatcc](https://github.com/dvidelabs/flatcc)
- [concurrencykit](http://concurrencykit.org)
- [llrb](https://github.com/mjolk/llrb-interval.git) a left leaning red/black tree with augmentation for 
  intervals, this will be a git submodule but for development a symlink is easier for now -.-. 
  Check out in the same parent dir as epx.
    eg:  
    parent  
        |_ llrb  
        |_ epx

