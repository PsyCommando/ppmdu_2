#ifndef COMMON_SUFFIXES_HPP
#define COMMON_SUFFIXES_HPP
#include <cstdint>
#ifndef _MSC_BUILD
    #if __GNUG__
        //Get rid of the warning for the suffix not having an underscore, since this is a substitution for MSVC.
        //If suffix names matching those ever get used, they'll make the compiler complain about duplicate definitions anyways.
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wliteral-suffix"
    #endif
constexpr uint8_t  operator"" ui8 (unsigned long long literal_) {return literal_;}
constexpr uint16_t operator"" ui16(unsigned long long literal_) {return literal_;}
constexpr uint32_t operator"" ui32(unsigned long long literal_) {return literal_;}
constexpr uint64_t operator"" ui64(unsigned long long literal_) {return literal_;}

constexpr int8_t  operator"" i8 (unsigned long long literal_){return literal_;}
constexpr int16_t operator"" i16(unsigned long long literal_){return literal_;}
constexpr int32_t operator"" i32(unsigned long long literal_){return literal_;}
constexpr int64_t operator"" i64(unsigned long long literal_){return literal_;}
    #if __GNUG__
        #pragma GCC diagnostic pop
    #endif
#endif // !_MSC_BUILD
#endif // ! COMMON_SUFFIXES_HPP