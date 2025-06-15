#ifndef SIMULATION_H
#define SIMULATION_H

#include "gps_data.h"

class NavigationGUI; // Forward declaration
class TimeSeriesPlot; // Forward declaration

class Simulation {
public:
    Simulation(NavigationGUI* gui, TimeSeriesPlot* timeSeries);
    
    // Add incremental simulated data (one point at a time)
    void addIncrementalSimulatedData();
    
    // Simulation control
    void setActive(bool active) { m_isActive = active; }
    bool isActive() const { return m_isActive; }
    
    // Get current simulated timestamp
    uint32_t getTimestamp() const { return m_timestamp; }
    
private:
    NavigationGUI* gui;
    TimeSeriesPlot* m_timeSeries;
    uint32_t m_timestamp = 1000; // Base timestamp for simulated data
    bool m_isActive = false;      // Flag to control simulation
};

#endif // SIMULATION_H