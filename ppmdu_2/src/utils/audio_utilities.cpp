#include <utils/audio_utilities.hpp>

namespace utils
{
    std::vector<int16_t> RawBytesToPCM16Vec(const std::vector<uint8_t>* praw)
    {
        return utils::RawVector2PCM16Parser<int16_t>(*praw);
    }

    std::vector<uint8_t> PCM16VecToRawBytes(const std::vector<int16_t>* vec)
    {
        std::vector<uint8_t> result;
        result.reserve(vec->size() * 2);
        for (uint16_t smpl : *vec)
        {
            uint8_t hi = static_cast<uint8_t>((smpl >> 8) & 0xFF);
            uint8_t low = static_cast<uint8_t>(smpl & 0xFF);
            result.push_back(low);
            result.push_back(hi);
        }
        return result;
    }

    std::vector<int16_t> PCM8RawBytesToPCM16Vec(const std::vector<uint8_t>* praw)
    {
        std::vector<int16_t> out;
        auto                 itread = praw->begin();
        auto                 itend = praw->end();
        out.reserve(praw->size() * 2);

        for (; itread != itend; ++itread)
            out.push_back(((static_cast<int16_t>(*itread) ^ 0x80) * std::numeric_limits<int16_t>::max()) / std::numeric_limits<uint8_t>::max()); //Convert from excess-k coding to 2's complement. And scale up to pcm16.

        return out;
    }

};