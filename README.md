WIP

Works on Clang 19.1.7

Does not work on GCC 15.1.1 due to `std` module implementation bugs present
in that version (will try a newer GCC - at least one of the bugs is fixed there)

Release mode flags:
```
CC=clang CXX=clang++ cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS='-stdlib=libc++ -march=native -O3 -mllvm -force-vector-width=8'
```

Debug mode flags:
```
CC=clang CXX=clang++ cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_FLAGS='-stdlib=libc++ -fsanitize=address,undefined'
```
