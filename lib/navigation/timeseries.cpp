#include "timeseries.h"
#include "gui.h"

extern "C" {
    #include "LCD_GUI.h"
}

TimeSeriesPlot::TimeSeriesPlot(NavigationGUI* gui) : m_gui(gui) {
}

// Add a new data point to the plot data buffer
void TimeSeriesPlot::addDataPoint(float vmg, float sog, uint32_t timestamp) {
    // Add the new data point to the circular buffer
    m_plotData[m_dataIndex].vmg = vmg;
    m_plotData[m_dataIndex].sog = sog;
    m_plotData[m_dataIndex].timestamp = timestamp;
    
    // Update the index and count
    m_dataIndex = (m_dataIndex + 1) % DATA_POINTS;
    if (m_dataCount < DATA_POINTS) {
        m_dataCount++;
    }
    
    // Update the last update time
    m_lastUpdate = timestamp;
}

// Get the SOG value of the last data point
float TimeSeriesPlot::getLastSOG() const {
    if (m_dataCount == 0) return 0.0f;
    int lastIndex = (m_dataIndex - 1 + DATA_POINTS) % DATA_POINTS;
    return m_plotData[lastIndex].sog;
}

// Clear the plot area (only the data area to reduce flickering)
void TimeSeriesPlot::clearPlotArea() {
    // Clear only the data area with a black rectangle, leaving axes and labels intact
    GUI_DrawRectangle(
        X_START + 1, Y_START, 
        X_END, Y_END - 1, 
        LCD_BACKGROUND, DRAW_FULL, DOT_PIXEL_1X1
    );
}

// Draw the time series plot
void TimeSeriesPlot::drawPlot() {
    // Fixed y-axis range as requested (0-8 knots)
    float minValue = 0;
    float maxValue = 8;
    float yScale = (float)HEIGHT / (maxValue - minValue);
    
    // Draw axes
    GUI_DrawLine(X_START, Y_START, X_START, Y_END, WHITE, LINE_SOLID, DOT_PIXEL_1X1);
    GUI_DrawLine(X_START, Y_END, X_END, Y_END, WHITE, LINE_SOLID, DOT_PIXEL_1X1);
    
    // Draw horizontal grid lines
    int numGridLines = 5; // 0, 2, 4, 6, 8 knots
    for (int i = 0; i < numGridLines; i++) {
        int y = Y_END - (i * HEIGHT) / (numGridLines - 1);
        GUI_DrawLine(X_START, y, X_END, y, GRAY, LINE_DOTTED, DOT_PIXEL_1X1);
        
        // Draw y-axis labels (no decimal places)
        char label[10];
        int value = (i * maxValue) / (numGridLines - 1);
        snprintf(label, sizeof(label), "%d", value);
        GUI_DisString_EN(5, y - 8, label, &Font16, BLACK, WHITE);
    }
    
    // Draw vertical grid lines (time markers)
    int numTimeMarkers = 6; // 0, 1, 2, 3, 4, 5 minutes
    for (int i = 0; i < numTimeMarkers; i++) {
        int x = X_START + (i * WIDTH) / (numTimeMarkers - 1);
        GUI_DrawLine(x, Y_START, x, Y_END, GRAY, LINE_DOTTED, DOT_PIXEL_1X1);
        
        // Draw x-axis labels (minutes) - fixed to show 5, 4, 3, 2, 1, 0
        char label[10];
        snprintf(label, sizeof(label), "%d", 5-i);
        GUI_DisString_EN(x - 5, Y_END + 5, label, &Font16, BLACK, WHITE);
    }
    
    // Draw centered legend
    int legendX = X_START + (WIDTH / 2) - 70; // Center position
    
    // Draw legend
    GUI_DrawLine(legendX, Y_START - 10, legendX + 20, Y_START - 10, CYAN, LINE_SOLID, DOT_PIXEL_2X2);
    GUI_DisString_EN(legendX + 25, Y_START - 15, "VMG", &Font16, BLACK, WHITE);
    
    GUI_DrawLine(legendX + 70, Y_START - 10, legendX + 90, Y_START - 10, YELLOW, LINE_SOLID, DOT_PIXEL_2X2);
    GUI_DisString_EN(legendX + 95, Y_START - 15, "SOG", &Font16, BLACK, WHITE);
    
    // If no data points, just return after drawing the axes and labels
    if (m_dataCount == 0) {
        return;
    }
    
    // Calculate the x scaling factor - always based on full buffer size
    // Each pixel represents DATA_POINTS / WIDTH data points
    float pointsPerPixel = (float)DATA_POINTS / WIDTH;
    
    // Draw the data series
    if (m_dataCount > 1) {
        // Draw VMG data series
        for (int x = 0; x < WIDTH - 1; x++) {
            // Calculate the data indices for this pixel and the next
            // X-axis is inverted: left is t-5min, right is current time
            float dataIdx1 = pointsPerPixel * (WIDTH - 1 - x);
            float dataIdx2 = pointsPerPixel * (WIDTH - x);
            
            // Convert to actual indices in the circular buffer
            int bufferIdx1 = (m_dataIndex - 1 - (int)dataIdx1 + DATA_POINTS) % DATA_POINTS;
            int bufferIdx2 = (m_dataIndex - 1 - (int)dataIdx2 + DATA_POINTS) % DATA_POINTS;
            
            // Skip if we don't have enough data yet (leave blank for non-existent data)
            if (dataIdx1 >= m_dataCount || dataIdx2 >= m_dataCount) {
                continue;
            }
            
            // Calculate screen coordinates
            int x1 = X_START + x;
            int y1 = Y_END - (m_plotData[bufferIdx1].vmg - minValue) * yScale;
            int x2 = X_START + x + 1;
            int y2 = Y_END - (m_plotData[bufferIdx2].vmg - minValue) * yScale;
            
            // Ensure y values are within the plot area
            y1 = (y1 < Y_START) ? Y_START : (y1 > Y_END) ? Y_END : y1;
            y2 = (y2 < Y_START) ? Y_START : (y2 > Y_END) ? Y_END : y2;
            
            GUI_DrawLine(x1, y1, x2, y2, CYAN, LINE_SOLID, DOT_PIXEL_2X2);
        }
        
        // Draw SOG data series
        for (int x = 0; x < WIDTH - 1; x++) {
            // Calculate the data indices for this pixel and the next
            // X-axis is inverted: left is t-5min, right is current time
            float dataIdx1 = pointsPerPixel * (WIDTH - 1 - x);
            float dataIdx2 = pointsPerPixel * (WIDTH - x);
            
            // Convert to actual indices in the circular buffer
            int bufferIdx1 = (m_dataIndex - 1 - (int)dataIdx1 + DATA_POINTS) % DATA_POINTS;
            int bufferIdx2 = (m_dataIndex - 1 - (int)dataIdx2 + DATA_POINTS) % DATA_POINTS;
            
            // Skip if we don't have enough data yet (leave blank for non-existent data)
            if (dataIdx1 >= m_dataCount || dataIdx2 >= m_dataCount) {
                continue;
            }
            
            // Calculate screen coordinates
            int x1 = X_START + x;
            int y1 = Y_END - (m_plotData[bufferIdx1].sog - minValue) * yScale;
            int x2 = X_START + x + 1;
            int y2 = Y_END - (m_plotData[bufferIdx2].sog - minValue) * yScale;
            
            // Ensure y values are within the plot area
            y1 = (y1 < Y_START) ? Y_START : (y1 > Y_END) ? Y_END : y1;
            y2 = (y2 < Y_START) ? Y_START : (y2 > Y_END) ? Y_END : y2;
            
            GUI_DrawLine(x1, y1, x2, y2, YELLOW, LINE_SOLID, DOT_PIXEL_2X2);
        }
    }
}