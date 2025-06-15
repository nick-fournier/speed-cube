#include "simulation.h"
#include "gui.h"
#include "timeseries.h"
#include "math.h"

Simulation::Simulation(NavigationGUI* gui, TimeSeriesPlot* timeSeries) : gui(gui), m_timeSeries(timeSeries) {
}

// Add incremental simulated data (one point at a time)
void Simulation::addIncrementalSimulatedData() {
    // Generate one new data point
    float i = m_timestamp - 1000; // For the sine wave calculation
    
    // SOG varies between 3 and 6 knots
    float sog = 3.0 + (sin(i * 0.1) + 1) * 1.5;
    
    // Use a fixed course for simulation
    float course = 135.0;
    
    // Calculate VMG using the same method as real data for consistency
    float vmg = gui->calculateVMG(sog, course, gui->getTargetBearing());
    
    // Add the data point
    m_timeSeries->addDataPoint(vmg, sog, m_timestamp);
    
    // Increment the timestamp for next time
    m_timestamp++;
}