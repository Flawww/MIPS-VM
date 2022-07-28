#pragma once
#include "pch.h"

template<class To, size_t NumBits = 0, class From>
static To& bit_cast(const From& in) {
    static_assert(sizeof(To) * 8 >= NumBits && sizeof(From) * 8 >= NumBits, "bit_cast with too many target bits, would cause overflow");

    constexpr size_t num_bits = (NumBits > 0) ? NumBits : std::min(sizeof(From), sizeof(To)) * 8;
    constexpr size_t num_bytes = num_bits / 8;
    constexpr size_t rem_bits = num_bits % 8;
    constexpr size_t mask = (rem_bits != 0) * ~(uint64_t)0 >> (64 - rem_bits);

    To var{};
    // copy as many full bytes as possible
    memcpy(&var, &in, num_bytes);
    // copy remaining bits
    *((uint8_t*)(&var) + num_bytes) |= *((uint8_t*)(&in) + num_bytes) & mask;

    return var;
}

static bool string_terminates(const char* str, size_t buf_size) {
    for (int i = 0; i < buf_size; i++) {
        if (str[i] == 0) {
            return true;
        }
    }

    return false;
}