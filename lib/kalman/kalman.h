#ifndef KALMAN_H
#define KALMAN_H

#include <Eigen/Dense>

#define M_PI 3.14159265358979323846
#define EARTH_RADIUS 6371000.0 // meters

class KalmanFilter {
public:
    KalmanFilter();

    void predict(double dt);
    void update(double lat_deg, double lon_deg, double speed_mps, double course_deg);

    double getLatitude() const;
    double getLongitude() const;
    double getSpeed() const;
    double getCourse() const;

private:
    Eigen::Vector4d x;      // State vector: [lat_deg, lon_deg, speed_mps, course_deg]
    Eigen::Matrix4d P;      // Estimate uncertainty
    Eigen::Matrix4d Q;      // Process noise
    Eigen::Matrix4d R;      // Measurement noise
    Eigen::Matrix4d I;      // Identity matrix

    double deg2rad(double deg) const;
    double rad2deg(double rad) const;
};

#endif // KALMAN_H
