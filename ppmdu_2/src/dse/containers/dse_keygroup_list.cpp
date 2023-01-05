#include <dse/containers/dse_keygroup_list.hpp>

namespace DSE
{
    std::ostream& operator<<(std::ostream& strm, const DSE::KeyGroupList& other)
    {
        strm << "#ID\t\tPoly\t\tPrio\t\tVc.Low\t\tVc.High\t\tunk50 & unk51\n";
        for (const Keygroup& k : other.GetVector())
        {
            strm << k <<"\n";
        }
        return strm;
    }

};