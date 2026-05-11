
#ifndef _CatalogReader
#define _CatalogReader

#include <string>
#include <vector>
#include "point.h"
#include "galaxy.h"
#include "star.h"
#include "planet.h"

extern std::vector<std::string> known_catalog_names;

class CatalogReader
{
    public:
    std::vector<std::string>find_catalogs(std::string path);
    void download_catalogs();
    int read_Gliese_catalog(CelestialObject** cels, int max);
    int read_BrightStars_catalog(CelestialObject** cels, int max);
    int read_Hipparcos_catalog(CelestialObject** cels, int max);

    protected:
    void read_field_onebased(char* buffer, int start, int end, char* out);
};

#endif
