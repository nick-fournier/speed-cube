#include "control.h"

NavigationController::NavigationController(L76B& gps, KalmanFilter& filter)
    : gps_(gps), filter_(filter) {
    state_ = {"000000", 0.0, 0.0, 0.0, 0.0, false};
}

void NavigationController::update() {
    GPSData data = gps_.getData();

    if (gps_.Status()) {
        // Update Kalman filter with new raw data
        filter_.update(
            data.latitude,
            data.longitude,
            data.speed,
            data.course
            // data.time,
        );

        // Extract smoothed values from filter
        state_.latitude = filter_.getLatitude();
        state_.longitude = filter_.getLongitude();
        state_.speed = filter_.getSpeed();
        state_.course = filter_.getCourse();
        state_.status = true;
    } else {
        state_.status = false;
    }
}

bool NavigationController::hasFix() const {
    return state_.status;
}

GPSData NavigationController::getDisplayData() const {
    return state_;
}
