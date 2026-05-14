
#ifndef _Star
#define _Star

#include <cstdint>
#include "celestial.h"

class Star : public CelestialObject
{
    public:
    double proper_motion_RA = 0;            // radians / second
    double proper_motion_decl = 0;          // radians / second
    double radial_velocity = 0;             // meters / second
    double apparent_magnitude;              // visual/550nm
    double parallax = 0;                    // radians

    std::string spectral_type;
    std::string Bayer;
    std::string Flamsteed;
    std::string Gliese;
    int BayerGrkno = -1;
    int FlamsteedNo = -1;
    std::string constellation;

    __uint32_t HR = 0;                      // Harvard Revised catalog number
    __uint32_t HD = 0;                      // Henry Draper catalog number
    __uint32_t HIP = 0;                     // Hipparcos catalog number
    __uint32_t SAO = 0;                     // USNO/SAO catalog number

    double estimate_temperature();          // kelvin
    void update_location(double epoch);     // Apply proper motion and re-derive 3D coordinates from the result.
    void rename_from_Bayer_Flamsteed();
    bool is_sunlike();
};

void rename_all_from_Bayer_Flamsteed();
void Gliese_doubles_fix();

#endif