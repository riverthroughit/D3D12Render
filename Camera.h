#pragma once
#include "MyMath.h"
#include<utility>

class Camera
{
    //球坐标系参数
    //https://baike.baidu.com/item/%E7%90%83%E5%9D%90%E6%A0%87%E7%B3%BB/8315363?fr=aladdin
    float theta;
    float phi;
    float radius;
    std::pair<float, float> thetaRange = { 0.1f,DirectX::XM_PI - 0.1f };
    std::pair<float, float> radiusRange = { 3.0f,15.0f };

    DirectX::XMVECTOR pos, target, up;
    float fovAngleY;
    float nearPlane, farPlane;

    void updatePos() {
        float x = radius * sinf(theta) * cosf(phi);
        float z = radius * sinf(theta) * sinf(phi);
        float y = radius * cosf(theta);
        pos = DirectX::XMVectorSet(x, y, z, 1.0f);
    }

public:

    Camera(float fov = DirectX::XM_PIDIV4,
        float n = 1.0f, float f = 1000.0f,
        float t = DirectX::XM_PIDIV4, float p = 1.5f * DirectX::XM_PI, float r = 5.0f) :
        nearPlane(n), farPlane(f), fovAngleY(fov), theta(t), phi(p), radius(r) {
        updatePos();
        target = DirectX::XMVectorZero();
        up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }

    void setTheta(float t) {
        theta = MyMath::clamp(t, thetaRange.first, thetaRange.second);
        updatePos();
    }

    void setPhi(float p) {
        phi = p;
        updatePos();
    }

    void setThetaAndPhi(float t, float p) {
        theta = t;
        phi = p;
        updatePos();
    }

    void setRadius(float r) {
        radius = MyMath::clamp(r, radiusRange.first, radiusRange.second);
        updatePos();
    }

    DirectX::XMVECTOR getPos() {
        return pos;
    }

    DirectX::XMVECTOR getTarget() {
        return target;
    }

    DirectX::XMVECTOR getUp() {
        return up;
    }

    float getFovAngleY() {
        return fovAngleY;
    }

    float getNear() {
        return nearPlane;
    }

    float getFar() {
        return farPlane;
    }

    float getTheta() {
        return theta;
    }

    float getPhi() {
        return phi;
    }

    float getRadius() {
        return radius;
    }
};

