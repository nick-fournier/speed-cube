#include "L76B.h"
#include "kalman.h"

class NavigationController {
public:
    NavigationController(L76B& gps, KalmanFilter& filter);

    void update();  // Called periodically on Core 0

    bool hasFix() const;
    GPSData getDisplayData() const;

private:
    L76B& gps_;
    KalmanFilter& filter_;
    GPSData state_;
};
