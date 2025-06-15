#ifndef TIMESERIES_H
#define TIMESERIES_H

#include <stdint.h>

class NavigationGUI; // Forward declaration

class TimeSeriesPlot {
public:
    TimeSeriesPlot(NavigationGUI* gui);
    
    // Data structure for plot data points
    struct PlotDataPoint {
        float vmg;      // Velocity Made Good
        float sog;      // Speed Over Ground
        uint32_t timestamp; // Timestamp
    };
    
    // Plot methods
    void drawPlot();
    void clearPlotArea();
    void addDataPoint(float vmg, float sog, uint32_t timestamp);
    
    // Constants for plot dimensions
    static constexpr int DATA_POINTS = 300;  // 5 minutes at 1 point per second
    static constexpr int X_START = 30;       // Left margin for y-axis labels
    static constexpr int Y_START = 300;      // Top of the plot area
    static constexpr int WIDTH = 280;        // Width of the plot area
    static constexpr int HEIGHT = 160;       // Height of the plot area
    static constexpr int X_END = X_START + WIDTH;  // Right edge of plot
    static constexpr int Y_END = Y_START + HEIGHT; // Bottom edge of plot
    
    // Accessor methods
    int getDataCount() const { return m_dataCount; }
    float getLastSOG() const;
    uint32_t getLastUpdateTime() const { return m_lastUpdate; }
    
    // Update interval configuration
    void setUpdateInterval(uint32_t seconds) { m_updateIntervalSecs = seconds; }
    uint32_t getUpdateInterval() const { return m_updateIntervalSecs; }
    bool shouldUpdate(uint32_t currentTime) const;
    void updateLastVisualTimestamp(uint32_t timestamp) { m_lastVisualUpdate = timestamp; }
    uint32_t getLastVisualUpdateTime() const { return m_lastVisualUpdate; }
    
private:
    NavigationGUI* m_gui;
    PlotDataPoint m_plotData[DATA_POINTS]; // Circular buffer for plot data
    int m_dataIndex = 0;      // Current index in the circular buffer
    int m_dataCount = 0;      // Number of valid data points
    uint32_t m_lastUpdate = 0; // Last time the plot was updated
    uint32_t m_lastVisualUpdate = 0; // Last time the plot was visually updated
    uint32_t m_updateIntervalSecs = 1; // Update interval in seconds (default: 1 second)
};

#endif // TIMESERIES_H