
#ifndef _Star
#define _Star

#include "celestial.h"

class Star : public CelestialObject
{
    double proper_motion_RA = 0;            // radians / second
    double proper_motion_decl = 0;          // radians / second

    void update_location(double epoch);     // Apply proper motion and re-derive 3D coordinates from the result.
};

#endif