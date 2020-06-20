#pragma once

#include <type_traits>
#include "common/common_types.h"
#include "core/core.h"
#include "core/memory.h"

namespace zelda {

template <typename T, typename U, typename = void>
struct IsSafelyCastable : std::false_type {};

template <typename T, typename U>
struct IsSafelyCastable<T, U, std::void_t<decltype(static_cast<U>(std::declval<T>()))>>
    : std::true_type {};

template <typename T>
struct Ptr {
    constexpr Ptr() = default;
    constexpr Ptr(std::nullptr_t ptr) : addr(0) {}
    // This is a dangerous constructor, so make this explicit.
    constexpr explicit Ptr(VAddr addr) : addr(addr) {}

    explicit operator bool() {
        return addr != 0;
    }

    T* operator->() const {
        return get();
    }

    template <typename T2 = T>
    std::enable_if_t<!std::is_void_v<T2>, T2&> operator*() const {
        return *get();
    }

    operator T*() const {
        return get();
    }

    T* get() const {
        if (addr == 0)
            return nullptr;
        return reinterpret_cast<T*>(Core::System::GetInstance().Memory().GetPointer(addr));
    }

    template <typename T2>
    Ptr<T2> StaticCast() const {
        static_assert(IsSafelyCastable<T, T2>());
        return Cast<T2>();
    }

    template <typename T2>
    Ptr<T2> Cast() const {
        return Ptr<T2>{addr};
    }

    bool operator==(const Ptr& rhs) const {
        return addr == rhs.addr;
    }

    bool operator!=(const Ptr& rhs) const {
        return addr != rhs.addr;
    }

    VAddr addr;
};

template <typename T = float>
struct TVec3 {
    T x;
    T y;
    T z;

    constexpr bool operator==(const TVec3& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    constexpr bool operator!=(const TVec3& rhs) const {
        return !(*this == rhs);
    }

    T Distance(const TVec3& other) const {
        const T delta_x = x - other.x;
        const T delta_y = y - other.y;
        const T delta_z = z - other.z;
        return std::sqrt(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);
    }
};
using Vec3 = TVec3<float>;
using Vec3s = TVec3<s16>;

} // namespace zelda
