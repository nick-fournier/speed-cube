#include "pointers.h"
#include "gui.h"

extern "C" {
    #include "LCD_GUI.h"
}

Pointers::Pointers(NavigationGUI* gui) : m_gui(gui) {
}

// Update mark pointer
void Pointers::updateMarkPointer(float bearing_deg) {
    // Erase old pointer
    if (m_prevMarkDeg >= 0) {
        GUI_DrawRadialCircle(m_prevMarkDeg, 10, m_centerX, m_centerY, m_radius + 15, YELLOW);
    }

    // Draw new pointer
    GUI_DrawRadialCircle(bearing_deg, 10, m_centerX, m_centerY, m_radius + 15, YELLOW);
    m_prevMarkDeg = bearing_deg;
}

// Update tack pointer
void Pointers::updateTackPointer(float bearing_deg) {
    // If bearing on starboard, make pointer green
    if (bearing_deg > 180) {
        GUI_DrawRadialTriangle(bearing_deg, m_radius - 5, m_centerX, m_centerY, GREEN, 1);
    } else {
        GUI_DrawRadialTriangle(bearing_deg, m_radius - 5, m_centerX, m_centerY, RED, 1);
    }

    // Erase old pointer
    if (m_prevTackDeg >= 0) {
        GUI_DrawRadialTriangle(m_prevTackDeg, m_radius - 5, m_centerX, m_centerY, BLACK, 1);
    }

    // Draw new pointer
    GUI_DrawRadialTriangle(bearing_deg, m_radius - 5, m_centerX, m_centerY, WHITE, 1);
    m_prevTackDeg = bearing_deg;
}