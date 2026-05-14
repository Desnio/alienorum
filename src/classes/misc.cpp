#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <math.h>

#include "misc.h"

const std::string WHITESPACE = " \n\r\t\f\v";
__uint32_t xonsm[13] = {0x0e432843, 0x0e4328ec, 0x25443485, 0x29cc28ec, 0x29cc513a, 0x43363485, 0x511e0000, 0x511e3485, 0x511e513a, 0x511e5147, 0x511eab3a, 0x2b85e980, 0x57e47000};
double magnbase = pow(100, 1.0/5);
std::vector<std::string> consname, consabbrev, consgen;

std::string Greek_letter[24] =
{
    "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta",
    "Eta", "Theta", "Iota", "Kappa", "Lambda", "Mu",
    "Nu", "Xi", "Omicron", "Pi", "Rho", "Sigma",
    "Tau", "Upsilon", "Phi", "Chi", "Psi", "Omega"
};

double frand(double lmin, double lmax)
{
    int r = rand();
    double f = (double)r / RAND_MAX;
    f *= (lmax-lmin);
    return f+lmin;
}

int Grkno_from_abbrev(char *abbrev)
{
    int i;
    for (i=0; i<24; i++)
    {
        if (Greek_letter[i][0] == abbrev[0]
            && Greek_letter[i][1] == abbrev[1]
            && Greek_letter[i][2] == abbrev[2]
            )
            return i;
    }
    return -1;
}

std::string Greek_from_abbrev(char *abbrev)
{
    int i;
    for (i=0; i<24; i++)
    {
        if (Greek_letter[i][0] == abbrev[0]
            && Greek_letter[i][1] == abbrev[1]
            && Greek_letter[i][2] == abbrev[2]
            )
            return Greek_letter[i];
    }
    return std::string("");
}

std::string Greek_from_abbrev(std::string abbrev)
{
    return Greek_from_abbrev(abbrev.c_str());
}

double compute_time_dilation(double velocity)
{
    return sqrt(1.0 - (velocity*velocity)/(speed_of_light*speed_of_light));
}

// Trim from start (left)
std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

// Trim from end (right)
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

// Trim from both ends
std::string trim(const std::string &s)
{
    return rtrim(ltrim(s));
}

int Damerau_Levenshtein(const std::string& s1, const std::string& s2)
{
    const std::size_t m = s1.size();
    const std::size_t n = s2.size();

    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));

    for (int i = 0; i <= m; ++i) dp[i][0] = i;
    for (int j = 0; j <= n; ++j) dp[0][j] = j;

    for (int i = 1; i <= m; ++i)
    {
        for (int j = 1; j <= n; ++j)
        {
            char c1 = s1[i - 1];
            char c2 = s2[j - 1];
            if (c1 >= 'A' && c1 <= 'Z') c1 += 0x20;
            if (c2 >= 'A' && c2 <= 'Z') c2 += 0x20;
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;

            dp[i][j] = std::min(
            {
                dp[i - 1][j] + 1,       // Deletion
                dp[i][j - 1] + 1,       // Insertion
                dp[i - 1][j - 1] + cost // Substitution
            });

            // Add the transposition check (Damerau modification)
            if (i > 1 && j > 1 && s1[i - 1] == s2[j - 2] && s1[i - 2] == s2[j - 1])
            {
                dp[i][j] = std::min(dp[i][j], dp[i - 2][j - 2] + cost); // Transposition
            }
        }
    }
    return dp[m][n];
}