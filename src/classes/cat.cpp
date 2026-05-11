
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <stdio.h>
#include <math.h>
#include <string.h>
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

void CatalogReader::download_catalogs()
{
    std::string path = "catalogs/urls.dat";
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp)
    {
        std::cerr << "File not found " << path << std::endl;
        throw 0xbadf12e;
    }

    char buffer[1024], *catname = nullptr, *url = nullptr;
    int i, j, l;
    bool frist = true;
    while (fgets(buffer, 1020, fp))
    {
        if (buffer[0] == '#') continue;
        for (i=0; buffer[i] && buffer[i] <= ' '; i++);
        catname = &buffer[i];
        if (!*catname) continue;
        if (catname[0] == '#') continue;
        for (j=i; buffer[j] && buffer[j] > ' '; j++);
        buffer[j] = 0;
        for (j++; buffer[j] && buffer[j] <= ' '; j++);
        url = &buffer[j];
        if (!*url) continue;
        for (l=j; buffer[l] && buffer[l] > ' '; l++);
        buffer[l] = 0;

        // If the destination folder exists, assume we already have the catalog.
        std::string destdir = (std::string)"catalogs/" + (std::string)catname;
        fs::path p = destdir.c_str();
        if (!fs::exists(p))
        {
            // Create the dest folder.
            fs::create_directories(destdir);

            // Download the gzipped tarball.
            std::string destfname = destdir + "/download.tar.gz";
            p = destfname.c_str();
            if (!fs::exists(p))
            {
                // TODO: Add compatibility for Windows and Mac.
                if (frist) std::cout << "Downloading catalogs..." << std::endl;
                std::string cmd = (std::string)"wget -O " + destfname + (std::string)" " + (std::string)url;
                std::cout << cmd << std::endl;
                std::system(cmd.c_str());
                frist = false;
            }

            // Extract the tarball.
            std::string cmd = (std::string)"tar -xvzf " + destfname + (std::string)" -C " + destdir;
            std::cout << cmd << std::endl;
            std::system(cmd.c_str());

            // Delete the tarball.
            fs::remove(destfname);

            // Any .gz files in the destination folder, unzip them.
            for (const auto& entry : fs::directory_iterator(destdir))
            {
                std::string entry_name = entry.path().filename();
                i = entry_name.size();
                j = i - 3;
                if (!strcmp(".gz", &entry_name.c_str()[j]))
                {
                    cmd = (std::string)"gunzip " + destdir + (std::string)"/" + entry_name;
                    std::cout << cmd << std::endl;
                    std::system(cmd.c_str());
                }
            }
        }
    }
}

int CatalogReader::read_BrightStars_catalog(CelestialObject **cels, int max)
{
    std::string path = "catalogs/BSC/catalog";
    char buffer[65536];
    char field[32];
    int num_read = 0;
    int offset;
    double deg, mnt, sec;

    for (offset=0; offset<max && cels[offset]; offset++);
    if (offset >= max) return 0;

    FILE* fp = fopen(path.c_str(), "rb");

    while (fgets(buffer, 65520, fp))
    {
        Star* s = new Star();
        s->type = star;

        //    Bytes Format  Units   Label    Explanations
        // --------------------------------------------------------------------------------
        //    1-  4  I4     ---     HR       [1/9110]+ Harvard Revised Number
        read_field_onebased(buffer, 1, 4, field);
        s->HR = atoi(field);

        //    5- 14  A10    ---     Name     Name, generally Bayer and/or Flamsteed name
        read_field_onebased(buffer, 5, 14, field);
        s->name = trim(field);

        //   26- 31  I6     ---     HD       [1/225300]? Henry Draper Catalog Number
        read_field_onebased(buffer, 26, 31, field);
        s->HD = atoi(field);

        //   32- 37  I6     ---     SAO      [1/258997]? SAO Catalog Number
        read_field_onebased(buffer, 32, 37, field);
        s->SAO = atoi(field);

        //   76- 77  I2     h       RAh      ?Hours RA, equinox J2000, epoch 2000.0 (1)
        read_field_onebased(buffer, 76, 77, field);
        deg = atof(field) * 15;

        //   78- 79  I2     min     RAm      ?Minutes RA, equinox J2000, epoch 2000.0 (1)
        read_field_onebased(buffer, 78, 79, field);
        mnt = atof(field) * 15;

        //   80- 83  F4.1   s       RAs      ?Seconds RA, equinox J2000, epoch 2000.0 (1)
        read_field_onebased(buffer, 80, 83, field);
        sec = atof(field) * 15;

        s->right_ascension = (deg + mnt/60 + sec/3600) * fiftyseventh;

        //       84  A1     ---     DE-      ?Sign Dec, equinox J2000, epoch 2000.0 (1)
        //   85- 86  I2     deg     DEd      ?Degrees Dec, equinox J2000, epoch 2000.0 (1)
        read_field_onebased(buffer, 84, 86, field);
        if (field[0] == '+') field[0] = '0';
        deg = atof(field);

        //   87- 88  I2     arcmin  DEm      ?Minutes Dec, equinox J2000, epoch 2000.0 (1)
        read_field_onebased(buffer, 87, 88, field);
        mnt = atof(field);

        //   89- 90  I2     arcsec  DEs      ?Seconds Dec, equinox J2000, epoch 2000.0 (1)
        read_field_onebased(buffer, 89, 90, field);
        sec = atof(field);

        s->declination = (deg + mnt/60 + sec/3600) * fiftyseventh;
        s->epoch = 2451544.5;

        //  103-107  F5.2   mag     Vmag     ?Visual magnitude (1)
        read_field_onebased(buffer, 103, 107, field);
        s->apparent_magnitude = atof(field);

        //  110-114  F5.2   mag     B-V      ? B-V color in the UBV system
        read_field_onebased(buffer, 110, 114, field);
        s->BV_magnitude = atof(field);

        //  116-120  F5.2   mag     U-B      ? U-B color in the UBV system
        read_field_onebased(buffer, 116, 120, field);
        s->UB_magnitude = atof(field);

        //  122-126  F5.2   mag     R-I      ? R-I   in system specified by n_R-I
        read_field_onebased(buffer, 122, 126, field);
        s->RI_magnitude = atof(field);

        //  128-147  A20    ---     SpType   Spectral type
        read_field_onebased(buffer, 128, 147, field);
        s->spectral_type = trim(field);

        //  149-154  F6.3 arcsec/yr pmRA    *?Annual proper motion in RA J2000, FK5 system
        read_field_onebased(buffer, 149, 154, field);
        s->proper_motion_RA = atof(field) * fiftyseventh / 3600 / year;

        //  155-160  F6.3 arcsec/yr pmDE     ?Annual proper motion in Dec J2000, FK5 system
        read_field_onebased(buffer, 155, 160, field);
        s->proper_motion_decl = atof(field) * fiftyseventh / 3600 / year;

        //  162-166  F5.3   arcsec  Parallax ? Trigonometric parallax (unless n_Parallax)
        read_field_onebased(buffer, 162, 166, field);
        s->parallax = atof(field);
        if (!strcmp(s->name.c_str(), "Sun"))
            s->distance = 0;
        else
            s->distance = s->parallax ? (parsec / s->parallax) : light_year*1e4;
        s->parallax /= fiftyseven * 3600;

        //  167-170  I4     km/s    RadVel   ? Heliocentric Radial Velocity
        read_field_onebased(buffer, 167, 170, field);
        s->radial_velocity = atof(field) * 1000;

        // Estimate some more parameters based on the data.
        s->VR_magnitude = (s->RI_magnitude + s->BV_magnitude*2) / 3;      // VERY rough estimate
        double intrinsic_brightness = pow(magnbase, -s->apparent_magnitude) * pow(fmax(AU, s->distance) / parsec / 10, 2);
        s->absolute_magnitude = -log(intrinsic_brightness) / log(magnbase);

        #if 0
        // stellar radius = sqrt( lum(sun) * 10^(0.4 * mag(sun)-mag) / 4 pi sigma T^4 )
        // better also apply https://en.wikipedia.org/wiki/Bolometric_correction
        s->volumetric_mean_radius = solar_radius * 

        s->mass = ?????
        #endif

        s->update_location(2451544.5);

        cels[offset+num_read] = s;
        num_read++;
        if ((offset+num_read) >= (max-1)) break;
    }
    fclose(fp);
    return num_read;
}

void CatalogReader::read_field_onebased(char *buffer, int start, int end, char *out)
{
    // Non-programmers, smh.
    start--;
    int len = end - start;
    int i;
    for (i=0; i<len; i++) out[i] = buffer[i+start];
    out[i] = 0;
}
