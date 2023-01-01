#ifndef DSE_SOUND_EFFECT_SEQUENCE_HPP
#define DSE_SOUND_EFFECT_SEQUENCE_HPP
#include <dse/dse_common.hpp>
#include <dse/containers/dse_mcrl.hpp>
#include <dse/containers/dse_bnkl.hpp>
#include <dse/containers/dse_music_sequence.hpp>

namespace DSE
{
    /*****************************************************************************************
    *****************************************************************************************/
    /// <summary>
    /// Stores the contents of a SEDL file
    /// </summary>
    class SoundEffectSequences
    {
    public:
        SoundEffectSequences()
        {}

        SoundEffectSequences(std::vector<MusicSequence>&& sequences, DSE::DSE_MetaDataSEDL&& meta)
            :m_meta(meta), m_sequences(sequences)
        {}

        SoundEffectSequences(std::vector<MusicSequence>&& sequences, McrlChunkContents&& mcrl, BnklChunkContents&& bnkl, DSE::DSE_MetaDataSEDL&& meta)
            :m_meta(meta), m_sequences(sequences), m_mcrl(mcrl), m_bnkl(bnkl)
        {}

        inline DSE::DSE_MetaDataSEDL& metadata() { return m_meta; }
        inline const DSE::DSE_MetaDataSEDL& metadata()const { return m_meta; }

        DSE::DSE_MetaDataSEDL       m_meta;
        std::vector<MusicSequence>  m_sequences;
        McrlChunkContents           m_mcrl;
        BnklChunkContents           m_bnkl;
    };
};

#endif //!DSE_SOUND_EFFECT_SEQUENCE_HPP