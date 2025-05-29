#pragma once

#include "Core.hpp"

#include <math.h>
#include <float.h>
#include <limits.h>

#define Pi 3.14159265358979323846

template<typename T>
T Min(T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
T Max(T a, T b)
{
    return a > b ? a : b;
}

#define Clamp(x, a, b) (((x) < (a)) ? (a) : ((x) > (b)) ? (b) : (x))
#define Abs(x) (((x) < 0) ? -(x) : (x))
#define ToRads(x) ((x) * Pi / 180)
#define ToDegs(x) ((x) * 180 / Pi)
#define Lerp(a, b, t) ((a) * (1 - (t)) + (b) * (t))
#define InverseLerp(a, b, v) (((v) - (a)) / ((b) - (a)))
#define AlignForward(x, align) ((((x) + (align) - 1) / (align)) * (align))

struct RNG
{
    u32 state;
};

void RandomSeed(RNG *rng, u32 seed);
u32 RandomGetNext(RNG *rng);
float RandomGetZeroToOnef(RNG *rng);
float RandomGetRangef(RNG *rng, float min, float max);

struct Recti
{
    s32 x = 0;
    s32 y = 0;
    s32 w = 0;
    s32 h = 0;
};

struct Vec2f
{
    float x = 0;
    float y = 0;

    // explicit Vec2f(float x = 0, float y = 0)
    // {
    //     this->x = x;
    //     this->y = y;
    // }

    Vec2f &operator +=(const Vec2f &b);
    Vec2f &operator -=(const Vec2f &b);
    Vec2f &operator *=(float b);
    Vec2f &operator /=(float b);
};

struct Vec3f
{
    float x = 0;
    float y = 0;
    float z = 0;

    // explicit Vec3f(float x = 0, float y = 0, float z = 0)
    // {
    //     this->x = x;
    //     this->y = y;
    //     this->z = z;
    // }

    Vec3f &operator +=(const Vec3f &b);
    Vec3f &operator -=(const Vec3f &b);
    Vec3f &operator *=(float b);
    Vec3f &operator /=(float b);
};

struct Vec3u
{
    u32 x = 0;
    u32 y = 0;
    u32 z = 0;
};

struct Vec4f
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 0;

    // explicit Vec4f(float x = 0, float y = 0, float z = 0, float w = 0)
    // {
    //     this->x = x;
    //     this->y = y;
    //     this->z = z;
    //     this->w = w;
    // }

    Vec4f &operator +=(const Vec4f &b);
    Vec4f &operator -=(const Vec4f &b);
    Vec4f &operator *=(float b);
    Vec4f &operator /=(float b);
};

struct Quatf
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 1;

    // explicit Quatf(float x = 0, float y = 0, float z = 0, float w = 1)
    // {
    //     this->x = x;
    //     this->y = y;
    //     this->z = z;
    //     this->w = w;
    // }

    Quatf &operator +=(const Quatf &b);
    Quatf &operator -=(const Quatf &b);
    Quatf &operator *=(float b);
    Quatf &operator /=(float b);
};

struct Mat3f
{
    float r0c0 = 1;
    float r0c1 = 0;
    float r0c2 = 0;
    float r1c0 = 0;
    float r1c1 = 1;
    float r1c2 = 0;
    float r2c0 = 0;
    float r2c1 = 0;
    float r2c2 = 1;

    // explicit Mat3f(
    //     float r0c0 = 1, float r0c1 = 0, float r0c2 = 0,
    //     float r1c0 = 0, float r1c1 = 1, float r1c2 = 0,
    //     float r2c0 = 0, float r2c1 = 0, float r2c2 = 1
    // ) {
    //     this->r0c0 = r0c0;
    //     this->r0c1 = r0c1;
    //     this->r0c2 = r0c2;
    //     this->r1c0 = r1c0;
    //     this->r1c1 = r1c1;
    //     this->r1c2 = r1c2;
    //     this->r2c0 = r2c0;
    //     this->r2c1 = r2c1;
    //     this->r2c2 = r2c2;
    // }
};

struct Mat4f
{
    float r0c0 = 1;
    float r0c1 = 0;
    float r0c2 = 0;
    float r0c3 = 0;
    float r1c0 = 0;
    float r1c1 = 1;
    float r1c2 = 0;
    float r1c3 = 0;
    float r2c0 = 0;
    float r2c1 = 0;
    float r2c2 = 1;
    float r2c3 = 0;
    float r3c0 = 0;
    float r3c1 = 0;
    float r3c2 = 0;
    float r3c3 = 1;

    // explicit Mat4f(
    //     float r0c0 = 1, float r0c1 = 0, float r0c2 = 0, float r0c3 = 0,
    //     float r1c0 = 0, float r1c1 = 1, float r1c2 = 0, float r1c3 = 1,
    //     float r2c0 = 0, float r2c1 = 0, float r2c2 = 1, float r2c3 = 0,
    //     float r3c0 = 0, float r3c1 = 0, float r3c2 = 0, float r3c3 = 1
    // ) {
    //     this->r0c0 = r0c0;
    //     this->r0c1 = r0c1;
    //     this->r0c2 = r0c2;
    //     this->r0c3 = r0c3;
    //     this->r1c0 = r1c0;
    //     this->r1c1 = r1c1;
    //     this->r1c2 = r1c2;
    //     this->r1c3 = r1c3;
    //     this->r2c0 = r2c0;
    //     this->r2c1 = r2c1;
    //     this->r2c2 = r2c2;
    //     this->r2c3 = r2c3;
    //     this->r3c0 = r3c0;
    //     this->r3c1 = r3c1;
    //     this->r3c2 = r3c2;
    //     this->r3c3 = r3c3;
    // }
};

Vec2f Add(const Vec2f &a, const Vec2f &b);
Vec3f Add(const Vec3f &a, const Vec3f &b);
Vec4f Add(const Vec4f &a, const Vec4f &b);
Quatf Add(const Quatf &a, const Quatf &b);

Vec2f Sub(const Vec2f &a, const Vec2f &b);
Vec3f Sub(const Vec3f &a, const Vec3f &b);
Vec4f Sub(const Vec4f &a, const Vec4f &b);
Quatf Sub(const Quatf &a, const Quatf &b);

Vec2f Neg(const Vec2f &a);
Vec3f Neg(const Vec3f &a);
Vec4f Neg(const Vec4f &a);
Quatf Neg(const Quatf &a);

Vec2f Mul(const Vec2f &a, float b);
Vec3f Mul(const Vec3f &a, float b);
Vec4f Mul(const Vec4f &a, float b);
Quatf Mul(const Quatf &a, float b);

Vec2f Div(const Vec2f &a, float b);
Vec3f Div(const Vec3f &a, float b);
Vec4f Div(const Vec4f &a, float b);
Quatf Div(const Quatf &a, float b);

bool Equals(const Vec2f &a, const Vec2f &b);
bool Equals(const Vec3f &a, const Vec3f &b);
bool Equals(const Vec4f &a, const Vec4f &b);
bool Equals(const Quatf &a, const Quatf &b);

bool ApproxEquals(float a, float b, float epsilon);
bool ApproxEquals(const Vec2f &a, const Vec2f &b, float epsilon);
bool ApproxEquals(const Vec3f &a, const Vec3f &b, float epsilon);
bool ApproxEquals(const Vec4f &a, const Vec4f &b, float epsilon);
bool ApproxEquals(const Quatf &a, const Quatf &b, float epsilon);

bool ApproxZero(float a, float epsilon);
bool ApproxZero(const Vec2f &a, float epsilon);
bool ApproxZero(const Vec3f &a, float epsilon);
bool ApproxZero(const Vec4f &a, float epsilon);
bool ApproxZero(const Quatf &a, float epsilon);

inline Vec2f operator +(const Vec2f &a, const Vec2f &b) { return Add(a, b); }
inline Vec3f operator +(const Vec3f &a, const Vec3f &b) { return Add(a, b); }
inline Vec4f operator +(const Vec4f &a, const Vec4f &b) { return Add(a, b); }
inline Quatf operator +(const Quatf &a, const Quatf &b) { return Add(a, b); }

inline Vec2f operator -(const Vec2f &a, const Vec2f &b) { return Sub(a, b); }
inline Vec3f operator -(const Vec3f &a, const Vec3f &b) { return Sub(a, b); }
inline Vec4f operator -(const Vec4f &a, const Vec4f &b) { return Sub(a, b); }
inline Quatf operator -(const Quatf &a, const Quatf &b) { return Sub(a, b); }

inline Vec2f operator -(const Vec2f &a) { return Neg(a); }
inline Vec3f operator -(const Vec3f &a) { return Neg(a); }
inline Vec4f operator -(const Vec4f &a) { return Neg(a); }
inline Quatf operator -(const Quatf &a) { return Neg(a); }

inline Vec2f operator *(const Vec2f &a, float b) { return Mul(a, b); }
inline Vec3f operator *(const Vec3f &a, float b) { return Mul(a, b); }
inline Vec4f operator *(const Vec4f &a, float b) { return Mul(a, b); }
inline Quatf operator *(const Quatf &a, float b) { return Mul(a, b); }

inline Vec2f operator /(const Vec2f &a, float b) { return Div(a, b); }
inline Vec3f operator /(const Vec3f &a, float b) { return Div(a, b); }
inline Vec4f operator /(const Vec4f &a, float b) { return Div(a, b); }
inline Quatf operator /(const Quatf &a, float b) { return Div(a, b); }

inline bool operator ==(const Vec2f &a, const Vec2f &b) { return Equals(a, b); }
inline bool operator ==(const Vec3f &a, const Vec3f &b) { return Equals(a, b); }
inline bool operator ==(const Vec4f &a, const Vec4f &b) { return Equals(a, b); }
inline bool operator ==(const Quatf &a, const Quatf &b) { return Equals(a, b); }

float Dot(const Vec2f &a, const Vec2f &b);
float Dot(const Vec3f &a, const Vec3f &b);
float Dot(const Vec4f &a, const Vec4f &b);
float Dot(const Quatf &a, const Quatf &b);

float Length(const Vec2f &v);
float Length(const Vec3f &v);
float Length(const Vec4f &v);
float Length(const Quatf &q);

Vec2f Normalized(const Vec2f &v, const Vec2f &fallback = Vec2f{});
Vec3f Normalized(const Vec3f &v, const Vec3f &fallback = Vec3f{});
Vec4f Normalized(const Vec4f &v, const Vec4f &fallback = Vec4f{});

Vec3f Cross(const Vec3f &a, const Vec3f &b);
Vec3f Reject(const Vec3f &a, const Vec3f &b);

Mat4f Transposed(const Mat4f &m);
Mat4f Inverted(const Mat4f &m);

Vec3f RightVector(const Mat4f &m);
Vec3f UpVector(const Mat4f &m);
Vec3f ForwardVector(const Mat4f &m);

Mat4f Mat4fTranslate(const Vec3f &translation);
Mat4f Mat4fRotate(const Vec3f &axis, float angle);
Mat4f Mat4fLookAt(const Vec3f &position, const Vec3f &target, const Vec3f &up);
Mat4f Mat4fInfinitePerspectiveProjection(float fovy, float aspect, float znear);
Mat4f Mat4fPerspectiveProjection(float fovy, float aspect, float znear, float zfar);

Mat4f Mul(const Mat4f &a, const Mat4f &b);

inline Mat4f operator *(const Mat4f &a, const Mat4f &b) { return Mul(a, b); }

float PerlinNoise(float x, float y);
float PerlinNoise(float x, float y, float z);

#define Perlin_Fractal_Max_Octaves 10
#define Perlin_Fractal_Min_Amplitude 0.00001

struct NoiseParams
{
    float scale = 1.0;
    int octaves = 1;
    float persistance = 0.5; // By how much is amplitude multiplied for each octave (except the first)
    float lacunarity = 2.0; // By how much is frequency multiplied for each octave (exept the first)
    float max_amplitude = 1.0; // Calculate using PerlinFractalMax
    float final_amplitude = 1.0;
};

float PerlinFractalMax(int octaves, float persistance);
void PerlinGenerateOffsets(RNG *rng, Slice<Vec2f> *offsets);
void PerlinGenerateOffsets(RNG *rng, Slice<Vec3f> *offsets);
float PerlinFractalNoise(NoiseParams params, Slice<Vec2f> offsets, float x, float y);
float PerlinFractalNoise(NoiseParams params, Slice<Vec3f> offsets, float x, float y, float z);

#define Spline_Max_Points 20

struct SplinePoint
{
    float x = 0;
    float y = 0;
    float derivative = 0;
};

struct Spline
{
    SplinePoint points[Spline_Max_Points];
    int num_points = 0;
};

int AddPoint(Spline *spline, float x, float y, float derivative);
int AddPoint(Spline *spline, SplinePoint point);
void RemovePoint(Spline *spline, int index);
float SampleSpline(Spline *spline, float t);
