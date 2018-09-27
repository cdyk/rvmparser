#pragma once

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


struct Mat3f
{
  Mat3f() = default;
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
