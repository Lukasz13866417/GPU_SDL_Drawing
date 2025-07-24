#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "util.hpp"
#include <cmath>

// Camera class for 3D transformations
class Camera {
private:
    // Camera position in world space
    float x, y, z;
    
    // Camera orientation (in radians)
    float yaw, pitch, roll;
    
public:
    // Constructor - default camera at origin looking down -Z axis
    Camera(float x = 0.0f, float y = 0.0f, float z = 0.0f, 
           float yaw = 0.0f, float pitch = 0.0f, float roll = 0.0f);
    
    // Position setters/getters
    void setPosition(float x, float y, float z);
    void setX(float x) { this->x = x; }
    void setY(float y) { this->y = y; }
    void setZ(float z) { this->z = z; }
    
    float getX() const { return x; }
    float getY() const { return y; }
    float getZ() const { return z; }
    
    // Orientation setters/getters (in radians)
    void setOrientation(float yaw, float pitch, float roll);
    void setYaw(float yaw) { this->yaw = yaw; }
    void setPitch(float pitch) { this->pitch = pitch; }
    void setRoll(float roll) { this->roll = roll; }
    
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }
    float getRoll() const { return roll; }
    
    // Convenience methods for degrees
    void setYawDegrees(float degrees) { yaw = degrees * M_PI / 180.0f; }
    void setPitchDegrees(float degrees) { pitch = degrees * M_PI / 180.0f; }
    void setRollDegrees(float degrees) { roll = degrees * M_PI / 180.0f; }
    
    float getYawDegrees() const { return yaw * 180.0f / M_PI; }
    float getPitchDegrees() const { return pitch * 180.0f / M_PI; }
    float getRollDegrees() const { return roll * 180.0f / M_PI; }
    
    // Movement methods
    void move(float dx, float dy, float dz);
    void moveForward(float distance);  // Move along current view direction
    void moveRight(float distance);    // Strafe right
    void moveUp(float distance);       // Move up in world space
    
    // Rotation methods
    void rotate(float dyaw, float dpitch, float droll);
    void lookAt(float targetX, float targetY, float targetZ);
    
    // Transform a vertex from world space to camera space
    vec transformVertex(const vec& worldVertex) const;
    
    // Get view direction vector
    vec getForwardVector() const;
    vec getRightVector() const;
    vec getUpVector() const;
};

#endif // CAMERA_HPP 