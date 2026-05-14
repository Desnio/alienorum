#include <iostream>
#include <string.h>
#include "star.h"

void Star::update_location(double new_epoch)
{
    // How many seconds since star's epoch
    double elapsed = (new_epoch - epoch) * 86400;

    // Estimate RA and Decl using proper motion
    double l_RA = right_ascension + proper_motion_RA * elapsed;
    double l_Decl = declination + proper_motion_decl * elapsed;

    // Estimate distance using radial velocity
    double l_dist = distance + radial_velocity * elapsed;

    // Compute new location
    Point newloc = Point::from_ra_dec(l_RA, l_Decl, l_dist);

    // Set system location
    location.system_center = newloc;
}

void Star::rename_from_Bayer_Flamsteed()
{
    if (!constellation.size()) return;
    if (BayerGrkno < 0 && !FlamsteedNo) return;

    if (!consabbrev.size() || !consgen.size())
    {
        std::cerr << "Must read constellation definitions before setting Bayer-Flamsteed names." << std::endl;
        throw 0xbadc0de;
    }

    // Find gentive of constellation.
    int i, j=-1, n = consabbrev.size();
    for (i=0; i<n; i++)
    {
        if (!strcmp(consabbrev[i].c_str(), constellation.c_str()))
        {
            j = i;
            break;
        }
    }

    if (j<0)
    {
        // Not a valid constellation.
        constellation = "";
        return;
    }

    if (BayerGrkno >= 0)
    {
        name = Greek_letter[BayerGrkno] + std::string(" ") + consgen[j];
    }
    else if (FlamsteedNo)
    {
        name = std::to_string(FlamsteedNo) + std::string(" ") + consgen[j];
    }
}
