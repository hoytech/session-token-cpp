#include <iostream>
#include <cassert>
#include <set>
#include <algorithm>

#include "include/SessionToken.h"

void test_deterministic_seeding() {
    std::cout << "Testing deterministic seeding..." << std::endl;

    // Create a known seed (all zeros for simplicity)
    std::array<uint8_t, SessionToken::Generator::seed_size> seed{};
    std::fill(seed.begin(), seed.end(), 0);

    // Create two generators with the same seed
    auto gen1 = SessionToken::Generator::with_seed(seed);
    auto gen2 = SessionToken::Generator::with_seed(seed);

    // They should produce identical tokens
    for (int i = 0; i < 10; ++i) {
        std::string token1 = gen1.get();
        std::string token2 = gen2.get();
        assert(token1 == token2);
        std::cout << "  Token " << i << ": " << token1 << std::endl;
    }

    std::cout << "  PASSED: Deterministic seeding produces identical sequences" << std::endl;
}

void test_getrandom_seeding() {
    std::cout << "Testing getrandom() seeding..." << std::endl;

    // Create two generators without explicit seeds (uses getrandom)
    SessionToken::Generator gen1;
    SessionToken::Generator gen2;

    // They should produce different tokens (with extremely high probability)
    std::set<std::string> tokens;
    for (int i = 0; i < 100; ++i) {
        tokens.insert(gen1.get());
        tokens.insert(gen2.get());
    }

    // All 200 tokens should be unique
    assert(tokens.size() == 200);
    std::cout << "  PASSED: getrandom() seeding produces unique tokens" << std::endl;
}

void test_default_token_length() {
    std::cout << "Testing default token length (128-bit entropy)..." << std::endl;

    SessionToken::Generator gen;
    std::string token = gen.get();

    // Default is 128-bit entropy with base-62 alphabet
    // ceil(128 / log2(62)) = ceil(128 / 5.954) = ceil(21.49) = 22
    assert(token.length() == 22);
    assert(gen.token_length() == 22);
    std::cout << "  Token: " << token << " (length: " << token.length() << ")" << std::endl;
    std::cout << "  PASSED: Default token length is 22 characters" << std::endl;
}

void test_custom_entropy() {
    std::cout << "Testing custom entropy..." << std::endl;

    auto gen = SessionToken::Generator::with_entropy(256);
    std::string token = gen.get();

    // ceil(256 / log2(62)) = ceil(256 / 5.9542) = ceil(42.9949) = 43
    assert(token.length() == 43);
    std::cout << "  Token: " << token << " (length: " << token.length() << ")" << std::endl;
    std::cout << "  PASSED: Custom entropy produces correct length" << std::endl;
}

void test_custom_length() {
    std::cout << "Testing custom length..." << std::endl;

    auto gen = SessionToken::Generator::with_length(50);
    std::string token = gen.get();

    assert(token.length() == 50);
    std::cout << "  Token: " << token << " (length: " << token.length() << ")" << std::endl;
    std::cout << "  PASSED: Custom length works correctly" << std::endl;
}

void test_custom_alphabet() {
    std::cout << "Testing custom alphabet..." << std::endl;

    // Binary alphabet
    auto gen_binary = SessionToken::Generator::with_entropy(32, "01");
    std::string token = gen_binary.get();

    // Verify all characters are from alphabet
    for (char c : token) {
        assert(c == '0' || c == '1');
    }
    std::cout << "  Binary token: " << token << std::endl;

    // Hex alphabet
    auto gen_hex = SessionToken::Generator::with_entropy(64, "0123456789abcdef");
    token = gen_hex.get();
    for (char c : token) {
        assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
    std::cout << "  Hex token: " << token << std::endl;

    // DNA alphabet
    auto gen_dna = SessionToken::Generator::with_length(20, "ACGT");
    token = gen_dna.get();
    for (char c : token) {
        assert(c == 'A' || c == 'C' || c == 'G' || c == 'T');
    }
    std::cout << "  DNA token: " << token << std::endl;

    std::cout << "  PASSED: Custom alphabets work correctly" << std::endl;
}

void test_alphabet_validation() {
    std::cout << "Testing alphabet validation..." << std::endl;

    // Too small alphabet
    bool caught = false;
    try {
        SessionToken::Generator gen("a");  // Only 1 character
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
    std::cout << "  PASSED: Rejects alphabet with < 2 characters" << std::endl;

    // Empty alphabet
    caught = false;
    try {
        SessionToken::Generator gen("");
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
    std::cout << "  PASSED: Rejects empty alphabet" << std::endl;

    // Alphabet too big
    caught = false;
    try {
        SessionToken::Generator gen(std::string(257, 'A'));
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
    std::cout << "  PASSED: Rejects alphabet too large" << std::endl;
}

void test_length_entropy_mutual_exclusion() {
    std::cout << "Testing length/entropy mutual exclusion..." << std::endl;

    std::array<uint8_t, SessionToken::Generator::seed_size> seed{};

    bool caught = false;
    try {
        // Try to specify both length and entropy
        SessionToken::Generator gen(seed, SessionToken::Generator::default_alphabet, 10, 128);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
    std::cout << "  PASSED: Cannot specify both length and entropy" << std::endl;
}

void test_deterministic_with_custom_settings() {
    std::cout << "Testing deterministic seeding with custom settings..." << std::endl;

    std::array<uint8_t, SessionToken::Generator::seed_size> seed{};
    for (size_t i = 0; i < seed.size(); ++i) {
        seed[i] = static_cast<uint8_t>(i);
    }

    auto gen1 = SessionToken::Generator::with_seed_and_length(seed, 10, "ACGT");
    auto gen2 = SessionToken::Generator::with_seed_and_length(seed, 10, "ACGT");

    for (int i = 0; i < 5; ++i) {
        std::string token1 = gen1.get();
        std::string token2 = gen2.get();
        assert(token1 == token2);
        assert(token1.length() == 10);
        std::cout << "  Token " << i << ": " << token1 << std::endl;
    }

    std::cout << "  PASSED: Deterministic seeding with custom settings" << std::endl;
}

void test_token_uniformity() {
    std::cout << "Testing token character distribution (basic uniformity check)..." << std::endl;

    // Use a small alphabet to make distribution easier to check
    std::string alphabet = "ABCDE";
    auto gen = SessionToken::Generator::with_length(10000, alphabet);

    std::string token = gen.get();
    std::array<int, 5> counts = {0, 0, 0, 0, 0};

    for (char c : token) {
        counts[c - 'A']++;
    }

    std::cout << "  Distribution over 10000 characters:" << std::endl;
    for (int i = 0; i < 5; ++i) {
        double pct = (counts[i] / 10000.0) * 100.0;
        std::cout << "    " << static_cast<char>('A' + i) << ": " << counts[i]
                  << " (" << pct << "%)" << std::endl;
        // Each should be roughly 20% (2000 +/- some variance)
        // Allow 18-22% range for reasonable statistical variance
        assert(counts[i] > 1800 && counts[i] < 2200);
    }

    std::cout << "  PASSED: Character distribution is reasonably uniform" << std::endl;
}

void test_multiple_tokens() {
    std::cout << "Testing multiple token generation..." << std::endl;

    SessionToken::Generator gen;
    std::set<std::string> tokens;

    for (int i = 0; i < 1000; ++i) {
        tokens.insert(gen.get());
    }

    // All 1000 tokens should be unique
    assert(tokens.size() == 1000);
    std::cout << "  PASSED: 1000 unique tokens generated" << std::endl;
}

int main() {
    std::cout << "=== SessionToken Tests ===" << std::endl << std::endl;

    test_deterministic_seeding();
    std::cout << std::endl;

    test_getrandom_seeding();
    std::cout << std::endl;

    test_default_token_length();
    std::cout << std::endl;

    test_custom_entropy();
    std::cout << std::endl;

    test_custom_length();
    std::cout << std::endl;

    test_custom_alphabet();
    std::cout << std::endl;

    test_alphabet_validation();
    std::cout << std::endl;

    test_length_entropy_mutual_exclusion();
    std::cout << std::endl;

    test_deterministic_with_custom_settings();
    std::cout << std::endl;

    test_token_uniformity();
    std::cout << std::endl;

    test_multiple_tokens();
    std::cout << std::endl;

    std::cout << "=== All tests PASSED ===" << std::endl;
    return 0;
}
