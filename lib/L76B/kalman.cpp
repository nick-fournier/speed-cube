#include "kalman.h"

KalmanFilter::KalmanFilter() {
    x.setZero();
    P.setIdentity();
    Q.setZero();
    R.setZero();
    I.setIdentity();

    // Process noise
    Q(0,0) = 0.01;  // lat
    Q(1,1) = 0.01;  // lon
    Q(2,2) = 0.1;   // speed
    Q(3,3) = 0.1;   // course

    // Measurement noise
    R(0,0) = 1.0;   // lat
    R(1,1) = 1.0;   // lon
    R(2,2) = 0.2;   // speed
    R(3,3) = 3.0;   // course
}

void KalmanFilter::predict(double dt) {
    double lat_rad = deg2rad(x(0));
    double lon_rad = deg2rad(x(1));
    double speed = x(2); // m/s
    double course_rad = deg2rad(x(3));

    double distance = speed * dt;           // meters
    double delta = distance / EARTH_RADIUS; // angular distance

    double new_lat = std::asin(std::sin(lat_rad) * std::cos(delta) +
                               std::cos(lat_rad) * std::sin(delta) * std::cos(course_rad));

    double new_lon = lon_rad + std::atan2(std::sin(course_rad) * std::sin(delta) * std::cos(lat_rad),
                                          std::cos(delta) - std::sin(lat_rad) * std::sin(new_lat));

    x(0) = rad2deg(new_lat);
    x(1) = rad2deg(new_lon);
    // speed and course remain the same

    // Covariance prediction (simple model: P' = P + Q)
    P = P + Q;
}

void KalmanFilter::update(double lat_deg, double lon_deg, double speed_mps, double course_deg) {
    Eigen::Vector4d z;
    z << lat_deg, lon_deg, speed_mps, course_deg;

    Eigen::Matrix4d S = P + R;
    Eigen::Matrix4d K = P * S.inverse();

    x = x + K * (z - x);
    P = (I - K) * P;
}

double KalmanFilter::getLatitude() const {
    return x(0);
}

double KalmanFilter::getLongitude() const {
    return x(1);
}

double KalmanFilter::getSpeed() const {
    return x(2);
}

double KalmanFilter::getCourse() const {
    return x(3);
}

double KalmanFilter::deg2rad(double deg) const {
    return deg * M_PI / 180.0;
}

double KalmanFilter::rad2deg(double rad) const {
    return rad * 180.0 / M_PI;
}
