#ifndef DSE_TO_SF2_HPP
#define DSE_TO_SF2_HPP
/*
* Utilities for converting dse audio to soundfonts
*/
#include <ext_fmts/sf2.hpp>
#include <dse/dse_common.hpp>
#include <dse/dse_containers.hpp>
#include <dse/dse_conversion.hpp>
#include <dse/dse_conversion_info.hpp>

namespace DSE
{
    //
    //constants
    //
    extern const uint16_t DSE_InfiniteAttenuation_cB; //Attenuation value for silence in centibell
    extern const uint16_t DSE_LowestAttenuation_cB; //This essentially defines the dynamic range of the tracks. It sets how low the most quiet sound can go. Which is around -20 dB
    extern const uint16_t DSE_SustainFactor_msec; //An extra time value to add to the decay when decay1 + decay2 are combined, and the sustain is non-zero
    extern const double BytePanToSoundfontPanMulti; //A multiplier to use to convert from DSE Pan to Soundfont Pan

    extern const utils::value_limits<int8_t> DSE_LimitsPan;
    extern const utils::value_limits<int8_t> DSE_LimitsVol;

    //
    //Helpers
    //

    /// <summary>
    /// Convert a pan value from a DSE file(0 to 127) to a SF2 pan( -500 to 500 ).
    /// </summary>
    /// <param name="dsepan"></param>
    /// <returns></returns>
    int16_t DSE_PanToSf2Pan(int8_t dsepan);

    //!#FIXME: MOST LIKELY INNACURATE !
    uint16_t DSE_VolToSf2Attenuation(int8_t dsevol);

    int16_t DSE_LFOFrequencyToCents(int16_t freqhz);
    int16_t DSE_LFODepthToCents(int16_t depth);

    /*********************************************************************************
        ScaleEnvelopeDuration
            Obtain from the duration table the duration of each envelope params,
            and scale them.
            Scale the various parts of the volume envelope by the specified factors.
    *********************************************************************************/
    DSEEnvelope IntepretAndScaleEnvelopeDuration(const DSEEnvelope& srcenv,
        double                ovrllscale = 1.0,
        double                atkf = 1.0,
        double                holdf = 1.0,
        double                decayf = 1.0,
        double                releasef = 1.0);

    DSEEnvelope IntepretEnvelopeDuration(const DSEEnvelope& srcenv);

    /*
        Convert the parameters of a DSE envelope to SF2
    */
    sf2::Envelope RemapDSEVolEnvToSF2(DSEEnvelope origenv);


    /***************************************************************************************
        AddSampleToSoundfont
    ***************************************************************************************/
    [[deprecated]]
    void AddSampleToSoundfont(sampleid_t cntsmslot, const DSE::PresetDatabase& presdb, bankid_t bankid, std::map<uint16_t, size_t>& swdsmplofftosf, sf2::SoundFont& sf);


    ///////////////////////////////////////////
    void PrepareSFPresetForDSEPreset(const DSE::ProgramInfo& dsePres, sf2::Preset& pre, SMDLPresetConversionInfo::PresetConvData& convinf, bool lfoeffectson);



    ///

    /// <summary>
    /// Helper for inserting and converting dse presets into a sounfont.
    /// Will keep track of all the relevant info, and greatly simplify the arg lists on the functions to build the things.
    /// </summary>
    struct DSE_SoundFontBuilder
    {
        sf2::SoundFont                _sf; //Soundfont currently being assembled
        const DSE::PresetDatabase&    _db;
        bool                          _lfoFxDisabled;
        bool                          _bakeSamples;
        std::map<sampleid_t, size_t>  _smplIdToSf2SmplId;
        DSE::SMDLPresetConversionInfo _convertionInfo;
        DSE::ProcessedPresets         _bakedPresets;
        std::map<sampleid_t, std::pair<DSESampleConvertionInfo, std::vector<int16_t>>> _processed_samples_cache; //Cache of samples already converted

        std::string _curPairName;    //The name of the swd + smdl/sedl set we're currently processing
        //sampleid_t _nbSampleSlots;     //The nb of sample slots for the preset bank currently being processed
        size_t     _cntExportedPresets;
        size_t     _cntExportedSplits;

        DSE_SoundFontBuilder(const DSE::PresetDatabase & db, bool blfoFxDisabled = false, bool bbakeSamples = false);
        void Reset();

        sf2::SoundFont operator()(const DSE::PresetBank& bnk, const std::string& sfname);
        std::set<sampleid_t> ListUsedSamples(std::shared_ptr<ProgramBank> prgbank);
        void ProcessSamples(std::set<sampleid_t> usedSamples);
        void ProcessPrograms(std::shared_ptr<DSE::ProgramBank> prgbank);
        sf2::Instrument ProcessSplits(const DSE::ProgramBank::ptrprg_t& prgm, SMDLPresetConversionInfo::PresetConvData& convinf, std::shared_ptr<ProgramBank> prgbank);
    };
};

#endif //DSE_TO_SF2_HPP