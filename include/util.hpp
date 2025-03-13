#ifndef UTIL_HPP
#define UTIL_HPP
#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <cmath> 
#include <ostream>
#include <CL/opencl.hpp>
#include <SDL2/SDL.h>

struct vec {
    float x, y, z;

    // Constructor for easy initialization
    vec(float x = 0.0f, float y = 0.0f, float z = 0.0f);

    // Operator overloads for vector arithmetic
    vec operator+(const vec& rhs) const;
    vec operator-(const vec& rhs) const;
    vec& operator+=(const vec& rhs);
    vec& operator-=(const vec& rhs);

    // Operator overloads for scalar multiplication and division
    vec operator*(float scalar) const;
    vec operator/(float scalar) const;
    vec& operator*=(float scalar);
    vec& operator/=(float scalar);

    // Equality and inequality operators
    bool operator==(const vec& rhs) const;
    bool operator!=(const vec& rhs) const;

    // Unary minus operator
    vec operator-() const;

    // Dot and cross products
    float dot(const vec& rhs) const;
    vec cross(const vec& rhs) const;

    // Additional utility functions
    float length() const;
    float squaredLength() const;
    vec normalized() const;
    friend std::ostream& operator<<(std::ostream& os, const vec& v);
};

vec operator^(const vec& v1, const vec& v2);
float operator*(const vec& v1, const vec& v2) ;

// Non-member operator overload for scalar multiplication
vec operator*(float scalar, const vec& v);

vec rotX(vec v, float angle);
vec rotY(vec v, float angle);
vec rotZ(vec v, float angle);

vec rotX(vec v, vec o, float angle);
vec rotY(vec v, vec o, float angle);
vec rotZ(vec v, vec o, float angle);

vec VX(float x, float y, float z);

// RGB color conversion functions
int fromRgb(int r, int g, int b);
void toRgb(int color, int &r, int &g, int &b);

bool monitorExecution(cl_int error);

struct tri{
    Uint32 color;
    vec *a,*b,*c;
};

float pointPlaneDist(vec P, vec A, vec B, vec C);

vec triangleNormal(const vec &A, const vec &B, const vec &C) ;

#endif // UTIL_HPP
