
#ifndef _AlienorumMisc
#define _AlienorumMisc

#include <string>
#include <vector>
#include <ctime>

#define light_year 9460730472580800
#define parsec 3.08567758128E+16
#define AU 149597870700.0
#define day 86400
#define year (365 * 86400 + 5 * 3600 + 48 * 60 + 45)
#define J2000 2451544.5
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
#define default_brightness 13.0
#define default_gamma 1.0
#define target_frame_rate 30
const std::time_t J2000_TIME_T = 946684800;

extern double magnbase;
extern std::string Greek_letter[24];
extern __uint32_t xonsm[13];
extern const std::string WHITESPACE;
extern std::vector<std::string> consname, consabbrev, consgen;

std::string ltrim(const std::string &s);
std::string rtrim(const std::string &s);
std::string trim(const std::string &s);
double frand(double lmin, double lmax);
int Grkno_from_abbrev(char* abbrev);
std::string Greek_from_abbrev(char* abbrev);
std::string Greek_from_abbrev(std::string abbrev);
int Damerau_Levenshtein(const std::string &s1, const std::string &s2);

// Takes velocity in m/s and computes the ratio of Δt(moving)/Δt(stationary). The result will always be <= 1.
double compute_time_dilation(double velocity);

#endif
