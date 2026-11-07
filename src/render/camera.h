#pragma once

#include <algorithm>
#include <array>
#include <cmath>

#include "primitives/vec.h"

/*
    The math of the camera: matrices and the position of the camera
    It knows nothing about OpenGL, so it is checked by ordinary tests
    A matrix is stored by columns, as OpenGL wants it: the element (row, column) is m[column * 4 + row]
*/

// 
struct Mat4
{
    std::array<float, 16> m{};
};

// a * b: first b, then a
inline Mat4 multiply(const Mat4& a, const Mat4& b)
{
    Mat4 result;
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
        {
            float sum = 0;
            for (int k = 0; k < 4; ++k) sum += a.m[k * 4 + row] * b.m[column * 4 + k];
            result.m[column * 4 + row] = sum;
        }
    return result;
}

// The vector with length 1 in the same direction
inline Vector<float, 3> normalized(const Vector<float, 3>& v)
{
    return v * (1.0f / v.length());
}

// View matrix: moves the world so that the camera is at the origin and looks along -z
inline Mat4 look_at(const Vector<float, 3>& eye, const Vector<float, 3>& target, const Vector<float, 3>& up)
{
    const Vector<float, 3> forward = normalized(target - eye);
    const Vector<float, 3> side = normalized(forward.cross(up));
    const Vector<float, 3> real_up = side.cross(forward);

    Mat4 result;
    auto& m = result.m;
    m[0] = side[0];      m[4] = side[1];      m[8] = side[2];       m[12] = -side.dot(eye);
    m[1] = real_up[0];   m[5] = real_up[1];   m[9] = real_up[2];    m[13] = -real_up.dot(eye);
    m[2] = -forward[0];  m[6] = -forward[1];  m[10] = -forward[2];  m[14] = forward.dot(eye);
    m[3] = 0;            m[7] = 0;            m[11] = 0;            m[15] = 1;
    return result;
}

// Perspective projection: what is between near and far goes to z from -1 to +1
inline Mat4 perspective(float fov_y, float aspect, float near_plane, float far_plane)
{
    const float f = 1.0f / std::tan(fov_y / 2);
    Mat4 result;
    auto& m = result.m;
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (far_plane + near_plane) / (near_plane - far_plane);
    m[11] = -1;
    m[14] = 2 * far_plane * near_plane / (near_plane - far_plane);
    return result;
}

// Free camera: a position and two angles
// yaw = -90 degrees and pitch = 0 look along -z
class Camera
{
public:
    Camera(const Vector<float, 3>& position, float yaw, float pitch) : position_(position), yaw_(yaw), pitch_(pitch) {}

    const Vector<float, 3>& position() const { return position_; }
    void set_position(const Vector<float, 3>& position) { position_ = position; }

    // Where the camera looks (length 1)
    Vector<float, 3> front() const
    {
        return Vector<float, 3>(std::cos(pitch_) * std::cos(yaw_), std::sin(pitch_), std::cos(pitch_) * std::sin(yaw_));
    }

    // Where the right hand of the camera is
    Vector<float, 3> right() const { return normalized(front().cross(world_up())); }

    Mat4 view() const { return look_at(position_, position_ + front(), world_up()); }

    // Move by distances along the camera axes: forward, to the right, up
    void move(float forward, float to_right, float up)
    {
        position_ += front() * forward + right() * to_right + world_up() * up;
    }

    // Turn by angles (radians), the pitch can not go over the vertical
    void rotate(float yaw_delta, float pitch_delta)
    {
        constexpr float limit = 1.553343f;   // 89 degrees
        yaw_ += yaw_delta;
        pitch_ = std::clamp(pitch_ + pitch_delta, -limit, limit);
    }

    // Turn to look at the point
    void look_at_point(const Vector<float, 3>& target)
    {
        const Vector<float, 3> direction = normalized(target - position_);
        yaw_ = std::atan2(direction[2], direction[0]);
        pitch_ = std::asin(std::clamp(direction[1], -1.0f, 1.0f));
    }

private:
    static Vector<float, 3> world_up() { return Vector<float, 3>(0.0f, 1.0f, 0.0f); }

    Vector<float, 3> position_;
    float yaw_;
    float pitch_;
};
