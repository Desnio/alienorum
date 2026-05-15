#include <iostream>
#include <cmath>
#include <iomanip>

#include "planet.h"
#include "point.h"


// Solve Kepler's Equation: M = E - e*sin(E) using Newton's Method
double solve_Kepler(double M, double e)
{
    double E = M; // Initial guess
    double delta;
    do
    {
        delta = E - e * std::sin(E) - M;
        E = E - delta / (1.0 - e * std::cos(E));
    } while (std::abs(delta) > 1e-10);
    return E;
}

double Planet::viewer_reflectance_magnitude(CelestialLocation seen_from)
{
    if (!orbit)
    {
        std::cerr << "Called viewer_reflectance_magnitude on an object without a center of orbit." << std::endl;
        throw 0xbadc0de;
    }
    double r = seen_from.distance_to(location) * invAU;
    double rcen = location.distance_to(orbit->center->location) * invAU;
    double rsq = r*r*rcen*rcen;
    CelestialObject *light_center = orbit->center;
    int i;
    for (i=0; i<5; i++)
    {
        if (light_center->type == rocky || light_center->type == ice_giant || light_center->type == gas_giant)
        {
            light_center = light_center->orbit->center;
            if (!light_center)
            {
                std::cerr << "Cannot find light source for " << name << std::endl;
                throw 0xbadda7a;
            }
        }
        else break;
    }

    double phase = find_3D_angle(seen_from.local_position, light_center->location.local_position, location.local_position);
    double phabs = fabs(phase);
    double amt_lit = fabs(M_PI - fmin(phabs, M_PI*2-phabs)) / M_PI;

    double reflectivity = pow(magnbase, -absolute_magnitude);
    double apparent = reflectivity * amt_lit / rsq;

    /*std::cout << name << ": " << (phase*fiftyseven) << ", " << amt_lit << ", " << reflectivity << ", " << rsq 
        << " = " << (-log(apparent) * invlogmagnbase) << std::endl;*/

    return -log(apparent) * invlogmagnbase;
}

void Planet::update_location(double tmnow)
{
    if (!orbit) return;

    // 1. Calculate current Mean Anomaly
    double rads_sec = (M_PI * 2) / orbit->orbit_period;
    double M = orbit->mean_anomaly + M_PI + rads_sec * (tmnow - J2000_TIME_T + (J2000 - epoch)*86400);
    M = std::fmod(M, 2.0 * M_PI);

    // 2. Solve for Eccentric Anomaly
    double E = solve_Kepler(M, orbit->eccentricity);

    // 3. Calculate position in orbital plane (x', y')
    double x_plane = orbit->semimajor_axis * (std::cos(E) - orbit->eccentricity);
    double y_plane = orbit->semimajor_axis * std::sqrt(1.0 - orbit->eccentricity * orbit->eccentricity) * std::sin(E);

    // 4. Rotate to 3D Heliocentric Coordinates
    double cosO = std::cos(orbit->ascending_node);
    double sinO = std::sin(orbit->ascending_node);
    double cosw = std::cos(orbit->arg_periapsis);
    double sinw = std::sin(orbit->arg_periapsis);
    double cosi = std::cos(orbit->inclination);
    double sini = std::sin(orbit->inclination);

    double x = (cosO * cosw - sinO * sinw * cosi) * x_plane + (-cosO * sinw - sinO * cosw * cosi) * y_plane;
    double y = (sinw * sini) * x_plane + (cosw * sini) * y_plane;
    double z = (sinO * cosw + cosO * sinw * cosi) * x_plane + (-sinO * sinw + cosO * cosw * cosi) * y_plane;

    // TODO: This ecliptic stuff is specific to the solar system.
    // For exoplanets, assume the planetary orbits and stellar equator are in the same plane and set the stellar inclination to zero.
    // Point result = rotate3D(Point(x,y,z), Point(0,0,0), ICRF_to_ecliptic.v, -ICRF_to_ecliptic.a);
    Point result = rotate3D(Point(x,y,z), Point(0,0,0), location.local_plane.v, -location.local_plane.a);

    location.system_center = orbit->center->location.system_center;
    location.local_position = result+ orbit->center->location.local_position;
}

Planet::Planet()
{
    BV_color = 1;
    UB_color = 0.5;
}