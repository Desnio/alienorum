
#ifndef _CatalogReader
#define _CatalogReader

#include <string>
#include <vector>
#include "celestial.h"

extern std::vector<std::string> known_catalog_names;

class CatalogReader
{
    public:
    std::vector<std::string>find_catalogs(std::string path);
    int read_Gliese_catalog(CelestialObject* cels, int max);
    int read_BrightStars_catalog(CelestialObject* cels, int max);
    int read_Hipparcos_catalog(CelestialObject* cels, int max);
};

#endif
