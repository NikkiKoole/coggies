#ifndef MY_MATH_H
#define MY_MATH_H

#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#if defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

//loads of errors: error: suggest braces around initialization of subobject
// which would want double curly braces around all initializers, screw that, its a clang bug
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"


struct _Matrix4
{
    float m[16];
} __attribute__((aligned(16)));
typedef struct _Matrix4 Matrix4;

union _Vector3
{
    struct { float x, y, z; };
    struct { float r, g, b; };
    struct { float s, t, p; };
    float v[3];
};
typedef union _Vector3 Vector3;

static __inline__ Vector3 Vector3Make(float x, float y, float z);
static __inline__ float Vector3Distance(Vector3 vectorStart, Vector3 vectorEnd);

static __inline__ Vector3 Vector3Limit(Vector3 vector, float max);
static __inline__ Vector3 Vector3Add(Vector3 vectorLeft, Vector3 vectorRight);
static __inline__ Vector3 Vector3Subtract(Vector3 vectorLeft, Vector3 vectorRight);
static __inline__ Vector3 Vector3MultiplyScalar(Vector3 vector, float value);
static __inline__ Vector3 Vector3DivideScalar(Vector3 vector, float value);

static __inline__ Vector3 Vector3Normalize(Vector3 vector);
static __inline__ float Vector3DotProduct(Vector3 vectorLeft, Vector3 vectorRight);
static __inline__ Vector3 Vector3CrossProduct(Vector3 vectorLeft, Vector3 vectorRight);

static __inline__ float Vector3Length(Vector3 vector);
static __inline__ Vector3 Vector3Negate(Vector3 vector);


static __inline__ Vector3 Vector3Make(float x, float y, float z)
{
    Vector3 v = {x, y, z};
    return v;
}
static __inline__ float Vector3Distance(Vector3 vectorStart, Vector3 vectorEnd)
{
    return Vector3Length(Vector3Subtract(vectorEnd, vectorStart));
}

static __inline__ Vector3 Vector3Negate(Vector3 vector)
{
    Vector3 v = { -vector.v[0], -vector.v[1], -vector.v[2] };
    return v;
}
static __inline__ float Vector3Length(Vector3 vector)
{
    return sqrt(vector.v[0] * vector.v[0] + vector.v[1] * vector.v[1] + vector.v[2] * vector.v[2]);
}
static __inline__ Vector3 Vector3Limit(Vector3 vector, float max) {
    if (Vector3Length(vector) > max) {
        return Vector3MultiplyScalar(Vector3Normalize(vector),max);
    } else {
        return  vector;
    }
}
static __inline__ Vector3 Vector3Add(Vector3 vectorLeft, Vector3 vectorRight)
{
    Vector3 v = { vectorLeft.v[0] + vectorRight.v[0],
                     vectorLeft.v[1] + vectorRight.v[1],
                     vectorLeft.v[2] + vectorRight.v[2] };
    return v;
}
static __inline__ Vector3 Vector3Subtract(Vector3 vectorLeft, Vector3 vectorRight)
{
    Vector3 v = { vectorLeft.v[0] - vectorRight.v[0],
                     vectorLeft.v[1] - vectorRight.v[1],
                     vectorLeft.v[2] - vectorRight.v[2] };
    return v;
}
static __inline__ Vector3 Vector3MultiplyScalar(Vector3 vector, float value)
{
    Vector3 v = { vector.v[0] * value,
                     vector.v[1] * value,
                     vector.v[2] * value };
    return v;
}
static __inline__ Vector3 Vector3DivideScalar(Vector3 vector, float value)
{
    Vector3 v = { vector.v[0] / value,
                     vector.v[1] / value,
                     vector.v[2] / value };
    return v;
}
static __inline__ Vector3 Vector3Normalize(Vector3 vector)
{
    float scale = 1.0f / Vector3Length(vector);
    Vector3 v = { vector.v[0] * scale, vector.v[1] * scale, vector.v[2] * scale };
    return v;
}
static __inline__ float Vector3DotProduct(Vector3 vectorLeft, Vector3 vectorRight)
{
    return vectorLeft.v[0] * vectorRight.v[0] + vectorLeft.v[1] * vectorRight.v[1] + vectorLeft.v[2] * vectorRight.v[2];
}
static __inline__ Vector3 Vector3CrossProduct(Vector3 vectorLeft, Vector3 vectorRight)
{
    Vector3 v = { vectorLeft.v[1] * vectorRight.v[2] - vectorLeft.v[2] * vectorRight.v[1],
                     vectorLeft.v[2] * vectorRight.v[0] - vectorLeft.v[0] * vectorRight.v[2],
                     vectorLeft.v[0] * vectorRight.v[1] - vectorLeft.v[1] * vectorRight.v[0] };
    return v;
}











// MATRIX STUFF

static __inline__ Matrix4 Matrix4MakeWithArray(float values[16]);
static __inline__ Matrix4 Matrix4MakeOrtho(float left, float right,
                                                 float bottom, float top,
                                                 float nearZ, float farZ);
static __inline__ Matrix4 Matrix4MakeLookAt(float eyeX, float eyeY, float eyeZ,
                                                  float centerX, float centerY, float centerZ,
                                                  float upX, float upY, float upZ);
static __inline__ Matrix4 Matrix4Multiply(Matrix4 matrixLeft, Matrix4 matrixRight);
static __inline__ Matrix4 Matrix4Translate(Matrix4 matrix, float tx, float ty, float tz);
static __inline__ Matrix4 Matrix4RotateZ(Matrix4 matrix, float radians);
static __inline__ Matrix4 Matrix4MakeZRotation(float radians);


static __inline__ Matrix4 Matrix4MakeWithArray(float values[16])
{
    Matrix4 m = { values[0], values[1], values[2], values[3],
                     values[4], values[5], values[6], values[7],
                     values[8], values[9], values[10], values[11],
                     values[12], values[13], values[14], values[15] };
    return m;
}

static __inline__ Matrix4 Matrix4MakeOrtho(float left, float right,
                                                 float bottom, float top,
                                                 float nearZ, float farZ)
{
    float ral = right + left;
    float rsl = right - left;
    float tab = top + bottom;
    float tsb = top - bottom;
    float fan = farZ + nearZ;
    float fsn = farZ - nearZ;

    Matrix4 m = { 2.0f / rsl, 0.0f, 0.0f, 0.0f,
                     0.0f, 2.0f / tsb, 0.0f, 0.0f,
                     0.0f, 0.0f, -2.0f / fsn, 0.0f,
                     -ral / rsl, -tab / tsb, -fan / fsn, 1.0f };

    return m;
}
static __inline__ Matrix4 Matrix4MakeLookAt(float eyeX, float eyeY, float eyeZ,
                                                  float centerX, float centerY, float centerZ,
                                                  float upX, float upY, float upZ)
{
    Vector3 ev = { eyeX, eyeY, eyeZ };
    Vector3 cv = { centerX, centerY, centerZ };
    Vector3 uv = { upX, upY, upZ };
    Vector3 n = Vector3Normalize(Vector3Add(ev, Vector3Negate(cv)));
    Vector3 u = Vector3Normalize(Vector3CrossProduct(uv, n));
    Vector3 v = Vector3CrossProduct(n, u);

    Matrix4 m = { u.v[0], v.v[0], n.v[0], 0.0f,
                     u.v[1], v.v[1], n.v[1], 0.0f,
                     u.v[2], v.v[2], n.v[2], 0.0f,
                     Vector3DotProduct(Vector3Negate(u), ev),
                     Vector3DotProduct(Vector3Negate(v), ev),
                     Vector3DotProduct(Vector3Negate(n), ev),
                     1.0f };

    return m;
}
static __inline__ Matrix4 Matrix4Multiply(Matrix4 matrixLeft, Matrix4 matrixRight)
{
#if defined(__ARM_NEON__)
    float32x4x4_t iMatrixLeft = *(float32x4x4_t *)&matrixLeft;
    float32x4x4_t iMatrixRight = *(float32x4x4_t *)&matrixRight;
    float32x4x4_t m;

    m.val[0] = vmulq_n_f32(iMatrixLeft.val[0], vgetq_lane_f32(iMatrixRight.val[0], 0));
    m.val[1] = vmulq_n_f32(iMatrixLeft.val[0], vgetq_lane_f32(iMatrixRight.val[1], 0));
    m.val[2] = vmulq_n_f32(iMatrixLeft.val[0], vgetq_lane_f32(iMatrixRight.val[2], 0));
    m.val[3] = vmulq_n_f32(iMatrixLeft.val[0], vgetq_lane_f32(iMatrixRight.val[3], 0));

    m.val[0] = vmlaq_n_f32(m.val[0], iMatrixLeft.val[1], vgetq_lane_f32(iMatrixRight.val[0], 1));
    m.val[1] = vmlaq_n_f32(m.val[1], iMatrixLeft.val[1], vgetq_lane_f32(iMatrixRight.val[1], 1));
    m.val[2] = vmlaq_n_f32(m.val[2], iMatrixLeft.val[1], vgetq_lane_f32(iMatrixRight.val[2], 1));
    m.val[3] = vmlaq_n_f32(m.val[3], iMatrixLeft.val[1], vgetq_lane_f32(iMatrixRight.val[3], 1));

    m.val[0] = vmlaq_n_f32(m.val[0], iMatrixLeft.val[2], vgetq_lane_f32(iMatrixRight.val[0], 2));
    m.val[1] = vmlaq_n_f32(m.val[1], iMatrixLeft.val[2], vgetq_lane_f32(iMatrixRight.val[1], 2));
    m.val[2] = vmlaq_n_f32(m.val[2], iMatrixLeft.val[2], vgetq_lane_f32(iMatrixRight.val[2], 2));
    m.val[3] = vmlaq_n_f32(m.val[3], iMatrixLeft.val[2], vgetq_lane_f32(iMatrixRight.val[3], 2));

    m.val[0] = vmlaq_n_f32(m.val[0], iMatrixLeft.val[3], vgetq_lane_f32(iMatrixRight.val[0], 3));
    m.val[1] = vmlaq_n_f32(m.val[1], iMatrixLeft.val[3], vgetq_lane_f32(iMatrixRight.val[1], 3));
    m.val[2] = vmlaq_n_f32(m.val[2], iMatrixLeft.val[3], vgetq_lane_f32(iMatrixRight.val[2], 3));
    m.val[3] = vmlaq_n_f32(m.val[3], iMatrixLeft.val[3], vgetq_lane_f32(iMatrixRight.val[3], 3));

    return *(Matrix4 *)&m;
#else
    Matrix4 m;

    m.m[0]  = matrixLeft.m[0] * matrixRight.m[0]  + matrixLeft.m[4] * matrixRight.m[1]  + matrixLeft.m[8] * matrixRight.m[2]   + matrixLeft.m[12] * matrixRight.m[3];
	m.m[4]  = matrixLeft.m[0] * matrixRight.m[4]  + matrixLeft.m[4] * matrixRight.m[5]  + matrixLeft.m[8] * matrixRight.m[6]   + matrixLeft.m[12] * matrixRight.m[7];
	m.m[8]  = matrixLeft.m[0] * matrixRight.m[8]  + matrixLeft.m[4] * matrixRight.m[9]  + matrixLeft.m[8] * matrixRight.m[10]  + matrixLeft.m[12] * matrixRight.m[11];
	m.m[12] = matrixLeft.m[0] * matrixRight.m[12] + matrixLeft.m[4] * matrixRight.m[13] + matrixLeft.m[8] * matrixRight.m[14]  + matrixLeft.m[12] * matrixRight.m[15];

	m.m[1]  = matrixLeft.m[1] * matrixRight.m[0]  + matrixLeft.m[5] * matrixRight.m[1]  + matrixLeft.m[9] * matrixRight.m[2]   + matrixLeft.m[13] * matrixRight.m[3];
	m.m[5]  = matrixLeft.m[1] * matrixRight.m[4]  + matrixLeft.m[5] * matrixRight.m[5]  + matrixLeft.m[9] * matrixRight.m[6]   + matrixLeft.m[13] * matrixRight.m[7];
	m.m[9]  = matrixLeft.m[1] * matrixRight.m[8]  + matrixLeft.m[5] * matrixRight.m[9]  + matrixLeft.m[9] * matrixRight.m[10]  + matrixLeft.m[13] * matrixRight.m[11];
	m.m[13] = matrixLeft.m[1] * matrixRight.m[12] + matrixLeft.m[5] * matrixRight.m[13] + matrixLeft.m[9] * matrixRight.m[14]  + matrixLeft.m[13] * matrixRight.m[15];

	m.m[2]  = matrixLeft.m[2] * matrixRight.m[0]  + matrixLeft.m[6] * matrixRight.m[1]  + matrixLeft.m[10] * matrixRight.m[2]  + matrixLeft.m[14] * matrixRight.m[3];
	m.m[6]  = matrixLeft.m[2] * matrixRight.m[4]  + matrixLeft.m[6] * matrixRight.m[5]  + matrixLeft.m[10] * matrixRight.m[6]  + matrixLeft.m[14] * matrixRight.m[7];
	m.m[10] = matrixLeft.m[2] * matrixRight.m[8]  + matrixLeft.m[6] * matrixRight.m[9]  + matrixLeft.m[10] * matrixRight.m[10] + matrixLeft.m[14] * matrixRight.m[11];
	m.m[14] = matrixLeft.m[2] * matrixRight.m[12] + matrixLeft.m[6] * matrixRight.m[13] + matrixLeft.m[10] * matrixRight.m[14] + matrixLeft.m[14] * matrixRight.m[15];

	m.m[3]  = matrixLeft.m[3] * matrixRight.m[0]  + matrixLeft.m[7] * matrixRight.m[1]  + matrixLeft.m[11] * matrixRight.m[2]  + matrixLeft.m[15] * matrixRight.m[3];
	m.m[7]  = matrixLeft.m[3] * matrixRight.m[4]  + matrixLeft.m[7] * matrixRight.m[5]  + matrixLeft.m[11] * matrixRight.m[6]  + matrixLeft.m[15] * matrixRight.m[7];
	m.m[11] = matrixLeft.m[3] * matrixRight.m[8]  + matrixLeft.m[7] * matrixRight.m[9]  + matrixLeft.m[11] * matrixRight.m[10] + matrixLeft.m[15] * matrixRight.m[11];
	m.m[15] = matrixLeft.m[3] * matrixRight.m[12] + matrixLeft.m[7] * matrixRight.m[13] + matrixLeft.m[11] * matrixRight.m[14] + matrixLeft.m[15] * matrixRight.m[15];

    return m;
#endif
}
static __inline__ Matrix4 Matrix4Translate(Matrix4 matrix, float tx, float ty, float tz)
{
    Matrix4 m = { matrix.m[0], matrix.m[1], matrix.m[2], matrix.m[3],
                     matrix.m[4], matrix.m[5], matrix.m[6], matrix.m[7],
                     matrix.m[8], matrix.m[9], matrix.m[10], matrix.m[11],
                     matrix.m[0] * tx + matrix.m[4] * ty + matrix.m[8] * tz + matrix.m[12],
                     matrix.m[1] * tx + matrix.m[5] * ty + matrix.m[9] * tz + matrix.m[13],
                     matrix.m[2] * tx + matrix.m[6] * ty + matrix.m[10] * tz + matrix.m[14],
                     matrix.m[15] };
    return m;
}
static __inline__ Matrix4 Matrix4MakeZRotation(float radians)
{
    float cos = cosf(radians);
    float sin = sinf(radians);

    Matrix4 m = { cos, sin, 0.0f, 0.0f,
                     -sin, cos, 0.0f, 0.0f,
                     0.0f, 0.0f, 1.0f, 0.0f,
                     0.0f, 0.0f, 0.0f, 1.0f };

    return m;
}

static __inline__ Matrix4 Matrix4RotateZ(Matrix4 matrix, float radians)
{
    Matrix4 rm = Matrix4MakeZRotation(radians);
    return Matrix4Multiply(matrix, rm);
}

#pragma GCC diagnostic pop

#endif
