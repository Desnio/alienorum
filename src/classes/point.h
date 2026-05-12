
#include <string>

#ifndef _Point
#define _Point

#define light_year 9460730472580800
#define parsec 3.08567758128E+16
#define AU 149597870700.0
#define day 86400
#define year (365 * 86400 + 5 * 3600 + 48 * 60 + 45)
#define speed_of_light 299792458.0
#define earth_mass 5.972e+27
#define jupiter_mass 1.898e+30
#define solar_mass 1.989e+33
#define earth_radius 6.371e+6
#define jupiter_radius 6.9886e+7
#define solar_radius 6.95700e+8
#define fiftyseven (180.0/M_PI)
#define fiftyseventh (M_PI/180)
#define arcminute (fiftyseventh / 60)
#define arcsecond (arcminute / 60)

#define its_behind_you 0xbe419d10

class Point;
class CelestialLocation;

class Cartesian2D
{
    public:
    double x = 0;
    double y = 0;

    Cartesian2D() {};
    Cartesian2D(Point, double azimuth = 0, double altitude = 0, double zoom = 1.0);
};

class Point
{
    public:
    double x = 0;
    double y = 0;
    double z = 0;

    Point() {};
    Point(double x, double y, double z);
    Point(CelestialLocation& cel);
    Point operator+(Point other);
    Point& operator+=(Point other);
    Point operator-(Point other);
    Point& operator-=(Point other);
    double distance_to(Point other);
    double magnitude() const;
    void scale(double new_magn);

    static Point from_ra_dec(double right_ascension, double declination, double distance);
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
    double distance_to(CelestialLocation& other);
};

struct Rotation
{
    Point v;
    double a=0;
    Rotation()
    {
        a = 0;
    };
};

extern double magnbase;

double frand(double lmin, double lmax);
Point compute_normal(Point& pt1, Point& pt2, Point& pt3);
double find_angle(double dx, double dy);
double find_angle_along_vector(Point& pt1, Point& pt2, Point& source, Point& v);
Rotation align_points_3d(Point& point, Point& align, Point& center);
Point rotate3D(Point& point, Point& source, Point& axis, double theta);

// Takes velocity in m/s and computes the ratio of Δt(moving)/Δt(stationary). The result will always be <= 1.
double compute_time_dilation(double velocity);

std::string ltrim(const std::string &s);
std::string rtrim(const std::string &s);
std::string trim(const std::string &s);

#endif
