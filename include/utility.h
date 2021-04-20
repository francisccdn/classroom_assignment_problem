#include <vector>
#include <string>

#ifndef FRANCISCCDN_UTILITY_H
#define FRANCISCCDN_UTILITY_H
namespace francisccdn
{
    std::vector<std::string> splice(std::string s, char c);
    int get_largest_natural(std::vector<int> vec);
}
#endif