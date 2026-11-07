#pragma once

#include <vector>
#include <array>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <type_traits>

// Numeric type of coordinates: any arithmetic type except bool
template <typename T>
concept Scalar = std::is_arithmetic_v<T> && !std::same_as<T, bool>;
 
// Vector in N-dimensional space
template <Scalar T = double, std::size_t N = 3> 
requires(N >= 2)
class Vector {
public:
    Vector() : data_{} {}

    // Constructor: number of arguments must match N
    template <typename... Args>
    requires(sizeof...(Args) == N && (std::convertible_to<Args, T> && ...))
    Vector(Args... args) : data_{static_cast<T>(args)...} {}

    // Converts a vector with another coordinate type, for example Vector<double, 3>(v_int)
    template <Scalar U>
    explicit Vector(const Vector<U, N>& vec) : data_{} 
    {
        for (std::size_t i = 0; i < N; i++) data_[i] = static_cast<T>(vec[i]);
    }

    T& operator[](std::size_t i) 
    {
        assert(i < N);
        return data_[i];
    }
    const T& operator[](std::size_t i) const 
    {
        assert(i < N);
        return data_[i];
    }

    Vector& operator+=(const Vector& o) 
    {
        for (std::size_t i = 0; i < N; ++i) data_[i] += o.data_[i];
        return *this;
    }
    Vector& operator-=(const Vector& o) 
    {
        for (std::size_t i = 0; i < N; ++i) data_[i] -= o.data_[i];
        return *this;
    }
    Vector& operator*=(T k) 
    {
        for (auto& c : data_) c *= k;
        return *this;
    }

    // Binary operators
    friend Vector operator+(Vector a, const Vector& b) { return a += b; }
    friend Vector operator-(Vector a, const Vector& b) { return a -= b; }
    friend Vector operator*(Vector a, T k) { return a *= k; }
    friend Vector operator*(T k, Vector a) { return a *= k; }

    // Length of the vector
    // For integer T it is double (the length of an integer vector is not an integer),
    // otherwise it is the same type as T
    auto length() const 
    { 
        // for integers types return double
        using R = std::conditional_t<std::floating_point<T>, T, double>;
        return std::sqrt(static_cast<R>(dot(*this)));
    }

    // Scalar product
    T dot(const Vector& o) const 
    {
        T s = 0;
        for (std::size_t i = 0; i < N; ++i) s += data_[i] * o.data_[i];
        return s;
    }

    // Vector product (only in 3D)
    Vector cross(const Vector& vec) const requires(N == 3)
    {
        return Vector(data_[1] * vec.data_[2] - data_[2] * vec.data_[1],
                      data_[2] * vec.data_[0] - data_[0] * vec.data_[2],
                      data_[0] * vec.data_[1] - data_[1] * vec.data_[0]);
    }

    // The determinant of a 2x2 matrix with rows *this and vec (only in 2D: analog cross in 3D)
    T det(const Vector& vec) const requires(N == 2)
    {
        return data_[0] * vec.data_[1] - data_[1] * vec.data_[0];
    }

private:
    std::array<T, N> data_;
};