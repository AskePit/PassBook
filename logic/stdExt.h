#pragma once

#include <cstdint>
#include <type_traits>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using uint = uint32_t;
using u64 = uint64_t;
using uid = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

#define enum_class(x) class x { public: enum type
#define enum_interface };
#define enum_end ;}

template<class OUT, class IN>
inline auto as(IN *data) {
    return reinterpret_cast<
        typename std::conditional<std::is_const<IN>::value, const OUT, OUT>::type
    >(data);
}

template <class OUT, class IN>
inline auto as(IN &t) -> decltype(as<OUT>(t.data())) {
    return as<OUT>(t.data());
}
