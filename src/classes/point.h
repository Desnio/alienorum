
#ifndef _Point
#define _Point

#define AU 149597870700.0
#define speed_of_light 299792458.0
#define earth_mass 5.972e+27
#define jupiter_mass 1.898e+30
#define solar_mass 1.989e+33
#define earth_radius 6.371e+6
#define jupiter_radius 6.9886e+7
#define solar_radius 6.95700e+8

class Point
{
    public:
    double x = 0;
    double y = 0;
    double z = 0;
};

// We cannot simply use 3 dimensional x,y,z coordinates to plot celestial objects in space.
// Why? Because the sheer distances involved are immense, as are the ranges of distances.
// Suppose you use the center of the Milky Way galaxy as [0,0,0].
// Having a point zero in space goes against relativity anyway, but in any case,
// from that point, let's say the Sun is at position [2.5e+20, 0, 0], reflecting our star's
// distance in meters to the galactic center. Earth is around 1.5e+11 meters from the Sun,
// so Earth's X coordinate is going to be somewhere between 2.4999999985e+20 and 2.5000000015e+20.
// That is much too small a variance even for double precision floats, which offer 15 to 17 sig figs.
// Computed locations can be off by as much as 100 kilometers. Put a satellite in low earth orbit
// and part of the time it will show up as being underground.
// This way, the system_center for the Sun and all solar system objects can be 2.5e+20 m from the
// galactic center, while the local_position for the Sun can be [0,0,0] and all solar system
// objects will have their own local positions relative to the Sun.
class CelestialLocation
{
    public:
    Point system_center;
    Point local_position;

    double distance_to(CelestialLocation other);
};

// Takes velocity in m/s and computes the ratio of Δt(moving)/Δt(stationary). The result will always be <= 1.
double compute_time_dilation(double velocity);

#endif
