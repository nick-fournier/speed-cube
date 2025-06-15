#ifndef MARKS_H
#define MARKS_H

#include <array>

namespace Navigation {

struct Mark {
    char name[10];  // Name of the mark (removed const to allow assignment)
    float lat;      // Latitude in decimal degrees
    float lon;      // Longitude in decimal degrees
};

// Predefined mark coordinates for San Francisco Bay
constexpr std::array<Mark, 8> MARKS = {{
    {"SBYC", 37.77797371, -122.3852661},
    {"SC1",  37.77555,    -122.3658167},
    {"NAS1", 37.77725,    -122.3412833},
    {"NAS2", 37.774183,   -122.3410167},
    {"YB",   37.79951667, -122.3605167},
    {"AS1",  37.771383,   -122.3830167},
    {"34",   37.75835,    -122.3685333},
    {"33",   37.801,      -122.3477333}
}};

} // namespace Navigation

#endif // MARKS_H