#include <dse/dse_to_sf2.hpp>
#include <dse/dse_conversion.hpp>

#include <ext_fmts/adpcm.hpp>

#include <utils/audio_utilities.hpp>

#include <string>
#include <sstream>
#include <map>
#include <memory>

using namespace std;

namespace DSE
{
    const uint16_t DSE_InfiniteAttenuation_cB = 1440;//1440; //sf2::SF_GenLimitsInitAttenuation.max_;
    const uint16_t DSE_LowestAttenuation_cB = 250;//200; //This essentially defines the dynamic range of the tracks. It sets how low the most quiet sound can go. Which is around -20 dB
    const uint16_t DSE_SustainFactor_msec = 3000; //An extra time value to add to the decay when decay1 + decay2 are combined, and the sustain is non-zero

    //A multiplier to use to convert from DSE Pan to Soundfont Pan
    const double BytePanToSoundfontPanMulti = static_cast<double>(sf2::SF_GenLimitsPan.max_ + (-sf2::SF_GenLimitsPan.min_)) / static_cast<double>(DSE_LimitsPan.max_);

    const utils::value_limits<int8_t> DSE_LimitsPan{ 0,  64, 127, 64 };
    const utils::value_limits<int8_t> DSE_LimitsVol{ 0, 127, 127, 64 };

    /*
        DsePanToSf2Pan
            Convert a pan value from a DSE file(0 to 127) to a SF2 pan( -500 to 500 ).
    */
    int16_t DSE_PanToSf2Pan(int8_t dsepan)
    {
        dsepan = abs(dsepan);

        if (dsepan == DSE_LimitsPan.max_)
            return sf2::SF_GenLimitsPan.max_;
        else if (dsepan == DSE_LimitsPan.min_)
            return sf2::SF_GenLimitsPan.min_;
        else
        {
            return static_cast<int16_t>(lround((dsepan - DSE_LimitsPan.mid_) * BytePanToSoundfontPanMulti));
        }
    }

    //!#FIXME: MOST LIKELY INNACURATE !
    uint16_t DSE_VolToSf2Attenuation(int8_t dsevol)
    {
        dsevol = abs(dsevol);

        //Because of the rounding, we need to make sure our limits match the SF limits
        if (dsevol == DSE_LimitsVol.min_)
            return DSE_InfiniteAttenuation_cB;
        else
        {
            return DSE_LowestAttenuation_cB - (dsevol * DSE_LowestAttenuation_cB) / 127; //The NDS's volume curve is linear, but soundfonts use a logarithmic volume curve.. >_<
        }
    }

    int16_t DSE_LFOFrequencyToCents(int16_t freqhz)
    {
        const double ReferenceFreq = 8.176; // 0 cents is 8.176 Hz
        return static_cast<int16_t>(lround(1200.00 * log2(static_cast<double>(freqhz) / ReferenceFreq)));
    }

    int16_t DSE_LFODepthToCents(int16_t depth)
    {
        return (depth / 12) * -1; /*static_cast<int16_t>( lround(depth * 12000.0 / 10000.0 ) )*/;

        //return static_cast<int16_t>( lround( 1200.00 * log2( static_cast<double>(depth)  ) ) );
    }

    /*********************************************************************************
        ScaleEnvelopeDuration
            Obtain from the duration table the duration of each envelope params,
            and scale them.
            Scale the various parts of the volume envelope by the specified factors.
    *********************************************************************************/
    inline DSEEnvelope IntepretAndScaleEnvelopeDuration(const DSEEnvelope& srcenv,
        double                ovrllscale,
        double                atkf,
        double                holdf,
        double                decayf,
        double                releasef)
    {
        DSEEnvelope outenv(srcenv);
        outenv.attack = static_cast<DSEEnvelope::timeprop_t>(lround(DSEEnveloppeDurationToMSec(static_cast<int8_t>(outenv.attack), outenv.envmulti) * atkf * ovrllscale));
        outenv.hold = static_cast<DSEEnvelope::timeprop_t>(lround(DSEEnveloppeDurationToMSec(static_cast<int8_t>(outenv.hold), outenv.envmulti) * holdf * ovrllscale));
        outenv.decay = static_cast<DSEEnvelope::timeprop_t>(lround(DSEEnveloppeDurationToMSec(static_cast<int8_t>(outenv.decay), outenv.envmulti) * decayf * ovrllscale));
        outenv.release = static_cast<DSEEnvelope::timeprop_t>(lround(DSEEnveloppeDurationToMSec(static_cast<int8_t>(outenv.release), outenv.envmulti) * releasef * ovrllscale));
        return outenv;
    }

    inline DSEEnvelope IntepretEnvelopeDuration(const DSEEnvelope& srcenv)
    {
        DSEEnvelope outenv(srcenv);
        outenv.attack = static_cast<DSEEnvelope::timeprop_t>(DSEEnveloppeDurationToMSec(static_cast<int8_t>(outenv.attack), outenv.envmulti));
        outenv.hold = static_cast<DSEEnvelope::timeprop_t>(DSEEnveloppeDurationToMSec(static_cast<int8_t>(outenv.hold), outenv.envmulti));
        outenv.decay = static_cast<DSEEnvelope::timeprop_t>(DSEEnveloppeDurationToMSec(static_cast<int8_t>(outenv.decay), outenv.envmulti));
        outenv.release = static_cast<DSEEnvelope::timeprop_t>(DSEEnveloppeDurationToMSec(static_cast<int8_t>(outenv.release), outenv.envmulti));
        return outenv;
    }

    /*
        Convert the parameters of a DSE envelope to SF2
    */
    sf2::Envelope RemapDSEVolEnvToSF2(DSEEnvelope origenv)
    {
        sf2::Envelope volenv;

        //#TODO: implement scaling by users !!!
        //DSEEnvelope interpenv = IntepretEnvelopeDuration(origenv);
        DSEEnvelope interpenv = IntepretAndScaleEnvelopeDuration(origenv, 1.0, 1.0, 1.0, 1.0, 1.0);

        if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
        {
            clog << "\tRemaping DSE( "
                << "atkvol : " << static_cast<short>(origenv.atkvol) << ", "
                << "atk    : " << static_cast<short>(origenv.attack) << ", "
                << "dec    : " << static_cast<short>(origenv.decay) << ", "
                << "sus    : " << static_cast<short>(origenv.sustain) << ", "
                << "hold   : " << static_cast<short>(origenv.hold) << ", "
                << "dec2   : " << static_cast<short>(origenv.decay2) << ", "
                << "rel    : " << static_cast<short>(origenv.release) << " )\n";
        }

        //Handle Attack
        if (origenv.attack != 0)
        {
            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "Handling Attack..\n";

            volenv.attack = sf2::MSecsToTimecents(interpenv.attack);
        }
        else if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
            clog << "Skipping Attack..\n";

        //Handle Hold
        if (origenv.hold != 0)
        {
            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "Handling Hold..\n";

            volenv.hold = sf2::MSecsToTimecents(interpenv.hold);
        }
        else if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
            clog << "Skipping Hold..\n";

        //Handle Sustain
        volenv.sustain = DSE_VolToSf2Attenuation(interpenv.sustain);

        //uint16_t sustainfactor = (interpenv.sustain * DSE_SustainFactor_msec) /127; //Add a bit more time, because soundfont handles volume exponentially, and thus, the volume sinks to 0 really quickly!


        //Handle Decay
        if (origenv.decay != 0 && origenv.decay2 != 0 && origenv.decay2 != 0x7F)
        {
            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "We got combined decays! decay1-2 : "
                << static_cast<unsigned short>(interpenv.decay) << " + "
                << static_cast<unsigned short>(interpenv.decay2) << " = "
                << static_cast<unsigned short>(interpenv.decay + interpenv.decay2) << " !\n";

            //If decay is set to infinite, we just ignore it!
            if (origenv.decay == 0x7F)
                volenv.decay = sf2::MSecsToTimecentsDecay(interpenv.decay2) /*+ sustainfactor*/;
            else if (origenv.sustain == 0) //The sustain check is to avoid the case where the first decay phase already should have brought the volume to 0 before the decay2 phase would do anything. 
                volenv.decay = sf2::MSecsToTimecentsDecay(interpenv.decay) /*+ sustainfactor*/;
            else
            {
                volenv.decay = sf2::MSecsToTimecentsDecay(interpenv.decay + interpenv.decay2) /*+ sustainfactor*/; //Add an extra factor based on the sustain value
            }
            volenv.sustain = DSE_InfiniteAttenuation_cB;
        }
        else if (origenv.decay != 0)
        {
            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "Handling single decay..\n";

            //We use Decay
            //if( decay != 0x7F )
            volenv.decay = sf2::MSecsToTimecentsDecay(interpenv.decay);
            //else
            //    volenv.sustain = 0; //No decay at all
        }
        else
        {
            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "Decay was 0, falling back to decay 2..\n";

            if (origenv.decay2 != 0x7F)
            {
                //We want to fake the volume going down until complete silence, while the key is still held down 
                volenv.decay = sf2::MSecsToTimecentsDecay(interpenv.decay2);
                volenv.sustain = DSE_InfiniteAttenuation_cB;
            }
            else
                volenv.sustain = 0; //No decay at all
        }

        //Handle Release
        if (origenv.release != 0)
        {
            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "Handling Release..\n";

            //if( rel != 0x7F )
            volenv.release = sf2::MSecsToTimecentsDecay(interpenv.release);
            //else
            //    volenv.release = SHRT_MAX; //Infinite
        }
        else if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
            clog << "Skipping Release..\n";

        if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
        {
            clog << "\tRemaped to (del, atk, hold, dec, sus, rel) ( "
                << static_cast<short>(volenv.delay) << ", "
                << static_cast<short>(volenv.attack) << ", "
                << static_cast<short>(volenv.hold) << ", "
                << static_cast<short>(volenv.decay) << ", "
                << static_cast<short>(volenv.sustain) << ", "
                << static_cast<short>(volenv.release) << " )" << endl;

            //Diagnostic            
            if (origenv.attack > 0 && origenv.attack < 0x7F &&
                (volenv.attack == sf2::SF_GenLimitsVolEnvAttack.max_ || volenv.attack == sf2::SF_GenLimitsVolEnvAttack.min_))
            {
                //Something fishy is going on !
                clog << "\tThe attack value " << static_cast<short>(origenv.attack) << " shouldn't result in the SF2 value " << volenv.attack << " !\n";
                assert(false);
            }

            if (origenv.hold > 0 && origenv.hold < 0x7F &&
                (volenv.hold == sf2::SF_GenLimitsVolEnvHold.max_ || volenv.hold == sf2::SF_GenLimitsVolEnvHold.min_))
            {
                //Something fishy is going on !
                clog << "\tThe hold value " << static_cast<short>(origenv.hold) << " shouldn't result in the SF2 value " << volenv.hold << " !\n";
                assert(false);
            }

            if (origenv.decay > 0 && origenv.decay < 0x7F &&
                (volenv.decay == sf2::SF_GenLimitsVolEnvDecay.max_ || volenv.decay == sf2::SF_GenLimitsVolEnvDecay.min_))
            {
                //Something fishy is going on !
                clog << "\tThe decay value " << static_cast<short>(origenv.decay) << " shouldn't result in the SF2 value " << volenv.decay << " !\n";
                assert(false);
            }

            //if( sustain > 0 && sustain < 0x7F && 
            //   (volenv.sustain == sf2::SF_GenLimitsVolEnvSustain.max_ || volenv.sustain == sf2::SF_GenLimitsVolEnvSustain.min_) )
            //{
            //    //Something fishy is going on !
            //    clog << "\tThe sustain value " <<static_cast<short>(sustain) <<" shouldn't result in the SF2 value " <<volenv.sustain  <<" !\n";
            //    assert(false);
            //}

            if (origenv.release > 0 && origenv.release < 0x7F &&
                (volenv.release == sf2::SF_GenLimitsVolEnvRelease.max_ || volenv.release == sf2::SF_GenLimitsVolEnvRelease.min_))
            {
                //Something fishy is going on !
                clog << "\tThe release value " << static_cast<short>(origenv.release) << " shouldn't result in the SF2 value " << volenv.release << " !\n";
                assert(false);
            }
        }

        return volenv;
    }

    /***************************************************************************************
        AddSampleToSoundfont
    ***************************************************************************************/


    [[deprecated]]
    void AddSampleToSoundfont(sampleid_t cntsmslot, std::shared_ptr<DSE::SampleBank>& samples, std::map<uint16_t, size_t>& swdsmplofftosf, sf2::SoundFont& sf)
    {
        using namespace sf2;
        using namespace ::audio;

        if (samples->IsInfoPresent(cntsmslot) && samples->IsDataPresent(cntsmslot))
        {
            const auto& cursminf = *(samples->sampleInfo(cntsmslot));
            Sample::loadfun_t   loadfun;
            Sample::smplcount_t smpllen = 0;
            Sample::smplcount_t loopbeg = 0;
            Sample::smplcount_t loopend = 0;

            if (cursminf.smplfmt == eDSESmplFmt::ima_adpcm4)
            {
                loadfun = std::bind(::audio::DecodeADPCM_NDS, std::ref(*samples->sample(cntsmslot)), 1);
                smpllen = ::audio::ADPCMSzToPCM16Sz(samples->sample(cntsmslot)->size());
                loopbeg = (cursminf.loopbeg - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
                loopend = smpllen;
            }
            else if (cursminf.smplfmt == eDSESmplFmt::pcm16)
            {
                loadfun = std::bind(&utils::RawBytesToPCM16Vec, samples->sample(cntsmslot));
                smpllen = samples->sample(cntsmslot)->size() / 2;
                loopbeg = cursminf.loopbeg * 2; //loopbeg is counted in int32, so multiply by 2 to get the loop beg as pcm16
                loopend = smpllen;
            }
            else if (cursminf.smplfmt == eDSESmplFmt::pcm8)
            {
                loadfun = std::bind(&utils::PCM8RawBytesToPCM16Vec, samples->sample(cntsmslot));
                smpllen = samples->sample(cntsmslot)->size() * 2; //PCM8 -> PCM16
                loopbeg = cursminf.loopbeg * 4; //loopbeg is counted in int32, for PCM8 data, so multiply by 4 to get the loop beg as pcm16
                loopend = smpllen;
            }
            else if (cursminf.smplfmt == eDSESmplFmt::ima_adpcm3)
            {
                //stringstream sstrerr;
                //sstrerr << "PSG instruments unsuported!";
                //throw std::runtime_error( sstrerr.str() );
                clog << "<!>- Warning: AddSampleToSoundfont(): Unsuported type 3 sample added!\n";
            }
            else
            {
                stringstream sstrerr;
                sstrerr << "Unknown sample format (0x" << hex << uppercase << static_cast<uint16_t>(cursminf.smplfmt) << nouppercase << dec << ") encountered !";
                throw std::runtime_error(sstrerr.str());
            }

#ifdef _DEBUG
            assert((loopbeg < smpllen || loopend >= smpllen));
#endif

            Sample sm(std::move(loadfun), smpllen);
            stringstream sstrname;
            sstrname << "smpl_0x" << uppercase << hex << cntsmslot;
            //sm.SetName( "smpl#" + to_string(cntsmslot) );
            sm.SetName(sstrname.str());
            sm.SetSampleRate(cursminf.smplrate);

            //#ifdef _DEBUG
                        //double pcorrect  = 1.0 / 100.0 / 2.5 / lround( static_cast<double>(cursminf.pitchoffst) );
                        //double remainder = abs(pcorrect) - 127;
                        //sm.SetPitchCorrection( static_cast<int8_t>(lround(pcorrect)) ); //Pitch correct is 1/250th of a semitone, while the SF2 pitch correction is 1/100th of a semitone

                        //if( remainder > 0)
                        //    cout <<"########## Sample pitch correct remainder !!!! ####### " <<showbase <<showpoint << remainder <<noshowbase <<noshowpoint <<"\n";
            //#endif

            sm.SetSampleType(Sample::eSmplTy::monoSample); //#TODO: Mono samples only for now !

            if (cursminf.smplloop != 0)
                sm.SetLoopBounds(loopbeg, loopend);

            swdsmplofftosf.emplace(cntsmslot, sf.AddSample(std::move(sm)));
        }
    }

    void PrepareSFPresetForDSEPreset(const DSE::ProgramInfo& dsePres, sf2::Preset& pre, SMDLPresetConversionInfo::PresetConvData& convinf, bool lfoeffectson)
    {
        using namespace sf2;
        ZoneBag global;

        //#1 - Setup Generators
        if (dsePres.prgvol != DSE_LimitsVol.def_)
            global.SetInitAtt(DSE_VolToSf2Attenuation(dsePres.prgvol));

        // Range of DSE Pan : 0x00 - 0x40 - 0x7F
        if (dsePres.prgpan != DSE_LimitsPan.def_)
            global.SetPan(DSE_PanToSf2Pan(dsePres.prgpan));

        //#2 - Setup LFOs
        unsigned int cntlfo = 0;
        for (const auto& lfo : dsePres.m_lfotbl)
        {
            //!#TODO: Put the content of the loop in its own function. This also seem like duplicate code!! (HandleBakedPrgInst)
            if (lfo.unk52 != 0 && lfoeffectson) //Is the LFO enabled ?
            {
                if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                    clog << "\tLFO" << cntlfo << " : Target: ";

                if (lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Pitch)) //Pitch
                {
                    //The effect on the pitch can be handled this way
                    global.AddGenerator(eSFGen::freqVibLFO, DSE_LFOFrequencyToCents(lfo.depth/*lfo.rate*//*/50*/)); //Frequency
                    global.AddGenerator(eSFGen::vibLfoToPitch, lfo.rate); //Depth /*static_cast<uint16_t>( lround( static_cast<double>(lfo.rate) / 2.5 )*/
                    global.AddGenerator(eSFGen::delayVibLFO, MSecsToTimecents(lfo.delay)); //Delay

                    if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                        clog << "(1)pitch";
                }
                else if (lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Volume)) //Volume
                {
                    //The effect on the pitch can be handled this way
                    global.AddGenerator(eSFGen::freqModLFO, DSE_LFOFrequencyToCents(lfo.rate/*lfo.rate*//*/50*/)); //Frequency
                    global.AddGenerator(eSFGen::modLfoToVolume, -(lfo.depth * (20) / 127) /*//*(lfo.depth/10) * -1*/); //Depth
                    global.AddGenerator(eSFGen::delayModLFO, MSecsToTimecents(lfo.delay)); //Delay

                    if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                        clog << "(2)volume";
                }
                else if (lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Pan)) //Pan
                {
                    //Leave the data for the MIDI exporter, so maybe it can do something about it..
                    convinf.extrafx.push_back(
                        SMDLPresetConversionInfo::ExtraEffects{ SMDLPresetConversionInfo::eEffectTy::Phaser, lfo.rate, lfo.depth, lfo.delay }
                    );

                    //#TODO:
                    //We still need to figure a way to get the LFO involved, and set the oscilation frequency!
                    if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                        clog << "(3)pan";
                }
                else if (lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::UNK_4))
                {
                    //Unknown LFO target
                    if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                        clog << "(4)unknown";
                }
                else
                {
                    if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                        clog << "(" << static_cast<unsigned short>(lfo.dest) << ")unknown";
                }

                if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                {
                    clog << ", Frequency: " << static_cast<unsigned short>(lfo.rate)
                        << " Hz, Depth: " << static_cast<unsigned short>(lfo.depth)
                        << ", Delay: " << static_cast<unsigned short>(lfo.delay) << " ms\n";
                }
            }
            ++cntlfo;
        }

        //#3 - Add the global zone to the list!
        if (global.GetNbGenerators() > 0 || global.GetNbModulators() > 0)
            pre.AddZone(std::move(global));
    }


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Soundfont Builder
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    DSE_SoundFontBuilder::DSE_SoundFontBuilder(const DSE::PresetDatabase& db, bool blfoFxDisabled, bool bbakeSamples)
        :_db(db), 
        _lfoFxDisabled(blfoFxDisabled), 
        _bakeSamples(bbakeSamples),
        _cntExportedPresets(0),
        _cntExportedSplits(0)
    {}

    void DSE_SoundFontBuilder::Reset()
    {
        //Setup our instance for a conversion 
        _curPairName = {};
        _sf = sf2::SoundFont();
        _cntExportedPresets = 0;
        _cntExportedSplits = 0;
        _smplIdToSf2SmplId.clear();
        _bakedPresets = ProcessedPresets();
        _convertionInfo = SMDLPresetConversionInfo();
    }

    std::set<sampleid_t> DSE_SoundFontBuilder::ListUsedSamples(std::shared_ptr<ProgramBank> prgbank)
    {
        using namespace sf2;
        using namespace ::audio;
        if (!prgbank)
            return {};

        std::set<sampleid_t> usedsmpls;
        for (const ProgramBank::ptrprg_t& prgm : prgbank->PrgmInfo())
        {
            if (prgm == nullptr)
                continue;
            for (const SplitEntry& split : prgm->m_splitstbl)
            {
                usedsmpls.insert(split.smplid);
            }

        }
        return usedsmpls;
    }

    void DSE_SoundFontBuilder::ProcessSamples(std::set<sampleid_t> usedSamples)
    {
        using namespace sf2;
        using namespace ::audio;
        //for (sampleid_t cntsmslot = 0; cntsmslot < _db.getNbSampleBlocks(); ++cntsmslot)
        for(sampleid_t cntsmslot : usedSamples)
        {
            //Grab the sample and sample info from the last bank that defined them
            const SampleBank::SampleBlock* blk = _db.getSampleBlockFor(cntsmslot, _curPairName);
            if (!blk || blk->isnull())
            {
                if (utils::LibWide().isLogOn())
                    clog << "WARNING: Sample ID " << cntsmslot <<" is missing for pair \"" << _curPairName << "\"!\n";
                continue;
            }

            //Get our pointers setup
            WavInfo* wavi = blk->pinfo_.get();
            std::vector<uint8_t>* smpldata = blk->pdata_.get();
            std::vector<int16_t>        pcmout;
            DSESampleConvertionInfo     cursmplcv;

            //Convert the sample to pcm16 for the soundfont
            ConvertDSESample(
                static_cast<uint16_t>(wavi->smplfmt),
                wavi->loopbeg,
                *smpldata,
                cursmplcv,
                pcmout);

            //Do extra processing if neccessary
            if (_bakeSamples)
            {
                assert(false); //#TODO: Apply baked enveloppes and etc
            }

            //Add the sample to the soundfont and keep track of its new sample id
            const std::string smplname = (stringstream() << "smpl_0x" << uppercase << hex << cntsmslot).str();
            _smplIdToSf2SmplId.emplace(
                cntsmslot,
                _sf.AddSample(
                    Sample(
                        pcmout.begin(),
                        pcmout.end(),
                        smplname,
                        (wavi->smplloop != 0) ? cursmplcv.loopbeg_ : 0,
                        (wavi->smplloop != 0) ? cursmplcv.loopend_ : 0,
                        wavi->smplrate,
                        wavi->rootkey
                    )
                )
            );

        }
    }

    void DSE_SoundFontBuilder::ProcessPrograms(std::shared_ptr<ProgramBank> prgbank)
    {
        using namespace sf2;
        using namespace ::audio;

        //Now build the Preset and instrument list !
        for (const ProgramBank::ptrprg_t& prgm : prgbank->PrgmInfo())
        {
            if (!prgm) //They may be null
                continue;

            const bankid_t bankid_replacemeplease = 0;
            auto itnewcv = _convertionInfo.AddPresetConvInfo(prgm->id, SMDLPresetConversionInfo::PresetConvData(prgm->id, bankid_replacemeplease));
            if (itnewcv == _convertionInfo.end())
                assert(false); //Fixme

            SMDLPresetConversionInfo::PresetConvData& convinf = itnewcv->second;
            const std::string presetname(std::to_string(prgm->id));
            Preset            newpreset(presetname, convinf.midipres, bankid_replacemeplease);

            if (utils::LibWide().isLogOn())
                clog << "======================\nHandling " << presetname << "\n======================\n";

            //Add a global zone for global preset settings
            PrepareSFPresetForDSEPreset(*prgm, newpreset, convinf, !_lfoFxDisabled);

            //Convert instruments
            ZoneBag instzone;
            instzone.SetInstrumentId(_cntExportedSplits);
            ++_cntExportedSplits; //Increase the instrument count

            _sf.AddInstrument(ProcessSplits(prgm, convinf, prgbank));
            newpreset.AddZone(std::move(instzone)); //Add the instrument zone after the global zone!
            
            _sf.AddPreset(std::move(newpreset));
            ++_cntExportedPresets;
        }
    }

    sf2::Instrument DSE_SoundFontBuilder::ProcessSplits(const DSE::ProgramBank::ptrprg_t& prgm, SMDLPresetConversionInfo::PresetConvData& convinf, std::shared_ptr<ProgramBank> prgbank)
    {
        using namespace sf2;
        using namespace ::audio;

        //Had to use this, as stringstreams are just too slow for this..
        std::array<char, 48> instrument_name;
        snprintf(instrument_name.data(), instrument_name.size(), "Inst%zu\0", _cntExportedSplits);

        Instrument myinst(string(instrument_name.begin(), instrument_name.end()));
        for (uint16_t cntsplit = 0; cntsplit < prgm->m_splitstbl.size(); ++cntsplit)
        {
            const DSE::SplitEntry&   dsesplit = prgm->m_splitstbl[cntsplit];
            const DSE::KeyGroupList& dsekgrp  = prgbank->Keygrps();

            if (utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn())
                clog << "\t--- Split#" << static_cast<unsigned short>(dsesplit.id) << " ---\n";

            //Make a zone for this entry
            ZoneBag myzone;
            //## Key range and vel range MUST be first, or else the soundfont parser will ignore them! ##
            myzone.SetKeyRange(dsesplit.lowkey, dsesplit.hikey); //Key range always in first
            myzone.SetVelRange(dsesplit.lovel, dsesplit.hivel);
            //## Add other generators below ##

            //Fetch Loop Flag
            auto ptrinf = _db.getSampleInfoForBank(dsesplit.smplid, _curPairName); //Grab the latest overriden sample info
            if (ptrinf != nullptr && ptrinf->smplloop != 0)
                myzone.SetSmplMode(eSmplMode::loop);

            //Since the soundfont format has no control on polyphony, we can only cut keygroup that only allow a single voice
            if (dsesplit.kgrpid != 0)
                convinf.maxpoly = dsekgrp[dsesplit.kgrpid].poly; //let the midi converter handle it

            ////#Test set pitch from scratch:
            myzone.SetRootKey(dsesplit.rootkey);

            //Volume
            if (dsesplit.smplvol != DSE_LimitsVol.def_)
                myzone.SetInitAtt(DSE_VolToSf2Attenuation(dsesplit.smplvol));
            //Pan
            if (dsesplit.smplpan != DSE_LimitsPan.def_)
                myzone.SetPan(DSE_PanToSf2Pan(dsesplit.smplpan));

            //Volume Envelope
            Envelope myenv = RemapDSEVolEnvToSF2(dsesplit.env);

            /*
                ### In order to handle the atkvol param correctly, we'll make a copy of the Instrument,
                    with a very short envelope!

                #TODO : Make this part of the envelope parsing functions !!!
            */
            if (dsesplit.env.atkvol != 0 && dsesplit.env.attack != 0)
            {
                ZoneBag atkvolzone(myzone);

                //Set attenuation to the atkvol value
                atkvolzone.SetInitAtt(DSE_VolToSf2Attenuation(dsesplit.env.atkvol) +
                    DSE_VolToSf2Attenuation(dsesplit.smplvol));

                //Set hold to the attack's duration
                Envelope atkvolenv;
                atkvolenv.hold = myenv.attack;
                atkvolenv.decay = (dsesplit.env.hold > 0) ? myenv.hold : (dsesplit.env.decay > 0 || dsesplit.env.decay2 > 0) ? myenv.decay : std::numeric_limits<int16_t>::min();
                atkvolenv.sustain = SF_GenLimitsVolEnvSustain.max_;

                //Leave everything else to default
                atkvolzone.AddGenerator(eSFGen::holdVolEnv, atkvolenv.hold);
                atkvolzone.AddGenerator(eSFGen::sustainVolEnv, atkvolenv.sustain);

                //Sample ID in last
                atkvolzone.SetSampleId(_smplIdToSf2SmplId.at(dsesplit.smplid));
                myinst.AddZone(std::move(atkvolzone));
            }

            //Set the envelope
            myzone.SetVolEnvelope(myenv);
            //-- Sample ID in last --
            myzone.SetSampleId(_smplIdToSf2SmplId.at(dsesplit.smplid));
            myinst.AddZone(std::move(myzone));
        }
        return myinst;
    }

    sf2::SoundFont DSE_SoundFontBuilder::operator()(const DSE::PresetBank& bnk, const std::string & sfname)
    {
        //Setup our pointers
        shared_ptr<ProgramBank> prgbank  = bnk.prgmbank().lock();
        
        //Make sure we have a clean state
        Reset();
        _sf.SetName(sfname);
        _curPairName = bnk.metadata().get_original_file_name_no_ext();

        //Baking
        //if (_bakeSamples)
        //    _bakedPresets = ProcessDSESamples(*samples, *prgbank);

        //Convert samples format for sf2 + apply extra processing if needed
        ProcessSamples(ListUsedSamples(prgbank));
        //Next get programs updated to match the processed samples, and add them to the soundfont
        ProcessPrograms(prgbank);

        return std::move(_sf); //We move out our temporary soundfont
    }


};