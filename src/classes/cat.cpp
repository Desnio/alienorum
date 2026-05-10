
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include "cat.h"

namespace fs = std::filesystem;

std::vector<std::string> known_catalog_names =
{
    "Gliese", "GJ", "Gliese-Jahreiss",
    "HD", "HenryDraper",
    "Hipparcos",
    "USNO", "SAO",
    "BSC", "BrightStarCatalog", "BrightStarCatalogue",
    "2MASS",
    "REGALADE",
    "GALEX",
    "astorb",
    "comets"
    // TODO: Add hundreds more...
};

std::vector<std::string> CatalogReader::find_catalogs(std::string path)
{
    std::vector<std::string> results;
    try
    {
        for (const auto& entry : fs::directory_iterator(path))
        {
            std::string entry_name = entry.path().filename();
            if (fs::is_directory(entry.path())
                &&
                std::find(known_catalog_names.begin(), known_catalog_names.end(), entry_name) != known_catalog_names.end()
                )
            {
                results.push_back(path + "/" + entry_name);
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }

    return results;
}