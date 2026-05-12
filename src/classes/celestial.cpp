
#include <math.h>
#include <iomanip>
#include <sstream>
#include "celestial.h"

double CelestialObject::viewer_magnitude(CelestialLocation seen_from)
{
    double r = seen_from.distance_to(location) / parsec / 10;
    double intrinsic = pow(magnbase, -absolute_magnitude);
    double apparent = intrinsic / (r*r);
    return -log(apparent) / log(magnbase);
}

double CelestialObject::distance_from_magnitudes(double apparent, double absolute)
{
    double flux = pow(magnbase, -apparent);
    double intrinsic = pow(magnbase, -absolute);
    double ratio = intrinsic / flux;
    return parsec * 10 * sqrt(ratio);
}

std::string CelestialObject::RA_as_hms()
{
    double RA = right_ascension * fiftyseven / 15;
    int hours = floor(RA);
    RA = (RA-hours) * 60;
    int minutes = floor(RA);
    float seconds = (RA-minutes) * 60;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << seconds;
    std::string sec = oss.str();

    return std::string(hours<10 ? "0" : "")
        + std::to_string(hours) + std::string(":")
        + std::string(minutes<10 ? "0" : "")
        + std::to_string(minutes) + std::string(":")
        + std::string(seconds<10 ? "0" : "")
        + sec;
}

std::string CelestialObject::Decl_as_degms()
{
    int sign = (declination < 0) ? -1 : 1;
    double decl = fabs(declination * fiftyseven);
    int degrees = floor(decl);
    decl = (decl-degrees) * 60;
    int minutes = floor(decl);
    float seconds = (decl-minutes) * 60;
    return std::string( sign < 0 ? "-" : "+" )
        + std::string(degrees<10 ? "0" : "")
        + std::to_string(degrees) + std::string(":")
        + std::string(minutes<10 ? "0" : "")
        + std::to_string(minutes) + std::string(":")
        + std::string(seconds<10 ? "0" : "")
        + std::to_string((int)seconds);
}

std::string CelestialObject::scaled_distance(CelestialLocation fromwhere)
{
    double r = location.distance_to(fromwhere), dispr = r;
    std::string units = " m";

    if (r >= light_year)
    {
        dispr = r / light_year;
        units = " ly";
    }
    else if (r >= AU)
    {
        dispr = r / AU;
        units = " AU";
    }
    else if (r >= 950)    // TODO: Offer choice of imperial units for < 1 AU.
    {
        dispr = r / 1000;
        units = " km";
    }

    return std::to_string(dispr) + units;
}
