#define CL_HPP_TARGET_OPENCL_VERSION 300
#include "util.hpp"
#include <ostream>
#include <iostream>
#include<CL/opencl.hpp>
vec::vec(float x, float y, float z) : x(x), y(y), z(z) {}

vec vec::operator+(const vec& rhs) const {
    return vec(x + rhs.x, y + rhs.y, z + rhs.z);
}

vec vec::operator-(const vec& rhs) const {
    return vec(x - rhs.x, y - rhs.y, z - rhs.z);
}

vec& vec::operator+=(const vec& rhs) {
    x += rhs.x; y += rhs.y; z += rhs.z;
    return *this;
}

vec& vec::operator-=(const vec& rhs) {
    x -= rhs.x; y -= rhs.y; z -= rhs.z;
    return *this;
}

vec vec::operator*(float scalar) const {
    return vec(x * scalar, y * scalar, z * scalar);
}

vec vec::operator/(float scalar) const {
    return vec(x / scalar, y / scalar, z / scalar);
}

vec& vec::operator*=(float scalar) {
    x *= scalar; y *= scalar; z *= scalar;
    return *this;
}

vec& vec::operator/=(float scalar) {
    x /= scalar; y /= scalar; z /= scalar;
    return *this;
}

bool vec::operator==(const vec& rhs) const {
    return x == rhs.x && y == rhs.y && z == rhs.z;
}

bool vec::operator!=(const vec& rhs) const {
    return !(*this == rhs);
}

vec vec::operator-() const {
    return vec(-x, -y, -z);
}

float vec::dot(const vec& rhs) const {
    return x * rhs.x + y * rhs.y + z * rhs.z;
}

vec vec::cross(const vec& rhs) const {
    return vec(
        y * rhs.z - z * rhs.y,
        z * rhs.x - x * rhs.z,
        x * rhs.y - y * rhs.x
    );
}

vec operator^(const vec& v1, const vec& v2) {
    return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x};
}

float operator*(const vec& v1, const vec& v2) {
    return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

float vec::length() const {
    return std::sqrt(x * x + y * y + z * z);
}

float vec::squaredLength() const {
    return x * x + y * y + z * z;
}

vec vec::normalized() const {
    float len = length();
    if (len == 0) return *this;
    return *this / len;
}

vec VX(float x, float y, float z){
    return vec{x,y,z};
}

vec operator*(float scalar, const vec& v) {
    return v * scalar;
}

vec rotX(vec v, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);

    return vec{
        v.x,
        cosA * v.y - sinA * v.z,
        sinA * v.y + cosA * v.z
    };
}

vec rotY(vec v, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);

    return vec{
        cosA * v.x + sinA * v.z,
        v.y,
        -sinA * v.x + cosA * v.z
    };
}

vec rotZ(vec v, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);

    return vec{
        cosA * v.x - sinA * v.y,
        sinA * v.x + cosA * v.y,
        v.z
    };
}

vec rotX(vec v, vec o, float angle){
    return rotX(v-o,angle)+o;
}

vec rotY(vec v, vec o, float angle){
    return rotY(v-o,angle)+o;
}

vec rotZ(vec v, vec o, float angle){
    return rotZ(v-o,angle)+o;
}


std::ostream& operator<<(std::ostream& os, const vec& v) {
    os << '(' << v.x << ", " << v.y << ", " << v.z << ')';
    return os;
}

int fromRgb(int r, int g, int b) {
    return (r << 16) | (g << 8) | b;
}

void toRgb(int color, int &r, int &g, int &b) {
    r = (color >> 16) & 0xFF;
    g = (color >> 8) & 0xFF;
    b = color & 0xFF;
}

bool monitorExecution(cl_int error)
{
    switch(error){
        // run-time and JIT compiler errors
        case 0:
            return 1;
        case -1: std::cerr<<"CL_DEVICE_NOT_FOUND\n";return 0;
        case -2: std::cerr<< "CL_DEVICE_NOT_AVAILABLE\n";return 0;
        case -3: std::cerr<< "CL_COMPILER_NOT_AVAILABLE\n";return 0;
        case -4: std::cerr<< "CL_MEM_OBJECT_ALLOCATION_FAILURE\n";return 0;
        case -5: std::cerr<< "CL_OUT_OF_RESOURCES\n";return 0;
        case -6: std::cerr<< "CL_OUT_OF_HOST_MEMORY\n";return 0;
        case -7: std::cerr<< "CL_PROFILING_INFO_NOT_AVAILABLE\n";return 0;
        case -8: std::cerr<< "CL_MEM_COPY_OVERLAP\n";return 0;
        case -9: std::cerr<< "CL_IMAGE_FORMAT_MISMATCH\n";return 0;
        case -10: std::cerr<< "CL_IMAGE_FORMAT_NOT_SUPPORTED\n";return 0;
        case -11: std::cerr<< "CL_BUILD_PROGRAM_FAILURE\n";return 0;
        case -12: std::cerr<< "CL_MAP_FAILURE\n";return 0;
        case -13: std::cerr<< "CL_MISALIGNED_SUB_BUFFER_OFFSET\n";return 0;
        case -14: std::cerr<< "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST\n";return 0;
        case -15: std::cerr<< "CL_COMPILE_PROGRAM_FAILURE\n";return 0;
        case -16: std::cerr<< "CL_LINKER_NOT_AVAILABLE\n";return 0;
        case -17: std::cerr<< "CL_LINK_PROGRAM_FAILURE\n";return 0;
        case -18: std::cerr<< "CL_DEVICE_PARTITION_FAILED\n";return 0;
        case -19: std::cerr<< "CL_KERNEL_ARG_INFO_NOT_AVAILABLE\n";return 0;

        // compile-time errors
        case -30: std::cerr<< "CL_INVALID_VALUE\n";return 0;
        case -31: std::cerr<< "CL_INVALID_DEVICE_TYPE\n";return 0;
        case -32: std::cerr<< "CL_INVALID_PLATFORM\n";return 0;
        case -33: std::cerr<< "CL_INVALID_DEVICE\n";return 0;
        case -34: std::cerr<< "CL_INVALID_CONTEXT\n";return 0;
        case -35: std::cerr<< "CL_INVALID_QUEUE_PROPERTIES\n";return 0;
        case -36: std::cerr<< "CL_INVALID_COMMAND_QUEUE\n";return 0;
        case -37: std::cerr<< "CL_INVALID_HOST_PTR\n";return 0;
        case -38: std::cerr<< "CL_INVALID_MEM_OBJECT\n";return 0;
        case -39: std::cerr<< "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR\n";return 0;
        case -40: std::cerr<< "CL_INVALID_IMAGE_SIZE\n";return 0;
        case -41: std::cerr<< "CL_INVALID_SAMPLER\n";return 0;
        case -42: std::cerr<< "CL_INVALID_BINARY\n";return 0;
        case -43: std::cerr<< "CL_INVALID_BUILD_OPTIONS\n";return 0;
        case -44: std::cerr<< "CL_INVALID_PROGRAM\n";return 0;
        case -45: std::cerr<< "CL_INVALID_PROGRAM_EXECUTABLE\n";return 0;
        case -46: std::cerr<< "CL_INVALID_KERNEL_NAME\n";return 0;
        case -47: std::cerr<< "CL_INVALID_KERNEL_DEFINITION\n";return 0;
        case -48: std::cerr<< "CL_INVALID_KERNEL\n";return 0;
        case -49: std::cerr<< "CL_INVALID_ARG_INDEX\n";return 0;
        case -50: std::cerr<< "CL_INVALID_ARG_VALUE\n";return 0;
        case -51: std::cerr<< "CL_INVALID_ARG_SIZE\n";return 0;
        case -52: std::cerr<< "CL_INVALID_KERNEL_ARGS\n";return 0;
        case -53: std::cerr<< "CL_INVALID_WORK_DIMENSION\n";return 0;
        case -54: std::cerr<< "CL_INVALID_WORK_GROUP_SIZE\n";return 0;
        case -55: std::cerr<< "CL_INVALID_WORK_ITEM_SIZE\n";return 0;
        case -56: std::cerr<< "CL_INVALID_GLOBAL_OFFSET\n";return 0;
        case -57: std::cerr<< "CL_INVALID_EVENT_WAIT_LIST\n";return 0;
        case -58: std::cerr<< "CL_INVALID_EVENT\n";return 0;
        case -59: std::cerr<< "CL_INVALID_OPERATION\n";return 0;
        case -60: std::cerr<< "CL_INVALID_GL_OBJECT\n";return 0;
        case -61: std::cerr<< "CL_INVALID_BUFFER_SIZE\n";return 0;
        case -62: std::cerr<< "CL_INVALID_MIP_LEVEL\n";return 0;
        case -63: std::cerr<< "CL_INVALID_GLOBAL_WORK_SIZE\n";return 0;
        case -64: std::cerr<< "CL_INVALID_PROPERTY\n";return 0;
        case -65: std::cerr<< "CL_INVALID_IMAGE_DESCRIPTOR\n";return 0;
        case -66: std::cerr<< "CL_INVALID_COMPILER_OPTIONS\n";return 0;
        case -67: std::cerr<< "CL_INVALID_LINKER_OPTIONS\n";return 0;
        case -68: std::cerr<< "CL_INVALID_DEVICE_PARTITION_COUNT\n";return 0;

        // extension errors
        case -1000: std::cerr<< "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR\n";return 0;
        case -1001: std::cerr<< "CL_PLATFORM_NOT_FOUND_KHR\n";return 0;
        case -1002: std::cerr<< "CL_INVALID_D3D10_DEVICE_KHR\n";return 0;
        case -1003: std::cerr<< "CL_INVALID_D3D10_RESOURCE_KHR\n";return 0;
        case -1004: std::cerr<< "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR\n";return 0;
        case -1005: std::cerr<< "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR\n";return 0;
    }
    std::cerr<<"Unknown OpenCL error";
    return 0;
}

float pointPlaneDist(vec A, vec B, vec C, vec P) {
    vec AB = B - A; 
    vec AC = C - A; 
    vec normal = AB ^ AC;
    normal = normal/normal.length(); 
    vec AP = P - A; 
    return (AP * normal); 
}

vec triangleNormal(const vec &A, const vec &B, const vec &C) {
    vec edge1 = B - A;
    vec edge2 = C - A;
    return edge1.cross(edge2).normalized();
}