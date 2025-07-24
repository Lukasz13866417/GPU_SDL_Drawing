#include "../include/camera.hpp"
#include "../include/log.hpp"

Camera::Camera(float x, float y, float z, float yaw, float pitch, float roll)
    : x(x), y(y), z(z), yaw(yaw), pitch(pitch), roll(roll) {
    LOG_DEBUG("Camera created at position (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
}

void Camera::setPosition(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

void Camera::setOrientation(float yaw, float pitch, float roll) {
    this->yaw = yaw;
    this->pitch = pitch;
    this->roll = roll;
}

void Camera::move(float dx, float dy, float dz) {
    x += dx;
    y += dy;
    z += dz;
}

void Camera::moveForward(float distance) {
    vec forward = getForwardVector();
    x += forward.x * distance;
    y += forward.y * distance;
    z += forward.z * distance;
}

void Camera::moveRight(float distance) {
    vec right = getRightVector();
    x += right.x * distance;
    y += right.y * distance;
    z += right.z * distance;
}

void Camera::moveUp(float distance) {
    y += distance; // Simple world-space up movement
}

void Camera::rotate(float dyaw, float dpitch, float droll) {
    yaw += dyaw;
    pitch += dpitch;
    roll += droll;
    
    // Keep angles in reasonable range
    while (yaw > M_PI) yaw -= 2 * M_PI;
    while (yaw < -M_PI) yaw += 2 * M_PI;
    
    // Clamp pitch to prevent gimbal lock
    if (pitch > M_PI/2 - 0.01f) pitch = M_PI/2 - 0.01f;
    if (pitch < -M_PI/2 + 0.01f) pitch = -M_PI/2 + 0.01f;
}

void Camera::lookAt(float targetX, float targetY, float targetZ) {
    float dx = targetX - x;
    float dy = targetY - y;
    float dz = targetZ - z;
    
    float distance = sqrt(dx*dx + dy*dy + dz*dz);
    if (distance < 0.001f) return; // Avoid division by zero
    
    // Calculate yaw and pitch to look at target
    yaw = atan2(dx, -dz); // Note: -dz because we want +Z to be "back"
    pitch = asin(dy / distance);
    roll = 0.0f; // Keep roll at zero for simplicity
}

vec Camera::getForwardVector() const {
    // Forward is -Z in camera space, so we need the direction the camera is looking
    return vec{
        sin(yaw) * cos(pitch),
        -sin(pitch),
        -cos(yaw) * cos(pitch)
    };
}

vec Camera::getRightVector() const {
    // Right vector is perpendicular to forward in the XZ plane
    return vec{
        cos(yaw),
        0.0f,
        sin(yaw)
    };
}

vec Camera::getUpVector() const {
    // Up vector (accounting for pitch and roll)
    vec forward = getForwardVector();
    vec right = getRightVector();
    
    // Cross product: right × forward = up
    return vec{
        right.y * forward.z - right.z * forward.y,
        right.z * forward.x - right.x * forward.z,
        right.x * forward.y - right.y * forward.x
    };
}

vec Camera::transformVertex(const vec& worldVertex) const {
    // Step 1: Translate to camera space (subtract camera position)
    vec translated = {
        worldVertex.x - x,
        worldVertex.y - y,
        worldVertex.z - z
    };
    
    // Step 2: Apply inverse rotation (rotate by -yaw, -pitch, -roll)
    // We apply rotations in reverse order: roll -> pitch -> yaw
    
    float cosYaw = cos(-yaw);
    float sinYaw = sin(-yaw);
    float cosPitch = cos(-pitch);
    float sinPitch = sin(-pitch);
    float cosRoll = cos(-roll);
    float sinRoll = sin(-roll);
    
    // Combined rotation matrix (yaw * pitch * roll)
    vec result;
    
    // First apply roll rotation (around Z axis)
    float tempX = translated.x * cosRoll - translated.y * sinRoll;
    float tempY = translated.x * sinRoll + translated.y * cosRoll;
    float tempZ = translated.z;
    
    // Then apply pitch rotation (around X axis)
    float tempY2 = tempY * cosPitch - tempZ * sinPitch;
    float tempZ2 = tempY * sinPitch + tempZ * cosPitch;
    tempY = tempY2;
    tempZ = tempZ2;
    
    // Finally apply yaw rotation (around Y axis)
    result.x = tempX * cosYaw + tempZ * sinYaw;
    result.y = tempY;
    result.z = -tempX * sinYaw + tempZ * cosYaw;
    
    return result;
} 