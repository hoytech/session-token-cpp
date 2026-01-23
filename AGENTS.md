# Agent Context for SessionToken C++ Library

This document provides context for LLMs working on this codebase.

## Project Overview

This is a C++20 header-only library for generating cryptographically secure random tokens. It is a port of the Perl [Session::Token](https://github.com/hoytech/Session-Token) library, adapted to use modern C++ idioms.

## Architecture

### Key Files

| File | Purpose |
|------|---------|
| `include/SessionToken.h` | Main header-only library implementation |
| `include/chacha20.hpp` | ChaCha20 stream cipher (third-party, slightly modified) |
| `test.cpp` | Test suite |
| `Makefile` | Build system |

### Design Decisions

1. **ChaCha20 instead of ISAAC**: The original Perl library uses the ISAAC PRNG. This C++ port uses ChaCha20, a more modern and widely-trusted stream cipher.

2. **32-byte seed instead of 1024-byte**: ISAAC required a 1024-byte seed. ChaCha20 uses a 32-byte key, so our seed size is 32 bytes.

3. **`getrandom()` instead of `/dev/urandom`**: The original opened `/dev/urandom` for seeding. This port uses the Linux `getrandom()` syscall which has fewer failure modes and doesn't require file descriptors.

4. **Zero nonce**: The ChaCha20 nonce is hard-coded to zero. This is safe because we never reuse keys (each generator instance gets a fresh random seed).

5. **1024-byte buffer**: We fetch 1024 bytes at a time from ChaCha20 for efficiency, matching the Perl implementation's buffer strategy.

6. **Mod bias elimination**: Uses mask-and-reroll technique from the original. See `compute_mask()` and the loop in `get()`.

## Key Algorithms

### Token Generation (`get()` method)

```
for each character in token:
    do:
        byte = get_byte() & mask
    while byte >= alphabet_size
    
    output[i] = alphabet[byte]
```

The mask is the smallest `(2^n - 1)` that is `>= alphabet_size - 1`. This ensures we only discard at most half the random bytes (on average much less).

### Mask Computation (`compute_mask()`)

```cpp
int v = alphabet_size - 1;
v |= v >> 1;
v |= v >> 2;
v |= v >> 4;
return v;
```

This computes the next power of 2 minus 1 (e.g., 62 → 63, 100 → 127).

### Length from Entropy (`compute_length_from_entropy()`)

```
length = ceil(entropy_bits / log2(alphabet_size))
```

For default 128-bit entropy with 62-character alphabet: `ceil(128 / 5.954) = 22`

## Testing

Run tests with:

```bash
make test
```

### Test Coverage

- Deterministic seeding (reproducibility)
- `getrandom()` seeding (uniqueness)
- Default token length (22 chars for 128-bit entropy)
- Custom entropy and length settings
- Custom alphabets
- Input validation (alphabet size, mutual exclusion of length/entropy)
- Character distribution uniformity
- Uniqueness across many tokens

### Adding New Tests

Add test functions to `test.cpp` following the existing pattern:

```cpp
void test_feature_name() {
    std::cout << "Testing feature..." << std::endl;
    // ... test logic with assert() ...
    std::cout << "  PASSED: Description" << std::endl;
}
```

Then call from `main()`.

## Common Tasks

### Adding a New Constructor Option

1. Add parameter to the main constructor in `SessionToken.h`
2. Add validation logic
3. Consider adding a factory method for cleaner API
4. Add tests in `test.cpp`
5. Document in `README.md`

### Changing the PRNG

The PRNG is abstracted through the `Chacha20` class from `chacha20.hpp`. To swap PRNGs:

1. Create/include a new PRNG header with compatible interface
2. Change `rng_` member type in `Generator`
3. Update `create_rng()` factory method
4. Update `seed_size` constant if needed
5. Update `refill_buffer()` if the API differs

### Platform Support

Currently Linux-only due to `getrandom()`. To add other platforms:

1. Add conditional compilation in `generate_seed()`
2. Use appropriate platform APIs:
   - macOS: `getentropy()` or `SecRandomCopyBytes()`
   - Windows: `BCryptGenRandom()`
   - BSD: `getentropy()` or `arc4random_buf()`

## Code Style

- Modern C++20 features encouraged (concepts, ranges, etc. where appropriate)
- Header-only design (everything in `.h`/`.hpp` files)
- `snake_case` for methods and variables
- `PascalCase` for class names
- Private members suffixed with `_`
- Use `[[nodiscard]]` for getters
- Prefer `std::array` over C arrays
- Use `std::string_view` for string parameters that aren't stored

## Security Considerations

1. **Never reuse seeds**: Each generator must have a unique seed
2. **Don't fork with active generators**: Document that forking duplicates state
3. **Seed quality**: Always seed from `getrandom()` in production
4. **No seed extraction**: Currently no API to extract seeds (intentional)
5. **Memory safety**: Consider `explicit_bzero()` for seed data in destructor (not currently implemented)

## Reference Materials

- Original Perl implementation: [github.com/hoytech/Session-Token](https://github.com/hoytech/Session-Token)
- ChaCha20 spec: [RFC 8439](https://datatracker.ietf.org/doc/html/rfc8439)

## Known Limitations

1. Linux-only (uses `getrandom()`)
2. Alphabet size limited to 256 characters
3. No thread safety guarantees (each thread should have its own generator)
4. No seed extraction API
5. No fork detection/protection
