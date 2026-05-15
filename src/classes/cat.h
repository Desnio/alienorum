
#ifndef _CatalogReader
#define _CatalogReader

#include <string>
#include <vector>
#include "point.h"
#include "galaxy.h"
#include "star.h"
#include "planet.h"

extern std::vector<std::string> known_catalog_names;
extern std::vector<std::string> consline_a, consline_b;
extern std::vector<int> considx, lnpercons;
extern std::vector<Cartesian2D> conscen;
extern int nconsln;
extern int *consaidx, *consbidx;
extern bool have_Gliese, have_BSC, have_HIP;

class CatalogReader
{
    public:
    std::vector<std::string>find_catalogs(std::string path);
    void download_catalogs();
    int read_Gliese_catalog(CelestialObject** cels, int max);
    int read_BrightStars_catalog(CelestialObject** cels, int max);
    int read_Hipparcos_catalog(CelestialObject** cels, int max);
    int read_starname_dat(CelestialObject** cels);                  // No max because we are not adding stars, only setting names.
    int read_local_planets(CelestialObject** cels, int max);

    protected:
    void read_field_onebased(char* buffer, int start, int end, char* out);
};

#endif
