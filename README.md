# Session::Token for C++

Secure, efficient, simple random session token generation.

A modern C++20 header-only library for creating session tokens, password reset codes, temporary passwords, random identifiers, and anything else you can think of.

## Synopsis

### Simple 128-bit session token

```cpp
#include "include/SessionToken.h"

auto token = SessionToken::Generator().get();
// 74da9DABOqgoipxqQDdygw
```

### Keep generator around

```cpp
SessionToken::Generator generator;

auto token = generator.get();
// bu4EXqWt5nEeDjTAZcbTKY

auto token2 = generator.get();
// 4Vez56Zc7el5Ggx4PoXCNL
```

### Custom minimum entropy in bits

```cpp
auto token = SessionToken::Generator::with_entropy(256).get();
// WdLiluxxZVkPUHsoqnfcQ1YpARuj9Z7or3COA4HNNAv
```

### Custom alphabet and length

```cpp
auto token = SessionToken::Generator::with_length(100000000, "ACGT").get();
// AGTACTTAGCAATCAGCTGGTTCATGGTTGCCCCCATAG...
```

## Description

When a `SessionToken::Generator` object is created, 32 bytes are read from `getrandom()` (Linux). These bytes are used to seed the ChaCha20 stream cipher which serves as a cryptographically secure pseudo-random number generator.

Once a generator is created, you can repeatedly call the `get()` method on the generator object and it will return a new token each time.

**IMPORTANT**: If your application calls `fork()`, make sure that any generators are re-created in one of the processes after the fork since forking will duplicate the generator state and both parent and child processes will go on to produce identical tokens.

After the generator is created, no system calls are used to generate tokens. This is one way that SessionToken helps with efficiency. However, this is only important for certain use cases (generally not web sessions).

ChaCha20 is a widely-used, cryptographically secure stream cipher. It is the basis of the ChaCha20-Poly1305 AEAD used in TLS and other protocols.

## Building

```bash
make test
```

Requires a C++20 compatible compiler (tested with g++).

## API Reference

### Constructors and Factory Methods

```cpp
// Default: 128-bit entropy, base-62 alphabet, seeded from getrandom()
SessionToken::Generator gen;

// Custom alphabet
SessionToken::Generator gen("0123456789abcdef");

// With explicit seed (32 bytes) for deterministic generation
std::array<uint8_t, 32> seed = { /* ... */ };
auto gen = SessionToken::Generator::with_seed(seed);

// With custom entropy (bits)
auto gen = SessionToken::Generator::with_entropy(256);
auto gen = SessionToken::Generator::with_entropy(64, "0123456789abcdef");

// With custom length (characters)
auto gen = SessionToken::Generator::with_length(50);
auto gen = SessionToken::Generator::with_length(20, "ACGT");

// With seed and custom settings
auto gen = SessionToken::Generator::with_seed_and_entropy(seed, 256);
auto gen = SessionToken::Generator::with_seed_and_length(seed, 50, "ACGT");
```

### Methods

```cpp
std::string get();                      // Generate a token
const std::string& alphabet() const;    // Get the alphabet
size_t token_length() const;            // Get the token length
```

### Constants

```cpp
SessionToken::Generator::default_alphabet  // "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
SessionToken::Generator::default_entropy   // 128
SessionToken::Generator::seed_size         // 32
```

## Generators and getrandom()

Developers must choose whether a single token generator will be kept around and used to generate all tokens, or if a new `SessionToken::Generator` object will be created every time a token is needed. This library accesses `getrandom()` in its constructor for seeding purposes, but not subsequently while generating tokens.

Generally speaking the generator should be kept around and re-used. Probably the most important reason for this is that generating a new token from an existing generator cannot fail due to resource exhaustion. Depending on the platform, creating a new `SessionToken::Generator` object could fail (ie because it may need to `/dev/urandom` but be out of file descriptors, although this is not a concern on linux because it uses `getrandom()`). In these events a C++ exception will be thrown.

Programs that re-use a generator are more likely to be portable to `chroot`ed environments where `/dev/urandom` may not be accessible (if relevant for your platform).

Finally, accessing the kernel's random source frequently is inefficient because it requires making system calls.

On the other hand, re-using a generator may be undesirable because servers are typically started immediately after a system reboot and the kernel's randomness pool might be poorly seeded at that point. Similarly, when starting a virtual machine a previously used entropy pool state may be restored. If an attacker is able to peek at the memory of get a glimpse into the private memory of a running process, it can predict all future tokens. For these reasons, you might choose to defer creating the generator until the first request actually comes in, periodically re-create the generator object, and/or manually handle seeding in some other way.

This library will always throw an exception if it can't securely seed itself.

## Custom Alphabets

Being able to choose exactly which characters appear in your token is sometimes useful. This set of characters is called the *alphabet*. **The default alphabet size is 62 characters: uppercase letters, lowercase letters, and digits** (`a-zA-Z0-9`).

For some purposes, base-62 is a sweet spot. It is more compact than hexadecimal encoding which helps with efficiency because session tokens are usually transferred over the network many times during a session (often uncompressed in HTTP headers).

Also, base-62 tokens don't use "wacky" characters like base-64 encodings do. These characters sometimes cause encoding/escaping problems (ie when embedded in URLs) and are annoying because often you can't select tokens by double-clicking on them.

Although the default is base-62, there are all kinds of reasons for using another alphabet. One example is if your users are reading tokens from a print-out or SMS or whatever, you may choose to omit characters like `o`, `O`, and `0` that can easily be confused.

To set a custom alphabet, pass a string to the constructor or factory methods:

```cpp
SessionToken::Generator::with_entropy(128, "01").get();           // Binary
SessionToken::Generator::with_length(32, "0123456789abcdef").get(); // Hex
SessionToken::Generator::with_length(20, "ACGT").get();           // DNA
```

## Entropy

There are two ways to specify the length of tokens. The most primitive is in terms of characters:

```cpp
auto token = SessionToken::Generator::with_length(5).get();
// -> wpLH4
```

But the primary way is to specify their minimum entropy in terms of bits:

```cpp
auto token = SessionToken::Generator::with_entropy(24).get();
// -> Fo5SX
```

In the above example, the resulting token contains at least 24 bits of entropy. Given the default base-62 alphabet, we can compute the exact entropy of a 5 character token as follows:

```
5 * log2(62) ≈ 29.77 bits
```

So these tokens have about 29.8 bits of entropy. Note that if we removed one character from this token, it would bring it below our desired 24 bits of entropy:

```
4 * log2(62) ≈ 23.82 bits
```

**The default minimum entropy is 128 bits.** Default tokens are 22 characters long and therefore have about 131 bits of entropy:

```
22 * log2(62) ≈ 130.99 bits
```

An interesting observation is that in base-64 representation, 128-bit minimum tokens also require 22 characters and that these tokens contain only 1 more bit of entropy.

Another design criterion is that all tokens should be the same length. The default token length is 22 characters and the tokens are always exactly 22 characters (no more, no less). A fixed token length is nice because it makes writing matching regular expressions easier, simplifies storage (you never have to store length), causes various log files and things to line up neatly on your screen, and ensures that encrypted tokens won't leak token entropy due to length.

In summary, the default token length of exactly 22 characters is a consequence of these decisions: base-62 representation, 128 bit minimum token entropy, and fixed token length.

## Mod Bias

Some token generation libraries that implement custom alphabets will generate a random value, compute its modulus over the size of an alphabet, and then use this modulus to index into the alphabet to determine an output character.

Assume we have a uniform random number source that generates values in the set `[0,1,2,3]` (most PRNGs provide sequences of bits, in other words power-of-2 size sets) and wish to use the alphabet `"abc"`.

If we use the naïve modulus algorithm described above then `0` maps to `a`, `1` maps to `b`, `2` maps to `c`, and `3` *also* maps to `a`. This results in the following biased distribution for each character in the token:

```
P(a) = 2/4 = 1/2
P(b) = 1/4
P(c) = 1/4
```

Of course in an unbiased distribution, each character would have the same chance:

```
P(a) = 1/3
P(b) = 1/3
P(c) = 1/3
```

Bias is undesirable because certain tokens are obvious starting points when token guessing and certain other tokens are very unlikely. Tokens that are unbiased are equally likely and therefore there is no obvious starting point with them.

SessionToken provides unbiased tokens regardless of the size of your alphabet (though see the *Introducing Bias* section for a mis-use warning). It does this in the same way that you might simulate producing unbiased random numbers from 1 to 5 given an unbiased 6-sided die: Re-roll every time a 6 comes up.

In the above example, SessionToken eliminates bias by only using values of `0`, `1`, and `2`.

## Efficiency of Re-Rolling

Throwing away a portion of random data in order to avoid mod bias is slightly inefficient. How many bytes from ChaCha20 do we expect to consume for every character in the token? It depends on the size of the alphabet.

SessionToken masks off each byte using the smallest power of two greater than or equal to the alphabet size minus one so the probability that any particular byte can be used is:

```
P = alphabet_size / next_power_of_two(alphabet_size)
```

For example, with the default base-62 alphabet `P` is `62/64`.

In order to find the average number of bytes consumed for each character, calculate the expected value `E`. There is a probability `P` that the first byte will be used and therefore only one byte will be consumed, and a probability `1 - P` that `1 + E` bytes will be consumed:

```
E = P*1 + (1 - P)*(1 + E)

E = P + 1 + E - P - P*E

0 = 1 - P*E

P*E = 1

E = 1/P
```

The expected number of bytes consumed for each character is `E = 1/P`. For the default base-62 alphabet:

```
E = 1/(62/64) = 64/62 ≈ 1.0323
```

Because of the next power of two masking optimisation, `E` will always be less than `2`. In the worst case scenario of an alphabet with 129 characters, `E` is roughly `1.9845`.

This minor inefficiency isn't an issue because the ChaCha20 implementation is quite fast and this library is very thrifty in how it uses ChaCha20's output.

## Introducing Bias

If your alphabet contains the same character two or more times, this character will be more biased than a character that only occurs once. You should be careful that your alphabets don't repeat in this way if you are trying to create random session tokens.

```cpp
// Don't do this!
SessionToken::Generator::with_length(5000, "0000001").get();
// -> 0000000000010000000110000000000000000000000100...
```

## Alphabet Size Limitation

Due to the byte-oriented design, alphabets can't be larger than 256 characters. If you need to map tokens onto larger character sets, you can post-process the output.

## Seeding

This library is designed to always seed itself from your kernel's secure random number source (ie `getrandom()`). You should never need to seed it yourself.

However if you know what you're doing you can pass in a custom seed as a 32-byte array. For example, here is how to create a "null seeded" generator:

```cpp
std::array<uint8_t, 32> seed{};  // All zeros
auto gen = SessionToken::Generator::with_seed(seed);
```

This is useful for testing to get reproducible output, but obviously don't do this in production because the generated tokens will be the same every time your program is run.

One valid reason for manually seeding is if you have some reason to believe that there isn't enough entropy in your kernel's randomness pool. In this case you should acquire your own seed data from somewhere trustworthy.

## Platform Support

This library currently only supports Linux, as it uses the `getrandom()` system call for seeding.

## See Also

- [Original Perl Session::Token](https://github.com/hoytech/Session-Token)
- [ChaCha20 cipher](https://en.wikipedia.org/wiki/Salsa20#ChaCha_variant)

## License

(C) 2012-2026 Doug Hoyte

This library is licensed under the MIT license.

ChaCha20 implementation from [github.com/983/ChaCha20](https://github.com/983/ChaCha20).
