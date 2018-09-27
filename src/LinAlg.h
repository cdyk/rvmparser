#pragma once
#include <cmath>

struct Vec3f
{
  Vec3f() = default;
  Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
  union {
    struct {
      float x;
      float y;
      float z;
    };
    float data[3];
  };
};

inline Vec3f cross(const Vec3f& a, const Vec3f& b)
{
  return Vec3f(a.y * b.z - a.z * b.y,
               a.z * b.x - a.x * b.z,
               a.x * b.y - a.y * b.x);
}

inline float dot(const Vec3f& a, const Vec3f& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3f operator+(const Vec3f& a, const Vec3f& b) { return Vec3f(a.x + b.x, a.y + b.y, a.z + b.z); }

inline Vec3f operator-(const Vec3f& a, const Vec3f& b) { return Vec3f(a.x - b.x, a.y - b.y, a.z - b.z); }

inline Vec3f operator*(const float a, const Vec3f& b) { return Vec3f(a*b.x, a*b.y, a*b.z); }

inline float lengthSquared(const Vec3f& a) { return dot(a, a); }

inline float length(const Vec3f& a) { return std::sqrt(dot(a, a)); }

inline float distanceSquared(const Vec3f& a, const Vec3f&b) { return lengthSquared(a - b); }

inline float distance(const Vec3f& a, const Vec3f&b) { return length(a - b); }

inline Vec3f normalize(const Vec3f& a) { return (1.f / length(a))*a; }


struct Mat3f
{
  Mat3f() = default;
  Mat3f(const float* ptr) { for (unsigned i = 0; i < 3 * 3; i++) data[i] = ptr[i]; }
  Mat3f(float m00, float m01, float m02,
        float m10, float m11, float m12,
        float m20, float m21, float m22) :
    m00(m00), m10(m10), m20(m20),
    m01(m01), m11(m11), m21(m21),
    m02(m02), m12(m12), m22(m22)
  {}


  union {
    struct {
      float m00;
      float m10;
      float m20;
      float m01;
      float m11;
      float m21;
      float m02;
      float m12;
      float m22;
    };
    Vec3f cols[3];
    float data[3 * 3];
  };
};

Mat3f inverse(const Mat3f& M);

Mat3f mul(const Mat3f& A, const Mat3f& B);

inline Vec3f mul(const Mat3f& A, const Vec3f& x)
{
  Vec3f r;
  for (unsigned k = 0; k < 3; k++) {
    r.data[k] = A.data[k] * x.data[0] + A.data[3 + k] * x.data[1] + A.data[6 + k] * x.data[2];
  }
  return r;
}

struct Mat3x4f
{
  Mat3x4f(const float* ptr) { for (unsigned i = 0; i < 4 * 3; i++) data[i] = ptr[i]; }

  union {
    struct {
      float m00;
      float m10;
      float m20;

      float m01;
      float m11;
      float m21;

      float m02;
      float m12;
      float m22;

      float m03;
      float m13;
      float m23;
    };
    Vec3f cols[4];
    float data[4 * 3];
  };
};

inline Vec3f mul(const Mat3x4f& A, const Vec3f& x)
{
  Vec3f r;
  for (unsigned k = 0; k < 3; k++) {
    r.data[k] = A.data[k] * x.data[0] + A.data[3 + k] * x.data[1] + A.data[6 + k] * x.data[2] + A.data[9 + k];
  }
  return r;
}
