#include <vector>
#include <string>
#include "../include/utility.h"

namespace francisccdn
{
    std::vector<std::string> splice(std::string s, char c)
    {
        std::string str = s;
        std::vector<std::string> spliced_string = std::vector<std::string>();

        size_t pos = str.find(c);

        while (pos != std::string::npos)
        {
            spliced_string.push_back(str.substr(0, pos));
            str = str.substr(pos + 1);
            pos = str.find(c);
        }

        spliced_string.push_back(str);
        
        return spliced_string;
    }

    int get_largest_natural(std::vector<int> vec)
    {
        int max = 0;
        for (int i = vec.size() - 1; i >= 0; i--)
        {
            if (vec[i] >= max)
            {
                max = vec[i];
            }
        }
        return max;
    }
}