#pragma once

struct Vec2f
{
  Vec2f() = default;
  Vec2f(const Vec2f&) = default;
  Vec2f(float x) : x(x), y(x) {}
  Vec2f(float* ptr) : x(ptr[0]), y(ptr[1]) {}
  Vec2f(float x, float y) : x(x), y(y) {}
  union {
    struct {
      float x;
      float y;
    };
    float data[2];
  };
  float& operator[](size_t i) { return data[i]; }
  const float& operator[](size_t i) const { return data[i]; }
};


struct Vec3f
{
  Vec3f() = default;
  Vec3f(const Vec3f&) = default;
  Vec3f(float x) : x(x), y(x), z(x) {}
  Vec3f(float* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) {}
  Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
  Vec3f(const Vec2f& a, float z) : x(a.x), y(a.y), z(z) {}

  union {
    struct {
      float x;
      float y;
      float z;
    };
    float data[3];
  };
  float& operator[](size_t i) { return data[i]; }
  const float& operator[](size_t i) const { return data[i]; }
};


struct BBox3f
{
  BBox3f() = default;
  BBox3f(const BBox3f&) = default;
  BBox3f(const Vec3f& min, const Vec3f& max) : min(min), max(max) {}
  BBox3f(const BBox3f& bbox, float margin);

  union {
    struct {
      Vec3f min;
      Vec3f max;
    };
    float data[6];
  };
};

struct Mat3f
{
  Mat3f() = default;
  Mat3f(const Mat3f&) = default;
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


struct Mat3x4f
{
  Mat3x4f() = default;
  Mat3x4f(const Mat3x4f&) = default;
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
