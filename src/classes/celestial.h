#ifndef _CelestialObject
#define _CelestialObject

#include <string>
#include "point.h"
#include "color.h"

enum cel_obj_type
{
    galaxy,
    star,
    gas_giant,
    ice_giant,
    rocky,
    comet,
    artificial
};

class CelestialObject;

class Orbit
{
    public:
    CelestialObject* center = nullptr;
    double ascending_node = 0;                  // RADIANS!
    double inclination = 0;                     // RADIANS!
    double omega = 0;                           // RADIANS!
    double semimajor_axis = 0;
    double eccentricity = 0;
    double mean_anomaly = 0;                    // RADIANS!
    double epoch = 2451544.5;                   // JD

    CelestialLocation compute_3d_location(double epoch);
};

class CelestialObject
{
    public:
    double mass = 0;                            // grams
    double volumetric_mean_radius = 0;          // meters
    double oblateness = 0;
    double sidereal_rotational_period = 0;      // seconds
    double right_ascension = 0;                 // RADIANS!
    double declination = 0;                     // RADIANS!
    double inclination = 0;                     // RADIANS!
    double distance = 0;                        // meters
    double epoch = 2451544.5;                   // JD
    double absolute_magnitude = 0;
    double UB_magnitude = 0;
    double BV_magnitude = 0;
    double VR_magnitude = 0;
    double RI_magnitude = 0;

    cel_obj_type type = star;
    std::string name;

    CelestialObject() {};
    CelestialLocation location;
    Orbit* orbit = nullptr;                     // Most stars won't have an orbit, unless we get into stellar orbital mechanics.
    void update_location(double epoch);         // Only applicable if we have an orbit; otherwise just return.

    double viewer_magnitude(CelestialLocation seen_from);
};


#endif