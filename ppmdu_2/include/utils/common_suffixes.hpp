#include <cstdint>

// Underscore-prefixed versions of the MSVC literal suffixes, which are more portable
constexpr uint8_t  operator"" _ui8 (unsigned long long literal_) {return literal_;}
constexpr uint16_t operator"" _ui16(unsigned long long literal_) {return literal_;}
constexpr uint32_t operator"" _ui32(unsigned long long literal_) {return literal_;}
constexpr uint64_t operator"" _ui64(unsigned long long literal_) {return literal_;}

constexpr int8_t  operator"" _i8 (unsigned long long literal_){return literal_;}
constexpr int16_t operator"" _i16(unsigned long long literal_){return literal_;}
constexpr int32_t operator"" _i32(unsigned long long literal_){return literal_;}
constexpr int64_t operator"" _i64(unsigned long long literal_){return literal_;}
