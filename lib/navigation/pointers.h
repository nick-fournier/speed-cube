#ifndef POINTERS_H
#define POINTERS_H

class NavigationGUI; // Forward declaration

class Pointers {
public:
    Pointers(NavigationGUI* gui);
    
    // Update mark pointer
    void updateMarkPointer(float bearing_deg);
    
    // Update tack pointer
    void updateTackPointer(float bearing_deg);
    
    // Set center and radius
    void setCenter(int x, int y) { m_centerX = x; m_centerY = y; }
    void setRadius(int r) { m_radius = r; }
    
private:
    NavigationGUI* m_gui;
    
    // Display parameters
    int m_centerX = 160;  // Center X coordinate of the display
    int m_centerY = 150;  // Center Y coordinate of the display
    int m_radius = 130;   // Radius of the circle
    
    // Internal variables
    float m_prevTackDeg = -1;  // -1 indicates "no arrow drawn"
    float m_prevMarkDeg = -1;  // -1 indicates "no arrow drawn"
};

#endif // POINTERS_H