#pragma once

class SessionToken {
    // FIXME: Implement this class as a header-only modern C++ library, using features up to C++20.
    // Follow the pattern from the `Session-Token.pm` and `Session-Token.xs` perl files, but use C++ idioms instead of perl idioms.
    //
    // * Instead of opening `/dev/urandom` use the `getrandom()` linux syscall. It is OK to be linux-only.
    // * Rather than using the isaac PRNG from `rand.h`, use the chacha20 library in `include/chacha20.hpp`.
    //   In should be initialised by a call to `getrandom()` for 32 bytes, and the nonce can be hard-coded at 0.
    // * Each call to chacha20 should get 1024 bytes, and these bytes should be consumed to generate output
    //   characters, in the same way is done with the `get_new_byte` and `get_mask` methods in the XS file.
    // * You should be able to provide a custom 32-byte seed to deterministically re-generate the same sequences.
    // * The constructor should allow you to override the default alphabet and the length/entropy (not both!).
    // * Create a `test.cpp` file and a simple `Makefile` to test the header. It should test both the `getrandom()`
    //   seeded path, and a deterministically seeded path.
}
