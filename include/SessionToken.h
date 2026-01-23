#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/random.h>

#include "chacha20.hpp"

namespace SessionToken {

class Generator {
public:
    static constexpr std::string_view default_alphabet = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static constexpr size_t default_entropy = 128;
    static constexpr size_t seed_size = 32;
    static constexpr size_t buffer_size = 1024;

private:
    std::string alphabet_;
    size_t token_length_;
    int mask_;

    Chacha20 rng_;
    std::array<uint8_t, buffer_size> buffer_;
    size_t buffer_pos_;

    static int compute_mask(size_t alphabet_size) {
        int v = static_cast<int>(alphabet_size) - 1;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        return v;
    }

    static size_t compute_length_from_entropy(size_t entropy, size_t alphabet_size) {
        double alphabet_entropy = std::log2(static_cast<double>(alphabet_size));
        return static_cast<size_t>(std::ceil(static_cast<double>(entropy) / alphabet_entropy));
    }

    static std::array<uint8_t, seed_size> generate_seed() {
        std::array<uint8_t, seed_size> seed;
        ssize_t result = getrandom(seed.data(), seed.size(), 0);
        if (result != static_cast<ssize_t>(seed.size())) {
            throw std::runtime_error("Failed to get random bytes from getrandom()");
        }
        return seed;
    }

    static Chacha20 create_rng(const std::array<uint8_t, seed_size>& seed) {
        constexpr uint8_t nonce[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        return Chacha20(seed.data(), nonce);
    }

    void refill_buffer() {
        rng_.rng(buffer_.data(), buffer_.size());
        buffer_pos_ = 0;
    }

    uint8_t get_byte() {
        if (buffer_pos_ >= buffer_.size()) {
            refill_buffer();
        }
        return buffer_[buffer_pos_++];
    }

public:
    // Constructor with custom seed (for deterministic generation)
    explicit Generator(const std::array<uint8_t, seed_size>& seed,
                       std::string_view alphabet = default_alphabet,
                       std::optional<size_t> length = std::nullopt,
                       std::optional<size_t> entropy = std::nullopt)
        : alphabet_(alphabet),
          token_length_(0),
          mask_(0),
          rng_(create_rng(seed)),
          buffer_(),
          buffer_pos_(buffer_size) {

        if (alphabet_.size() < 2 || alphabet_.size() > 256) {
            throw std::invalid_argument("alphabet must be between 2 and 256 characters");
        }

        if (length.has_value() && entropy.has_value()) {
            throw std::invalid_argument("cannot specify both length and entropy");
        }

        if (length.has_value()) {
            if (*length == 0) {
                throw std::invalid_argument("length must be greater than 0");
            }
            token_length_ = *length;
        } else {
            size_t ent = entropy.value_or(default_entropy);
            if (ent == 0) {
                throw std::invalid_argument("entropy must be greater than 0");
            }
            token_length_ = compute_length_from_entropy(ent, alphabet_.size());
        }

        mask_ = compute_mask(alphabet_.size());
    }

    // Constructor with automatic seeding from getrandom()
    explicit Generator(std::string_view alphabet = default_alphabet,
                       std::optional<size_t> length = std::nullopt,
                       std::optional<size_t> entropy = std::nullopt)
        : Generator(generate_seed(), alphabet, length, entropy) {}

    // Builder-style static factory methods for cleaner API
    static Generator with_entropy(size_t entropy, std::string_view alphabet = default_alphabet) {
        return Generator(alphabet, std::nullopt, entropy);
    }

    static Generator with_length(size_t length, std::string_view alphabet = default_alphabet) {
        return Generator(alphabet, length, std::nullopt);
    }

    static Generator with_seed(const std::array<uint8_t, seed_size>& seed,
                               std::string_view alphabet = default_alphabet) {
        return Generator(seed, alphabet, std::nullopt, std::nullopt);
    }

    static Generator with_seed_and_entropy(const std::array<uint8_t, seed_size>& seed,
                                           size_t entropy,
                                           std::string_view alphabet = default_alphabet) {
        return Generator(seed, alphabet, std::nullopt, entropy);
    }

    static Generator with_seed_and_length(const std::array<uint8_t, seed_size>& seed,
                                          size_t length,
                                          std::string_view alphabet = default_alphabet) {
        return Generator(seed, alphabet, length, std::nullopt);
    }

    // Generate a token
    std::string get() {
        std::string token;
        token.reserve(token_length_);

        for (size_t i = 0; i < token_length_; ++i) {
            size_t idx;
            // Re-roll to avoid mod bias
            do {
                idx = get_byte() & mask_;
            } while (idx >= alphabet_.size());

            token.push_back(alphabet_[idx]);
        }

        return token;
    }

    // Accessors
    [[nodiscard]] const std::string& alphabet() const { return alphabet_; }
    [[nodiscard]] size_t token_length() const { return token_length_; }
};

} // namespace SessionToken
