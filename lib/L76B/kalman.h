#ifndef KALMAN_H
#define KALMAN_H

#include <Eigen/Dense>

#define M_PI 3.14159265358979323846
#define EARTH_RADIUS 6371000.0 // meters

class KalmanFilter {
public:
    KalmanFilter();

    void predict(float dt);
    void update(float lat_deg, float lon_deg, float speed_mps, float course_deg);

    float getLatitude() const;
    float getLongitude() const;
    float getSpeed() const;
    float getCourse() const;

private:
    Eigen::Vector4d x;      // State vector: [lat_deg, lon_deg, speed_mps, course_deg]
    Eigen::Matrix4d P;      // Estimate uncertainty
    Eigen::Matrix4d Q;      // Process noise
    Eigen::Matrix4d R;      // Measurement noise
    Eigen::Matrix4d I;      // Identity matrix

    float deg2rad(float deg) const;
    float rad2deg(float rad) const;
};

#endif // KALMAN_H
