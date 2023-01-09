#include <dse/dse_conversion.hpp>
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/dse_interpreter.hpp>
#include <dse/containers/dse_music_sequence.hpp>
#include <dse/containers/dse_preset_bank.hpp>
#include <dse/containers/dse_se_sequence.hpp>
#include <dse/dse_to_sf2.hpp>

#include <utils/library_wide.hpp>
#include <utils/audio_utilities.hpp>
#include <utils/poco_wrapper.hpp>
#include <utils/parse_utils.hpp>

#include <dse/fmts/sedl.hpp>
#include <dse/fmts/smdl.hpp>
#include <dse/fmts/swdl.hpp>

#include <dse/bgm_container.hpp>
#include <dse/bgm_blob.hpp>

#include <sstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <map>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>

#include <ext_fmts/adpcm.hpp>
#include <ext_fmts/sf2.hpp>
#include <ext_fmts/wav_io.hpp>

using namespace std;

namespace DSE
{

    //A multiplier to use to convert from DSE volume to soundfont attenuation.
    //static const double ByteVolToSounfontAttnMulti = static_cast<double>(DSE_LowestAttenuation_cB) / static_cast<double>(DSE_LimitsVol.max_);

    /*
    * Helper struct for gathering statistics from dse conversion.
    */
    struct audiostats
    {
        template<class T> struct LimitVal
        {
            typedef T val_t;
            val_t min;
            val_t avg;
            val_t max;
            int   cntavg; //Counts nb of value samples
            int   accavg; //Accumulate values

            LimitVal()
                :min(0), avg(0), max(0), cntavg(0), accavg(0)
            {}

            void Process(val_t anotherval)
            {
                if (anotherval < min)
                    min = anotherval;
                else if (anotherval > max)
                    max = anotherval;

                ++cntavg;
                accavg += anotherval;
                avg = static_cast<val_t>(accavg / cntavg);
            }

            std::string Print()const
            {
                std::stringstream sstr;
                sstr << "(" << static_cast<int>(min) << ", " << static_cast<int>(max) << " ) Avg : " << avg;
                return std::move(sstr.str());
            }
        };

        audiostats()
        {}

        std::string Print()const
        {
            std::stringstream sstr;
            sstr << "Batch Converter Statistics:\n"
                << "-----------------------------\n"
                << "\tlforate : " << lforate.Print() << "\n"
                << "\tlfodepth : " << lfodepth.Print() << "\n";
            return std::move(sstr.str());
        }

        LimitVal<int16_t> lforate;
        LimitVal<int16_t> lfodepth;
    };


//===========================================================================================
//  Utility Functions
//===========================================================================================

    eDSESmplFmt ConvertDSESample( int16_t                                smplfmt, 
                                  size_t                                 origloopbeg,
                                  const std::vector<uint8_t>           & in_smpl,
                                  DSESampleConvertionInfo              & out_cvinfo,
                                  std::vector<int16_t>                 & out_smpl )
    {
        if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::ima_adpcm4) )
        {
            out_smpl = move(::audio::DecodeADPCM_NDS( in_smpl ) );
            out_cvinfo.loopbeg_ = (origloopbeg - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
            out_cvinfo.loopend_ = out_smpl.size(); /*::audio::ADPCMSzToPCM16Sz(in_smpl.size() );*/
            return eDSESmplFmt::ima_adpcm4;
        }
        else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::pcm8) )
        {
            out_smpl = move(utils::PCM8RawBytesToPCM16Vec(&in_smpl) );
            out_cvinfo.loopbeg_ = origloopbeg    * 4; //loopbeg is counted in int32, for PCM8 data, so multiply by 4 to get the loop beg as pcm16
            out_cvinfo.loopend_ = out_smpl.size(); //PCM8 -> PCM16
            return eDSESmplFmt::pcm8;
        }
        else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::pcm16) )
        {
            out_smpl = move( utils::RawBytesToPCM16Vec(&in_smpl) );
            //out_smpl = vector<int16_t>( (in_smpl.size() / 2), 0 );
            out_cvinfo.loopbeg_ = (origloopbeg * 2); //loopbeg is counted in int32, so multiply by 2 to get the loop beg as pcm16
            out_cvinfo.loopend_ = out_smpl.size();

            //if( (out_cvinfo.loopbeg_ - 2) > 0 )
            //    out_cvinfo.loopbeg_ -= 2;

            //if( (out_cvinfo.loopbeg_ - 1) > 0 && out_cvinfo.loopbeg_ % 2 != 0 )
            //    out_cvinfo.loopbeg_ -= 1;
            //else if( (out_cvinfo.loopbeg_ - 3) > 0 && out_cvinfo.loopbeg_ % 2 == 0 )
            //    out_cvinfo.loopbeg_ -= 3;

            return eDSESmplFmt::pcm16;
        }
        else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::ima_adpcm3) )
        {
            clog << "<!>- Warning: ConvertDSESample(): Samplefmt PSG ???\n";
#ifdef _DEBUG
            assert(false);
#endif
            return eDSESmplFmt::ima_adpcm3;
        }
        else
            return eDSESmplFmt::invalid;
    }


    //eDSESmplFmt ConvertAndLoopDSESample( int16_t                                smplfmt, 
    //                                     size_t                                 origloopbeg,
    //                                     size_t                                 origlooplen,
    //                                     size_t                                 nbloops,
    //                                     const std::vector<uint8_t>           & in_smpl,
    //                                     DSESampleConvertionInfo              & out_cvinfo,
    //                                     std::vector<int16_t>                 & out_smpl )
    //{
    //    if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::ima_adpcm4) )
    //    {
    //        auto postconv = move( ::audio::LoopAndConvertADPCM_NDS( in_smpl, origloopbeg, origlooplen, nbloops, 1 ) );
    //        
    //        out_cvinfo.loopbeg_ = postconv.second;
    //        out_cvinfo.loopend_ = postconv.first.front().size();
    //        out_smpl = move(postconv.first.front());
    //        //out_cvinfo.loopbeg_ = (origloopbeg - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
    //        //out_cvinfo.loopend_ = ::audio::ADPCMSzToPCM16Sz(in_smpl.size() );
    //        return eDSESmplFmt::ima_adpcm4;
    //    }
    //    else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::pcm8) )
    //    {
    //        out_smpl = move( PCM8RawBytesToPCM16Vec(&in_smpl) );
    //        out_cvinfo.loopbeg_ = origloopbeg    * 4; //loopbeg is counted in int32, for PCM8 data, so multiply by 4 to get the loop beg as pcm16
    //        out_cvinfo.loopend_ = out_smpl.size(); //in_smpl.size() * 2; //PCM8 -> PCM16
    //        return eDSESmplFmt::pcm8;
    //    }
    //    else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::pcm16) )
    //    {
    //        out_smpl = move( RawBytesToPCM16Vec(&in_smpl) );
    //        cerr<< "\n#FIXME ConvertAndLoopDSESample(): PCM16 sample loop beginning kept as-is: " <<origloopbeg <<" \n";
    //        //#FIXME: changed this just to test if it improves pcm16 looping.
    //        out_cvinfo.loopbeg_ = origloopbeg;//    * 2; //loopbeg is counted in int32, so multiply by 2 to get the loop beg as pcm16
    //        out_cvinfo.loopend_ = out_smpl.size();//in_smpl.size() / 2;
    //        return eDSESmplFmt::pcm16;
    //    }
    //    else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::ima_adpcm3) )
    //    {
    //        return eDSESmplFmt::ima_adpcm3;
    //    }
    //    else
    //        return eDSESmplFmt::invalid;
    //}

//========================================================================================
//  BatchAudioLoader
//========================================================================================

    //BatchAudioLoader::BatchAudioLoader( bool singleSF2, bool lfofxenabled )
    //    : m_bSingleSF2(singleSF2),m_lfoeffects(lfofxenabled)
    //{}


    //void BatchAudioLoader::LoadMasterBank( const std::string & mbank )
    //{
    //    m_mbankpath = mbank;
    //    m_master = move( DSE::ParseSWDL( m_mbankpath ) );
    //}

    //void BatchAudioLoader::LoadSmdSwdPair( const std::string & smd, const std::string & swd )
    //{
    //    DSE::PresetBank    bank( move( DSE::ParseSWDL( swd ) ) );
    //    DSE::MusicSequence seq( move( DSE::ParseSMDL( smd ) ) );

    //    //Tag our files with their original file name, for cvinfo lookups to work!
    //    seq.metadata().origfname  = Poco::Path(smd).getBaseName();
    //    bank.metadata().origfname = Poco::Path(swd).getBaseName();

    //    m_pairs.push_back( move( std::make_pair( std::move(seq), std::move(bank) ) ) );
    //}

    /*
    */
    //uint16_t BatchAudioLoader::GetSizeLargestPrgiChunk()const
    //{
    //    uint16_t largestprgi = m_master.metadata().nbprgislots; //Start with the master bank

    //    //Then look at the loaded pairs
    //    for( size_t cntpair = 0; cntpair < m_pairs.size(); ++cntpair ) //Iterate through all SWDs
    //    {
    //        uint16_t curprgisz = m_pairs[cntpair].second.metadata().nbprgislots;
    //        if( curprgisz > largestprgi )
    //            largestprgi = curprgisz;
    //    }

    //    return largestprgi;
    //}

    //bool BatchAudioLoader::IsMasterBankLoaded()const
    //{
    //    return (m_master.smplbank().lock() != nullptr );
    //}

    /***************************************************************************************
        HandlePrgSplitBaked
    ***************************************************************************************/
    //void BatchAudioLoader::HandlePrgSplitBaked( sf2::SoundFont                     * destsf2, 
    //                                            const DSE::SplitEntry              & split,
    //                                            size_t                               sf2sampleid,
    //                                            const DSE::WavInfo                 & smplinf,
    //                                            const DSE::KeyGroup                & keygroup,
    //                                            SMDLPresetConversionInfo::PresetConvData  & presetcvinf,
    //                                            sf2::Instrument                    * destinstsf2 )
    //{
    //    using namespace sf2;

    //    if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //        clog <<"\t--- Split#" <<static_cast<unsigned short>(split.id) <<" ---\n";

    //    //Make a zone for this entry
    //    ZoneBag myzone;

    //    //  --- Set the generators ---
    //    //## Key range and vel range MUST be first, or else the soundfont parser will ignore them! ##
    //    myzone.SetKeyRange( split.lowkey, split.hikey ); //Key range always in first
    //    myzone.SetVelRange( split.lovel,  split.hivel );

    //    //## Add other generators below ##

    //    //Fetch Loop Flag
    //    if( smplinf.smplloop != 0 )
    //        myzone.SetSmplMode( eSmplMode::loop );

    //    //Since the soundfont format has no control on polyphony, we can only cut keygroup that only allow a single voice
    //    if( split.kgrpid != 0 )
    //        presetcvinf.maxpoly = keygroup.poly; //let the midi converter handle anything else

    //    int8_t ctuneadd = (split.ftune) / 100;/*100;*/
    //    int8_t ftunes   = (split.ftune) % 100;/*100;*/ // utils::Clamp( dseinst.ftune, sf2::SF_GenLimitsFineTune.min_, sf2::SF_GenLimitsFineTune.max_ ); 


    //    //Root Pitch
    //    myzone.SetRootKey( split.rootkey ); // + (split.ktps + split.ctune) + ctuneadd ); //split.rootkey
    //    //myzone.SetRootKey( split.rootkey + (/*split.ktps +*/ split.ctune) + ctuneadd );
    //    //cout << "\trootkey :" << static_cast<short>(split.rootkey + (split.ktps + split.ctune) + ctuneadd) <<"\n"; 

    //    //Pitch Correction
    //    //if( split.ftune != 0 )
    //    //    myzone.SetFineTune( ftunes );

    //    //if( split.ctune != DSE::DSEDefaultCoarseTune )
    //    //    myzone.SetCoarseTune( split.ctune );

    //    //Pitch Correction
    //    //if( split.ftune != 0 )
    //    //    myzone.SetFineTune( ftunes/*split.ftune*/ );

    //    //if( split.ctune != DSE::DSEDefaultCoarseTune )
    //    //    myzone.SetCoarseTune( /*( split.ctune + 7 ) +*/ split.ctune + ctuneadd );

    //    //Volume
    //    if( split.smplvol != DSE_LimitsVol.def_ )
    //        myzone.SetInitAtt( DSE_VolToSf2Attenuation(split.smplvol) );

    //    //Pan
    //    if( split.smplpan != DSE_LimitsPan.def_ )
    //        myzone.SetPan( DSE_PanToSf2Pan(split.smplpan) );

    //    //Volume Envelope
    //    //DSEEnvelope dseenv(split);
    //    Envelope myenv = RemapDSEVolEnvToSF2(split.env);

    //    //Set the envelope
    //    myzone.SetVolEnvelope( myenv );
    //    
    //    if( split.env.release != 0 )
    //        myzone.AddGenerator( eSFGen::releaseVolEnv, static_cast<sf2::genparam_t>( lround(myenv.release * 1.1) ) );

    //    //Sample ID in last
    //    myzone.SetSampleId( sf2sampleid );

    //    destinstsf2->AddZone( std::move(myzone) );
    //}


    /***************************************************************************************
        HandleBakedPrgInst
    ***************************************************************************************/
    //void BatchAudioLoader::HandleBakedPrgInst( const ProcessedPresets::PresetEntry   & entry, 
    //                                           sf2::SoundFont                        * destsf2, 
    //                                           const std::string                     & presname, 
    //                                           int                                     cntpair, 
    //                                           SMDLPresetConversionInfo::PresetConvData  & presetcvinf,
    //                                           int                                   & instidcnt,
    //                                           int                                   & presetidcnt,
    //                                           const DSE::KeyGroupList     & keygroups )
    //{
    //    using namespace sf2;
    //    Preset sf2preset(presname, presetcvinf.midipres, presetcvinf.midibank );

    //    const auto & curprg      = entry.prginf;
    //    const auto & cursmpls    = entry.splitsamples;  //PCM16 samples for each split slots
    //    const auto & cursmplsinf = entry.splitsmplinf;  //New sample info for each samples in entry.splitsamples

    //    if( utils::LibWide().isLogOn() )
    //        clog <<"======================\nHandling " <<presname <<"\n======================\n";


    //    //#0 - Add a global zone for global preset settings
    //    {
    //        ZoneBag global;

    //        //#1 - Setup Generators
    //        //if( curprg.m_hdr.insvol != DSE_LimitsVol.def_ )
    //        //    global.SetInitAtt( DseVolToSf2Attenuation(curprg.m_hdr.insvol) );

    //        // Range of DSE Pan : 0x00 - 0x40 - 0x7F
    //        if( curprg.prgpan != DSE_LimitsPan.def_ )
    //            global.SetPan( DSE_PanToSf2Pan(curprg.prgpan) );
    //        
    //        //#2 - Setup LFOs
    //        unsigned int cntlfo = 0;
    //        for( const auto & lfo : curprg.m_lfotbl )
    //        {
    //            //!#TODO: Put the content of this loop in a function!!!!
    //            if( lfo.unk52 != 0 && m_lfoeffects ) //Is the LFO enabled ?
    //            {
    //                if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //                    clog << "\tLFO" <<cntlfo <<" : Target: ";

    //                //Gather statistics
    //                m_stats.lfodepth.Process( lfo.depth );
    //                m_stats.lforate.Process( lfo.rate );
    //                
    //                if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Pitch) ) //Pitch
    //                {
    //                    if( lfo.rate <= 100 )
    //                    {
    //                        //The effect on the pitch can be handled this way
    //                        global.AddGenerator( eSFGen::freqVibLFO,    DSE_LFOFrequencyToCents( lfo.rate ) ); //Frequency
    //                        global.AddGenerator( eSFGen::vibLfoToPitch, DSE_LFODepthToCents( lfo.depth ) ); //Depth 
    //                        global.AddGenerator( eSFGen::delayVibLFO,   MSecsToTimecents( lfo.delay ) ); //Delay
    //                    
    //                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //                            clog << "(1)pitch";
    //                    }
    //                    else if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //                    {
    //                        clog << "<!>- LFO Vibrato effect was ignored, because the rate(" <<lfo.rate <<") is higher than what Soundfont LFOs supports!\n";
    //                    }
    //                }
    //                else if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Volume) ) //Volume
    //                {
    //                    if( lfo.rate <= 100 )
    //                    {
    //                        //The effect on the pitch can be handled this way
    //                        global.AddGenerator( eSFGen::freqModLFO,     DSE_LFOFrequencyToCents(lfo.rate ) ); //Frequency
    //                        global.AddGenerator( eSFGen::modLfoToVolume, DSE_LFODepthToCents( lfo.depth ) ); //Depth
    //                        global.AddGenerator( eSFGen::delayModLFO,    MSecsToTimecents( lfo.delay ) ); //Delay

    //                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //                            clog << "(2)volume";
    //                    }
    //                    else if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //                    {
    //                        clog << "<!>- LFO volume level effect was ignored, because the rate(" <<lfo.rate <<") is higher than what Soundfont LFOs supports!\n";
    //                    }
    //                }
    //                else if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Pan) ) //Pan
    //                {
    //                    //Leave the data for the MIDI exporter, so maybe it can do something about it..
    //                    presetcvinf.extrafx.push_back( 
    //                        SMDLPresetConversionInfo::ExtraEffects{ SMDLPresetConversionInfo::eEffectTy::Phaser, lfo.rate, lfo.depth, lfo.delay } 
    //                    );

    //                    //#TODO:
    //                    //We still need to figure a way to get the LFO involved, and set the oscilation frequency!
    //                    if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //                        clog << "(3)pan";
    //                }
    //                else if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::UNK_4) )
    //                {
    //                    //Unknown LFO target
    //                    if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //                        clog << "(4)unknown";
    //                }
    //                else
    //                {
    //                    if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //                        clog << "(" << static_cast<unsigned short>(lfo.dest) <<")unknown";
    //                }

    //                if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
    //                {
    //                    clog <<", Frequency: " << static_cast<unsigned short>(lfo.rate) 
    //                            << " Hz, Depth: " << static_cast<unsigned short>(lfo.depth) 
    //                            << ", Delay: " <<static_cast<unsigned short>(lfo.delay) <<" ms\n";
    //                }
    //            }
    //            ++cntlfo;
    //        }

    //        //#3 - Add the global zone to the list!
    //        if( global.GetNbGenerators() > 0 || global.GetNbModulators() > 0 )
    //            sf2preset.AddZone( std::move(global) );
    //    }     

    //    //Process
    //    stringstream sstrinstnames;
    //    sstrinstnames << "Prg" <<presetidcnt << "->Inst" <<instidcnt;
    //    Instrument myinst( sstrinstnames.str() );
    //    ZoneBag    instzone;


    //    if( cursmpls.empty() && cursmplsinf.empty() )
    //    {
    //        clog << "<!>- No samples!!!\n";
    //        assert(false);
    //        return;
    //    }
    //    else if( cursmpls.empty() || cursmplsinf.empty() )
    //    {
    //        clog << "<!>- Mismatch between the nb of sample info and converted samples for this program!!! ( " <<cursmpls.size() <<"," <<cursmplsinf.size() <<" )\n";
    //        assert(false);
    //        return;
    //    }

    //    if (curprg.m_splitstbl.size() != cursmpls.size())
    //    {
    //        clog << "<!>- Mismatch between the nb of splits(" <<curprg.m_splitstbl.size() <<") and samples available(" <<cursmpls.size() <<") for the current program!\n";
    //    }

    //    for( size_t cntsplit = 0; cntsplit < curprg.m_splitstbl.size() && cntsplit < cursmpls.size(); ++cntsplit )
    //    {
    //        const auto & cursplit = curprg.m_splitstbl[cntsplit]; 
    //        stringstream sstrnames;
    //        sstrnames <<"Prg" <<presetidcnt << "->Smpl" <<cntsplit;

    //        //Place the sample used by the current split in the soundfont
    //        sf2::Sample sampl(  cursmpls[cntsplit].begin(), 
    //                            cursmpls[cntsplit].end(), 
    //                            sstrnames.str(),
    //                            0,
    //                            0,
    //                            cursmplsinf[cntsplit].smplrate,
    //                            cursplit.rootkey );

    //        if( cursmplsinf[cntsplit].smplloop != 0 )
    //            sampl.SetLoopBounds( cursmplsinf[cntsplit].loopbeg, cursmplsinf[cntsplit].loopbeg + cursmplsinf[cntsplit].looplen );

    //        size_t sf2smplindex = destsf2->AddSample( move(sampl) );

    //        //Make sure the KGrp exists, because prof layton is sloppy..
    //        const auto * ptrkgrp = &(keygroups.front());
    //        if( cursplit.kgrpid < keygroups.size() )
    //            ptrkgrp = &(keygroups[cursplit.kgrpid]);

    //        //Add our split
    //        HandlePrgSplitBaked( destsf2, 
    //                             cursplit, 
    //                             sf2smplindex, 
    //                             cursmplsinf[cntsplit], 
    //                             *ptrkgrp, 
    //                             presetcvinf, 
    //                             &myinst );
    //    }


    //    destsf2->AddInstrument( std::move(myinst) );
    //    instzone.SetInstrumentId(instidcnt);
    //    sf2preset.AddZone( std::move(instzone) ); //Add the instrument zone after the global zone!
    //    ++instidcnt; //Increase the instrument count
    //    destsf2->AddPreset( std::move(sf2preset) );
    //    ++presetidcnt;

    //}


    /***************************************************************************************
        HandleBakedPrg
            Handles all presets for a file
    ***************************************************************************************/
    //void BatchAudioLoader::HandleBakedPrg( const ProcessedPresets                & entry, 
    //                                       sf2::SoundFont                        * destsf2, 
    //                                       const std::string                     & curtrkname, 
    //                                       int                                     cntpair,
    //                                       std::vector<SMDLPresetConversionInfo> & presetcvinf,
    //                                       int                                   & instidcnt,
    //                                       int                                   & presetidcnt,
    //                                       const DSE::KeyGroupList      & keygroups
//                                           )
//    {
//        using namespace sf2;
//
//        auto & cvinfo = presetcvinf[cntpair];
//
//        for( const auto & dseprg : entry )
//        {
//            const auto & curprg      = dseprg.second.prginf;
//            const auto & cursmpls    = dseprg.second.splitsamples;
//            const auto & cursmplsinf = dseprg.second.splitsmplinf;
//
//            auto itcvinf = cvinfo.FindConversionInfo( curprg.id );
//
//            if( itcvinf == cvinfo.end() )
//            {
//                clog << "<!>- Warning: The SWDL + SMDL pair #" <<cntpair <<", for preset " <<presetidcnt <<" is missing a conversion info entry.\n";
//#ifdef _DEBUG
//                assert(false);
//#endif
//                ++presetidcnt;
//                continue;   //Skip this preset
//            }
//
//            if( cursmplsinf.size() != cursmpls.size() )
//            {
//                clog << "<!>- Warning: The SWDL + SMDL pair #" <<cntpair <<", for preset " <<presetidcnt <<" has mismatched lists of splits and samples.\n";
//#ifdef _DEBUG
//                assert(false);
//#endif
//                ++presetidcnt;
//                continue;   //Skip this preset
//            }
//            
//            if( cursmplsinf.empty() )
//            {
//                clog << "<!>- Warning: The SWDL + SMDL pair #" <<cntpair <<", for preset " <<presetidcnt <<" has " <<cursmplsinf.size() <<" Sample info slots, and " <<cursmpls.size() <<" samples .\n";
//                ++presetidcnt;
//                continue;   //Skip this preset
//            }
//
//            stringstream sstrprgname;
//            sstrprgname << curtrkname << "_prg#" <<showbase <<hex <<curprg.id;
//            const string presname = move( sstrprgname.str() );
//
//
//            HandleBakedPrgInst( dseprg.second, destsf2, presname, cntpair, itcvinf->second, instidcnt, presetidcnt, keygroups );
//        }
//    }


    /***************************************************************************************
        ExportSoundfontBakedSamples
            
    ***************************************************************************************/
    //vector<SMDLPresetConversionInfo> BatchAudioLoader::ExportSoundfontBakedSamples( const std::string & destf )
    //{
    //    using namespace sf2;
    //    vector<SMDLPresetConversionInfo> trackprgconvlist;
    //    m_stats = audiostats(); //reset stats

    //    if((m_pairs.size() > std::numeric_limits<int8_t>::max()) && !m_bSingleSF2 )
    //    {
    //        cout << "<!>- Error: Got over 127 different SMDL+SWDL pairs and set to preserve program numbers. Splitting into multiple soundfonts is not implemented yet!\n";
    //        //We need to build several smaller sf2 files, each in their own sub-dir with the midis they're tied to!
    //        assert(false); //#TODO
    //    }

    //    //Multiple separate sf2
    //    if (!IsMasterBankLoaded() && !m_bSingleSF2)
    //    {
    //        trackprgconvlist = move(BuildPresetConversionDB());
    //        deque<ProcessedPresets> procpres; //We need to put all processed stuff in there, because the samples need to exist when the soundfont is written.

    //        //If no master bank is loaded, assume we use the swd in the pair to get our samples
    //        for (size_t cntpair = 0; cntpair < m_pairs.size(); ++cntpair )
    //        {
    //            const auto & curpair = m_pairs[cntpair];
    //            const auto   prgptr = curpair.second.prgmbank().lock();
    //            const auto   samples = curpair.second.smplbank().lock();
    //            string       pairname = Poco::Path(curpair.first.metadata().fname).makeFile().getBaseName();

    //            if (prgptr == nullptr)
    //                continue;

    //            SoundFont sf(pairname);
    //            procpres.push_back(std::move(ProcessDSESamples(*samples, *prgptr)));
    //            int cntpres = 0;
    //            int cntinst = 0;
    //            HandleBakedPrg(procpres.back(), &sf, pairname, cntpair, trackprgconvlist, cntinst, cntpres, prgptr->Keygrps());
    //            cout << "\r\tProcessing pairs.. " << right << setw(3) << setfill(' ') << ((cntpair * 100) / m_pairs.size()) << "%";

    //            //Write the soundfont
    //            try
    //            {
    //                string sf2path = Poco::Path(destf).absolute().makeParent().setFileName(to_string(cntpair) + "_" + pairname + ".sf2").toString();
    //                if( utils::LibWide().isLogOn() )
    //                {
    //                    clog <<"\nWriting Soundfont \"" <<sf2path <<"\"...\n"
    //                         <<"================================================================================\n";
    //                }
    //                sf.Write(sf2path);
    //            }
    //            catch( const overflow_error & e )
    //            {
    //                stringstream sstr;
    //                sstr <<"There are too many different parameters throughout the music files in the folder to extract them to a single soundfont file!\n"
    //                     << e.what() 
    //                     << "\n";
    //                throw runtime_error( sstr.str() );
    //            }
    //        }
    //        cout <<"\n";
    //        

    //    }
    //    else //single sf2
    //    {
    //        if (!IsMasterBankLoaded())
    //            BuildMasterFromPairs();

    //        trackprgconvlist = move(BuildPresetConversionDB());
    //        SoundFont sf(m_master.metadata().fname);

    //        //Prepare
    //        shared_ptr<SampleBank>  samples = m_master.smplbank().lock();
    //        deque<ProcessedPresets> procpres; //We need to put all processed stuff in there, because the samples need to exist when the soundfont is written.

    //        //Counters for the unique preset and instruments IDs
    //        int cntpres = 0;
    //        int cntinst = 0;

    //        for (size_t cntpair = 0; cntpair < m_pairs.size(); )
    //        {
    //            const auto & curpair = m_pairs[cntpair];
    //            const auto   prgptr = curpair.second.prgmbank().lock();
    //            string       pairname = "Trk#" + to_string(cntpair);

    //            if (prgptr != nullptr)
    //            {
    //                procpres.push_back(move(ProcessDSESamples(*samples, *prgptr)));
    //                HandleBakedPrg(procpres.back(), &sf, pairname, cntpair, trackprgconvlist, cntinst, cntpres, prgptr->Keygrps());
    //            }

    //            ++cntpair;
    //            if (prgptr != nullptr)
    //                cout << "\r\tProcessing samples.." << right << setw(3) << setfill(' ') << ((cntpair * 100) / m_pairs.size()) << "%";
    //        }
    //        cout <<"\n";

    //        //Write the soundfont
    //        try
    //        {
    //            if( utils::LibWide().isLogOn() )
    //            {
    //                clog <<"\nWriting Soundfont...\n"
    //                     <<"================================================================================\n";
    //            }
    //            cout<<"\tWriting soundfont \"" <<destf <<"\"..";
    //            sf.Write( destf );
    //            cout<<"\n";
    //        }
    //        catch( const overflow_error & e )
    //        {
    //            stringstream sstr;
    //            sstr <<"There are too many different parameters throughout the music files in the folder to extract them to a single soundfont file!\n"
    //                 << e.what() 
    //                 << "\n";
    //            throw runtime_error( sstr.str() );
    //        }
    //    }
    //    

    //    //Print Stats
    //    if( utils::LibWide().isLogOn() )
    //        clog << m_stats.Print();

    //    return move( trackprgconvlist );
    //}



    /***************************************************************************************
        ExportSoundfont
            
    ***************************************************************************************/
    //vector<SMDLPresetConversionInfo> BatchAudioLoader::ExportSoundfont( const std::string & destf)
    //{
    //    using namespace sf2;

    //    if((m_pairs.size() > std::numeric_limits<int8_t>::max()) && !m_bSingleSF2)
    //    {
    //        cout << "<!>- Error: Got over 127 different SMDL+SWDL pairs and set to preserve program numbers. Splitting into multiple soundfonts is not implemented yet!\n";
    //        //We need to build several smaller sf2 files, each in their own sub-dir with the midis they're tied to!
    //        assert(false); //#TODO
    //    }

    //    //If the main bank is not loaded, try to make one out of the loaded pairs!
    //    if( !IsMasterBankLoaded() )
    //        BuildMasterFromPairs();



    //    vector<SMDLPresetConversionInfo> merged = move( BuildPresetConversionDB() );
    //    SoundFont                        sf( m_master.metadata().fname ); 
    //    map<uint16_t,size_t>             swdsmplofftosf; //Make a map with as key a the sample id in the Wavi table, and as value the sample id in the sounfont!
    //    
    //    //Prepare samples list
    //    shared_ptr<SampleBank> samples = m_master.smplbank().lock();
    //    vector<bool>           loopedsmpls( samples->NbSlots(), false ); //Keep track of which samples are looped

    //    //Check all our sample slots and prepare loading them in the soundfont
    //    for( size_t cntsmslot = 0; cntsmslot < samples->NbSlots(); ++cntsmslot )
    //    {
    //        AddSampleToSoundfont( cntsmslot, samples, swdsmplofftosf, sf );
    //    }

    //    //Now build the Preset and instrument list !
    //    size_t   cntsf2presets = 0; //Count presets total
    //    uint16_t instsf2cnt    = 0;

    //    for( size_t cntpairs = 0; cntpairs < m_pairs.size(); ++cntpairs )// const auto & aswd : merged )
    //    {
    //        SMDLPresetConversionInfo & curcvinfo   = merged[cntpairs];                                //Get the conversion info for all presets in the current swd
    //        const auto & curpair     = m_pairs[cntpairs];                               //Get the current swd + smd pair
    //        const auto & curprginfos = curpair.second.prgmbank().lock()->PrgmInfo();    //Get the current SWD's program list
    //        const auto & curkgrp     = curpair.second.prgmbank().lock()->Keygrps();     //Get the current SWD's keygroup list
    //        auto         itcvtbl     = curcvinfo.begin();                      //Iterator on the conversion map!
    //        string       pairname    = "Trk#" + to_string(cntpairs);

    //        for( size_t prgcnt = 0; /*prgcnt < curcvinfo.size() && */itcvtbl != curcvinfo.end(); ++prgcnt, ++itcvtbl )
    //        {
    //            if( curprginfos[itcvtbl->first] != nullptr )
    //            {
    //                stringstream sstrprgname;
    //                sstrprgname << pairname << "_prg#" <<showbase <<hex <<curprginfos[itcvtbl->first]->id;
    //                DSEPresetToSf2Preset( sstrprgname.str(), //pairname + "_prg#" + to_string(curprginfos[itcvtbl->first]->m_hdr.id), 
    //                                        itcvtbl->second.midibank,
    //                                        *(curprginfos[itcvtbl->first]),
    //                                        swdsmplofftosf,
    //                                        sf,
    //                                        instsf2cnt,
    //                                        loopedsmpls,
    //                                        curkgrp,
    //                                        m_master.smplbank().lock(),
    //                                        itcvtbl->second,
    //                                        m_lfoeffects );

    //                ++cntsf2presets;
    //            }
    //            else
    //            {
    //                assert(false); //This should never happen..
    //            }
    //        }
    //    }

    //    //Write the soundfont
    //    try
    //    {
    //        sf.Write( destf );
    //    }
    //    catch( const overflow_error & e )
    //    {
    //        stringstream sstr;
    //        sstr <<"There are too many different parameters throughout the music files in the folder to extract them to a single soundfont file!\n"
    //             << e.what() 
    //             << "\n";
    //        throw runtime_error( sstr.str() );
    //    }

    //    return move( merged );
    //}

    /***************************************************************************************
        ExportXMLPrograms
            Export all the presets for each loaded swdl pairs! And if the sample data is 
            present, it will also export it!
    ***************************************************************************************/
    //void BatchAudioLoader::ExportXMLPrograms( const std::string & destf )
    //{
    //    assert(false);
    //    //static const string _DefaultMainSampleDirName = "mainbank";
    //    //static const string _DefaultSWDLSmplDirName   = "samples";

    //    ////If the main bank is not loaded, try to make one out of the loaded pairs!
    //    //if( IsMasterBankLoaded() )
    //    //{
    //    //    //Make the main sample bank sub-directory if we have a master bank
    //    //    Poco::File outmbankdir( Poco::Path( destf ).append(_DefaultMainSampleDirName).makeDirectory() );
    //    //    if(!outmbankdir.createDirectory())
    //    //        throw runtime_error("BatchAudioLoader::ExportXMLPrograms(): Error, couldn't create output main bank samples directory!");

    //    //    //Export Main Bank samples :
    //    //    ExportPresetBank( outmbankdir.path(), m_master, true, false );
    //    //}

    //    ////Iterate through all the tracks 

    //    //for( size_t i = 0; i < m_pairs.size(); ++i )
    //    //{
    //    //    Poco::Path fpath(destf);
    //    //    fpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname);
    //    //    
    //    //    if( ! Poco::File(fpath).createDirectory() )
    //    //    {
    //    //        clog <<"Couldn't create directory for track " <<fpath.toString() << "!\n";
    //    //        continue;
    //    //    }

    //    //    const auto & curpair     = m_pairs[cntpairs];                               //Get the current swd + smd pair
    //    //    const auto & curprginfos = curpair.second.prgmbank().lock()->PrgmInfo();    //Get the current SWD's program list
    //    //    const auto & curkgrp     = curpair.second.prgmbank().lock()->Keygrps();     //Get the current SWD's keygroup list
    //    //    auto         itcvtbl     = curcvinfo.begin();                      //Iterator on the conversion map!
    //    //    string       pairname    = "Trk#" + to_string(cntpairs);

    //    //}

    //}

    /***************************************************************************************
        ExportSoundfontAndMIDIs
    ***************************************************************************************/
    //void BatchAudioLoader::ExportSoundfontAndMIDIs( const std::string & destdir, int nbloops, bool bbakesamples, bool bmultiplesf2)
    //{
    //    //Export the soundfont first

    //    Poco::Path outsoundfont(destdir);
    //    outsoundfont.append( outsoundfont.getBaseName() + ".sf2").makeFile();
    //    cerr<<"<*>- Currently processing soundfont.." <<"\n";

    //    vector<SMDLPresetConversionInfo> merged;
    //    if( bbakesamples )
    //        merged = std::move( ExportSoundfontBakedSamples( outsoundfont.toString() ) );
    //    else
    //        merged = std::move( ExportSoundfont( outsoundfont.toString() ) );

    //    //Then the MIDIs
    //    for( size_t i = 0; i < m_pairs.size(); ++i )
    //    {
    //        Poco::Path fpath(destdir);
    //        fpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname);
    //        fpath.makeFile();
    //        fpath.setExtension("mid");

    //        cerr<<"<*>- Currently exporting smd to " <<fpath.toString() <<"\n";
    //        DSE::SequenceToMidi( fpath.toString(), 
    //                             m_pairs[i].first, 
    //                             merged[i],
    //                             nbloops,
    //                             DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!
    //    }
    //}

    /***************************************************************************************
        ExportXMLAndMIDIs
            Export all music sequences to midis and export all preset data to 
            xml + pcm16 samples!
    ***************************************************************************************/
    //void BatchAudioLoader::ExportXMLAndMIDIs( const std::string & destdir, int nbloops )
    //{
    //    static const string _DefaultMainSampleDirName = "mainbank";

    //    if( IsMasterBankLoaded() )
    //    {
    //        //Make the main sample bank sub-directory if we have a master bank
    //        //Poco::File outmbankdir( Poco::Path( destdir ).append(_DefaultMainSampleDirName).makeDirectory() );
    //        Poco::Path outmbankpath( destdir );
    //        outmbankpath.append(_DefaultMainSampleDirName).makeDirectory().toString();

    //        
    //        if(! utils::DoCreateDirectory(outmbankpath.toString())  )
    //            throw runtime_error("BatchAudioLoader::ExportXMLPrograms(): Error, couldn't create output main bank samples directory!");

    //        //Export Main Bank samples :
    //        cerr<<"<*>- Currently exporting main bank to " <<outmbankpath.toString() <<"\n";
    //        ExportPresetBank( outmbankpath.toString(), m_master, false, false );
    //    }

    //    //Then the MIDIs + presets + optionally samples contained in the swd of the pair
    //    for( size_t i = 0; i < m_pairs.size(); ++i )
    //    {
    //        Poco::Path fpath(destdir);
    //        fpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname);
    //        
    //        if( ! utils::DoCreateDirectory(fpath.toString()) )
    //        {
    //            clog <<"Couldn't create directory for track " <<fpath.toString() << "!\n";
    //            continue;
    //        }

    //        Poco::Path midpath(fpath);
    //        midpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname);
    //        midpath.makeFile();
    //        midpath.setExtension("mid");

    //        cerr<<"<*>- Currently exporting smd + swd to " <<fpath.toString() <<"\n";
    //        DSE::SequenceToMidi( midpath.toString(), 
    //                             m_pairs[i].first, 
    //                             nbloops,
    //                             DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!

    //        ExportPresetBank( fpath.toString(), m_pairs[i].second, false, false );
    //    }
    //}


    /***************************************************************************************
        BuildMasterFromPairs
            If no main bank is loaded, and the loaded pairs contain their own samples, 
            build a main bank from those!
    ***************************************************************************************/
    //void BatchAudioLoader::BuildMasterFromPairs()
    //{
    //    vector<SampleBank::SampleBlock> smpldata;
    //    bool                           bnosmpldata = true;

    //    //Iterate through all the pairs and fill up our sample data list !
    //    for( size_t cntpairs = 0; cntpairs < m_pairs.size(); cntpairs++ )
    //    {
    //        auto & presetbank = m_pairs[cntpairs].second;
    //        auto   ptrprgs    = presetbank.prgmbank().lock();
    //        auto   ptrsmplbnk = presetbank.smplbank().lock();

    //        if( ptrsmplbnk != nullptr  )
    //        {
    //            //We have sample data
    //            bnosmpldata = false;

    //            if( ptrprgs != nullptr )
    //            {
    //                //Enlarge our vector when needed!
    //                //if( ptrsmplbnk->NbSlots() > smpldata.size() )
    //                //    smpldata.resize( ptrsmplbnk->NbSlots() );

    //                //Copy the samples over into their matching slot!
    //                for( size_t cntsmplslot = 0; cntsmplslot < ptrsmplbnk->NbSlots(); ++cntsmplslot )
    //                {
    //                    if( ptrsmplbnk->IsDataPresent(cntsmplslot) && 
    //                        ptrsmplbnk->IsInfoPresent(cntsmplslot) /*&&
    //                        !smpldata[cntsmplslot].pdata_ &&
    //                        !smpldata[cntsmplslot].pinfo_*/ )
    //                    {
    //                        for( auto & prgm : ptrprgs->PrgmInfo() )
    //                        {
    //                            if( prgm != nullptr )
    //                            {
    //                                for( auto & split : prgm->m_splitstbl )
    //                                {
    //                                    if( split.smplid == cntsmplslot )
    //                                    {
    //                                        split.smplid = smpldata.size(); //Reassign sample IDs to our unified table!
    //                                    }
    //                                }
    //                            }
    //                        }

    //                        //Insert a new sample entry in the table!
    //                        SampleBank::SampleBlock blk;
    //                        blk.pinfo_.reset( new DSE::WavInfo        (*ptrsmplbnk->sampleInfo(cntsmplslot) ) );
    //                        blk.pdata_.reset( new std::vector<uint8_t>(*ptrsmplbnk->sample(cntsmplslot)) );
    //                        smpldata.push_back( std::move(blk) );
    //                    }
    //                }
    //            }
    //        }
    //    }

    //    if( bnosmpldata )
    //        throw runtime_error("BatchAudioLoader::BuildMasterFromPairs(): No sample data found in the SWDL containers that were loaded! Its possible the main bank was not loaded, or that no SWDL were loaded.");

    //    DSE_MetaDataSWDL meta;
    //    meta.fname       = "Main.SWD";
    //    meta.nbprgislots = 0;
    //    meta.nbwavislots = static_cast<uint16_t>( smpldata.size() );

    //    m_master = move( PresetBank( move(meta), 
    //                                 move( unique_ptr<SampleBank>(new SampleBank(move( smpldata ))) ) 
    //                               ) );
    //}
    //{
    //    vector<SampleBank::smpldata_t> smpldata;
    //    bool                           bnosmpldata = true;

    //    //Iterate through all the pairs and fill up our sample data list !
    //    for( size_t cntpairs = 0; cntpairs < m_pairs.size(); cntpairs++ )
    //    {
    //        auto & presetbank = m_pairs[cntpairs].second;
    //        auto   ptrsmplbnk = presetbank.smplbank().lock();

    //        if( ptrsmplbnk != nullptr )
    //        {
    //            //We have sample data
    //            bnosmpldata = false;


    //            //!#FIXME: This is really stupid. Not all games alloc the same ammount of samples as eachothers. 

    //            //Enlarge our vector when needed!
    //            if( ptrsmplbnk->NbSlots() > smpldata.size() )
    //                smpldata.resize( ptrsmplbnk->NbSlots() );

    //            //Copy the samples over into their matching slot!
    //            for( size_t cntsmplslot = 0; cntsmplslot < ptrsmplbnk->NbSlots(); ++cntsmplslot )
    //            {
    //                if( ptrsmplbnk->IsDataPresent(cntsmplslot) && 
    //                    ptrsmplbnk->IsInfoPresent(cntsmplslot) &&
    //                    !smpldata[cntsmplslot].pdata_ &&
    //                    !smpldata[cntsmplslot].pinfo_ )
    //                {
    //                    smpldata[cntsmplslot].pinfo_.reset( new DSE::WavInfo(*ptrsmplbnk->sampleInfo(cntsmplslot) ) );
    //                    smpldata[cntsmplslot].pdata_.reset( new std::vector<uint8_t>(*ptrsmplbnk->sample(cntsmplslot)) );
    //                }
    //            }
    //        }
    //    }

    //    if( bnosmpldata )
    //        throw runtime_error("BatchAudioLoader::BuildMasterFromPairs(): No sample data found in the SWDL containers that were loaded! Its possible the main bank was not loaded, or that no SWDL were loaded.");

    //    DSE_MetaDataSWDL meta;
    //    meta.fname       = "Main.SWD";
    //    meta.nbprgislots = 0;
    //    meta.nbwavislots = static_cast<uint16_t>( smpldata.size() );

    //    m_master = move( PresetBank( move(meta), 
    //                                 move( unique_ptr<SampleBank>(new SampleBank(move( smpldata ))) ) 
    //                               ) );
    //}


    /***************************************************************************************
        LoadMatchedSMDLSWDLPairs
            This function loads all matched smdl + swdl pairs in one or two different 
            directories.
    ***************************************************************************************/
    //void BatchAudioLoader::LoadMatchedSMDLSWDLPairs( const std::string & swdldir, const std::string & smdldir )
    //{
    //    //Grab all the swd and smd pairs in the folder
    //    Poco::DirectoryIterator dirit(smdldir);
    //    Poco::DirectoryIterator diritend;
    //    cout << "<*>- Loading matched smd in the " << smdldir <<" directory..\n";

    //    unsigned int cntparsed = 0;
    //    while( dirit != diritend )
    //    {
    //        string fext = dirit.path().getExtension();
    //        std::transform(fext.begin(), fext.end(), fext.begin(), ::tolower);

    //        //Check all smd/swd file pairs
    //        if( fext == SMDL_FileExtension )
    //        {
    //            Poco::File matchingswd( Poco::Path(swdldir).append(dirit.path().getBaseName()).makeFile().setExtension(SWDL_FileExtension) );
    //                
    //            if( matchingswd.exists() && matchingswd.isFile() )
    //            {
    //                cout <<"\r[" <<setfill(' ') <<setw(4) <<right <<cntparsed <<" pairs loaded] - Currently loading : " <<dirit.path().getFileName() <<"..";
    //                LoadSmdSwdPair( dirit.path().toString(), matchingswd.path() );
    //                ++cntparsed;
    //            }
    //            else
    //                cout<<"<!>- File " << dirit.path().toString() <<" is missing a matching .swd file! Skipping !\n";
    //        }
    //        ++dirit;
    //    }
    //    cout <<"\n..done\n\n";
    //}



    /*
        LoadBgmContainer
            Load a single bgm container file.
            Bgm containers are SWDL and SMDL pairs packed into a single file using a SIR0 container.
    */
    //void BatchAudioLoader::LoadBgmContainer( const std::string & file )
    //{
    //    if( utils::LibWide().isLogOn() )
    //        clog << "--------------------------------------------------------------------------\n"
    //             << "Parsing BGM container \"" <<Poco::Path(file).getFileName() <<"\"\n"
    //             << "--------------------------------------------------------------------------\n";

    //    auto pairdata( move( ReadBgmContainer( file ) ) );
    //    //Tag our files with their original file name, for cvinfo lookups to work!
    //    pairdata.first.metadata().origfname  = Poco::Path(file).getBaseName();
    //    pairdata.second.metadata().origfname = Poco::Path(file).getBaseName();

    //    m_pairs.push_back( move( std::make_pair( std::move(pairdata.second), std::move(pairdata.first) ) ) );
    //}

    /*
        LoadBgmContainers
            Load all pairs in the folder. 
            Bgm containers are SWDL and SMDL pairs packed into a single file using a SIR0 container.

            - bgmdir : The directory where the bgm containers are located at.
            - ext    : The file extension the bgm container files have.
    */
    //void BatchAudioLoader::LoadBgmContainers( const std::string & bgmdir, const std::string & ext )
    //{
    //    //Grab all the bgm containers in here
    //    Poco::DirectoryIterator dirit(bgmdir);
    //    Poco::DirectoryIterator diritend;
    //    cout << "<*>- Loading bgm containers *." <<ext <<" in the " << bgmdir <<" directory..\n";

    //    stringstream sstrloadingmesage;
    //    sstrloadingmesage << " *." <<ext <<" loaded] - Currently loading : ";
    //    const string loadingmsg = sstrloadingmesage.str();

    //    unsigned int cntparsed = 0;
    //    while( dirit != diritend )
    //    {
    //        if( dirit->isFile() )
    //        {
    //            string fext = dirit.path().getExtension();
    //            std::transform(fext.begin(), fext.end(), fext.begin(), ::tolower);

    //            //Check all smd/swd file pairs
    //            try
    //            {
    //                if( fext == ext )
    //                {
    //                    cout <<"\r[" <<setfill(' ') <<setw(4) <<right <<cntparsed <<loadingmsg <<dirit.path().getFileName() <<"..";
    //                    LoadBgmContainer( dirit.path().absolute().toString() );
    //                    ++cntparsed;
    //                }
    //            }
    //            catch( std::exception & e )
    //            {
    //                cerr <<"\nBatchAudioLoader::LoadBgmContainers(): Exception : " <<e.what() <<"\n"
    //                     <<"Skipping file and attemnpting to recover!\n\n";
    //                if( utils::LibWide().isLogOn() )
    //                {
    //                    clog <<"\nBatchAudioLoader::LoadBgmContainers(): Exception : " <<e.what() <<"\n"
    //                     <<"Skipping file and attemnpting to recover!\n\n";
    //                }
    //            }
    //        }
    //        ++dirit;
    //    }
    //    cout <<"\n..done\n\n";
    //}

    //void BatchAudioLoader::LoadSingleSWDLs(const std::string& swdldir)
    //{
    //    //Grab all swd from the directory
    //    cout << "<*>- Loading swd files in directory \"" << swdldir << "\"..\n";
    //    utils::ProcessAllFileWithExtension(swdldir, SWDL_FileExtension, [this](const string& fpath, int cntprocessed) {
    //            cout << "\r[" << setfill(' ') << setw(4) << right << cntprocessed << " swd loaded] - Currently loading : " << Poco::Path(fpath).getFileName() << "..";
    //            LoadSWDL(fpath);
    //        });
    //    cout << "\n..done\n\n";
    //}

    //void BatchAudioLoader::LoadSWDL(const std::string& swdl)
    //{
    //    m_pairs.push_back(make_pair( MusicSequence(), ParseSWDL(swdl)));
    //    //Tag our files with their original file name, for cvinfo lookups to work!
    //    m_pairs.back().second.metadata().origfname = Poco::Path(swdl).getBaseName();
    //}

    ///*
    //    LoadSingleSMDLs
    //        Loads only SMDL in the folder.
    //*/
    //void BatchAudioLoader::LoadSingleSMDLs( const std::string & smdldir )
    //{
    //    //Grab all the smd in the directory
    //    cout << "<*>- Loading smd files in directory \"" << smdldir << "\"..\n";
    //    utils::ProcessAllFileWithExtension(smdldir, SMDL_FileExtension, [this](const string& fpath, int cntprocessed) 
    //        {
    //            cout << "\r[" << setfill(' ') << setw(4) << right << cntprocessed << " smd loaded] - Currently loading : " << Poco::Path(fpath).getFileName() << "..";
    //            LoadSMDL(fpath);
    //        });
    //    cout <<"\n..done\n\n";
    //}

    ///*
    //*/
    //void BatchAudioLoader::LoadSMDL( const std::string & smdl )
    //{
    //    m_pairs.push_back( make_pair( ParseSMDL(smdl), PresetBank() ) );
    //    //Tag our files with their original file name, for cvinfo lookups to work!
    //    m_pairs.back().first.metadata().origfname = Poco::Path(smdl).getBaseName();
    //}


    //void BatchAudioLoader::LoadSingleSEDLs(const std::string& sedldir)
    //{

    //}

    //void BatchAudioLoader::LoadSEDL(const std::string& sedl)
    //{
    //    m_sedl;
    //    //Tag our files with their original file name, for cvinfo lookups to work!
    //    m_sedl.back().metadata().origfname = Poco::Path(sedl).getBaseName();
    //}

    ///*
    //*/
    //void BatchAudioLoader::ExportMIDIs( const std::string & destdir, const std::string & cvinfopath, int nbloops )
    //{
    //    DSE::SMDLConvInfoDB cvinf;

    //    if( ! cvinfopath.empty() )
    //        cvinf.Parse( cvinfopath );

    //    //Then the MIDIs
    //    for( size_t i = 0; i < m_pairs.size(); ++i )
    //    {
    //        //Lookup cvinfo with the original filename from the game filesystem!
    //        auto itfound = cvinf.end();

    //        if(! cvinf.empty() )
    //            itfound = cvinf.FindConversionInfo( m_pairs[i].first.metadata().origfname );

    //        Poco::Path fpath(destdir);
    //        fpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname).makeFile().setExtension("mid");

    //        cout <<"<*>- Currently exporting smd to " <<fpath.toString() <<"\n";
    //        if( utils::LibWide().isLogOn() )
    //            clog <<"<*>- Currently exporting smd to " <<fpath.toString() <<"\n";

    //        if( itfound != cvinf.end() )
    //        {
    //            if( utils::LibWide().isLogOn() )
    //                clog << "<*>- Got conversion info for this track! MIDI will be remapped accordingly!\n";
    //            DSE::SequenceToMidi( fpath.toString(), 
    //                                 m_pairs[i].first, 
    //                                 itfound->second,
    //                                 nbloops,
    //                                 DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!
    //        }
    //        else
    //        {
    //            if( utils::LibWide().isLogOn() )
    //                clog <<"<!>- Couldn't find a conversion info entry for this SMDL! Falling back to converting as-is..\n";
    //            DSE::SequenceToMidi( fpath.toString(), 
    //                                 m_pairs[i].first, 
    //                                 nbloops,
    //                                 DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!
    //        }
    //    }
    //}


    ///*
    //*/
    //void BatchAudioLoader::LoadFromBlobFile(const std::string & blob, bool matchbyname)
    //{

    //    vector<uint8_t> filedata = utils::io::ReadFileToByteVector( blob );

    //    BlobScanner<vector<uint8_t>::const_iterator> blobscan( filedata.begin(), filedata.end(), matchbyname);
    //    blobscan.Scan();
    //    auto foundpairs = blobscan.ListAllMatchingSMDLPairs();

    //    size_t cntpairs = 0;
    //    cout << "-----------------------------------------\n"
    //         << "Loading pairs from blob " <<blob <<"..\n"
    //         << "-----------------------------------------\n";
    //    for( const auto apair : foundpairs )
    //    {
    //        if( utils::LibWide().isLogOn() )
    //        {
    //            clog <<"====================================================================\n"
    //                 <<"Parsing SWDL " <<apair.first._name <<"\n"
    //                 <<"====================================================================\n";
    //        }
    //        DSE::PresetBank    bank( move( DSE::ParseSWDL( apair.first._beg,  apair.first._end  ) ) );

    //        if( utils::LibWide().isLogOn() )
    //        {
    //            clog <<"====================================================================\n"
    //                 <<"Parsing SMDL " <<apair.second._name <<"\n"
    //                 <<"====================================================================\n";
    //        }
    //        DSE::MusicSequence seq ( move( DSE::ParseSMDL( apair.second._beg, apair.second._end ) ) );

    //        //Tag our files with their original file name, for cvinfo lookups to work!
    //        seq.metadata().origfname  = apair.second._name;
    //        bank.metadata().origfname = apair.first._name;

    //        m_pairs.push_back( move( std::make_pair( std::move(seq), std::move(bank) ) ) );

    //        ++cntpairs;
    //        cout <<"\r[" <<setfill(' ') <<setw(4) <<right <<cntpairs <<" of " <<foundpairs.size() <<" loaded]" <<"..";
    //    }

    //    cout<<"\n..done!\n";
    //}

    ///*
    //*/
    //void BatchAudioLoader::LoadSMDLSWDLSPairsFromBlob( const std::string & blob, bool matchbyname )
    //{
    //    Poco::Path blobpath(blob);
    //    if(blobpath.absolute().isDirectory())
    //    {
    //        Poco::DirectoryIterator dirit(blobpath);
    //        Poco::DirectoryIterator dirend;
    //        while (dirit != dirend)
    //        {
    //            if (dirit.path().isFile())
    //            {
    //                LoadFromBlobFile(dirit.path().toString(), matchbyname);
    //            }
    //            ++dirit;
    //        }
    //    }
    //    else
    //        LoadFromBlobFile(blob, matchbyname);

    //}

    ///*
    //*/
    //void BatchAudioLoader::LoadSMDLSWDLPairsAndBankFromBlob( const std::string & blob, const std::string & bankname )
    //{
    //    vector<uint8_t> filedata = utils::io::ReadFileToByteVector( blob );

    //    BlobScanner<vector<uint8_t>::const_iterator> blobscan( filedata.begin(), filedata.end() );
    //    blobscan.Scan();
    //    auto foundpairs = blobscan.ListAllMatchingSMDLPairs();

    //    for( const auto apair : foundpairs )
    //    {
    //        if( utils::LibWide().isLogOn() )
    //        {
    //            clog <<"====================================================================\n"
    //                 <<"Parsing SWDL " <<apair.first._name <<"\n"
    //                 <<"====================================================================\n";
    //        }
    //        DSE::PresetBank    bank( move( DSE::ParseSWDL( apair.first._beg,  apair.first._end  ) ) );

    //        if( utils::LibWide().isLogOn() )
    //        {
    //            clog <<"====================================================================\n"
    //                 <<"Parsing SMDL " <<apair.second._name <<"\n"
    //                 <<"====================================================================\n";
    //        }
    //        DSE::MusicSequence seq ( move( DSE::ParseSMDL( apair.second._beg, apair.second._end ) ) );

    //        //Tag our files with their original file name, for cvinfo lookups to work!
    //        seq.metadata().origfname  = apair.second._name;
    //        bank.metadata().origfname = apair.first._name;

    //        m_pairs.push_back( move( std::make_pair( std::move(seq), std::move(bank) ) ) );
    //    }

    //    string fixedbankname;
    //    if( bankname.size() > BlobScanner<vector<uint8_t>::const_iterator>::FilenameLength )
    //        fixedbankname = bankname.substr( 0, BlobScanner<vector<uint8_t>::const_iterator>::FilenameLength );

    //    auto foundBank = blobscan.FindSWDL( fixedbankname );

    //    if( !foundBank.empty() && foundBank.size() == 1 )
    //    {
    //        const auto & swdlfound = foundBank.front();
    //        
    //        if( utils::LibWide().isLogOn() )
    //        {
    //            clog <<"====================================================================\n"
    //                 <<"Parsing SWDL " <<swdlfound._name <<"\n"
    //                 <<"====================================================================\n";
    //        }
    //        DSE::PresetBank bank( move( DSE::ParseSWDL( swdlfound._beg, swdlfound._end ) ) );
    //        bank.metadata().origfname = swdlfound._name;
    //        m_master = move(bank);
    //    }
    //    else if( foundBank.empty() )
    //    {
    //        stringstream sstrer;
    //        sstrer << "BatchAudioLoader::LoadSMDLSWDLPairsAndBankFromBlob() : Couldn't find a SWDL bank named \""
    //               << fixedbankname
    //               << "\" ! (Name was trimed to 16 characters)";
    //        throw runtime_error( sstrer.str() );
    //    }
    //    else
    //    {
    //        stringstream sstrer;
    //        sstrer << "BatchAudioLoader::LoadSMDLSWDLPairsAndBankFromBlob() : Found more than a single SWDL bank named \""
    //               << fixedbankname
    //               << "\" ! (Name was trimed to 16 characters)";
    //        throw runtime_error( sstrer.str() );
    //    }

    //}

//===========================================================================================
//  Functions
//===========================================================================================
    const string _DefaultSamplesSubDir = "samples"s;
    const string SupportedImportSound_Wav = "wav"s;
    const string SupportedImportSound_Adpcm = "adpcm"s;

    DSE::PresetBank ImportPresetBank(const std::string& pathbankxml)
    {
        const std::string samplesdir = Poco::Path::transcode(Poco::Path(pathbankxml).makeAbsolute().setExtension("").makeDirectory().toString());
        PresetBank        bnk        = XMLToPresetBank(pathbankxml);
        Poco::File        smplsdir(samplesdir);

        //Handle sound samples:
        if (smplsdir.exists() && smplsdir.isDirectory())
        {
            Poco::DirectoryIterator itdir(smplsdir);
            Poco::DirectoryIterator itdirend;
            map<unsigned int, Poco::Path> samplepaths;
            //Read samples
            for (Poco::DirectoryIterator itdir(smplsdir); itdir != itdirend; ++itdir)
            {
                if (itdir->isFile())
                {
                    Poco::Path curfile(itdir->path());
                    if (curfile.getExtension() == SupportedImportSound_Wav || curfile.getExtension() == SupportedImportSound_Adpcm)
                        samplepaths[utils::parseHexaValToValue<unsigned int>(curfile.getBaseName())] = curfile; //Make sure we sort hexadecimal names properly
                }
            }

            auto samplebank = bnk.smplbank().lock();
            for (auto entry : samplepaths)
            {
                WavInfo * psmplinfo = samplebank->sampleInfo(entry.first);

                string ext = entry.second.getExtension();
                if (ext == SupportedImportSound_Wav)
                {
                    vector<uint8_t> fdat = utils::io::ReadFileToByteVector(entry.second.toString());
                    wave::WAVE_fmt_chunk fmt = wave::GetWaveFormatInfo(fdat.begin(), fdat.end());
                    if (fmt.bitspersample_ == 8)
                    {
                        wave::PCM8WaveFile fl;
                        fl.ReadWave(fdat.begin(), fdat.end());
                        auto samples = fl.GetSamples().front(); //Always one channel only

                        //#TODO: Check wtf is going on here?
                        for (auto & asample : samples)
                            asample ^= 0x80; //Since we did this on export, do it on import too. Flip the first bit, to turn from 2's complement to offset binary(excess-K).

                        samplebank->setSampleData(entry.first, std::move(samples));
                        if (psmplinfo)
                        {
                            psmplinfo->smplfmt = eDSESmplFmt::pcm8;
                            //psmplinfo->loopbeg /= 2;
                            //psmplinfo->looplen /= 2;
                        }
                    }
                    else if(fmt.bitspersample_ == 16)
                    {
                        wave::PCM16sWaveFile fl;
                        fl.ReadWave(fdat.begin(), fdat.end());
                        auto samples = fl.GetSamples().front(); //Always one channel only
                        samplebank->setSampleData(entry.first, utils::PCM16VecToRawBytes(&samples));
                        if (psmplinfo)
                        {
                            psmplinfo->smplfmt = eDSESmplFmt::pcm16;
                        }
                    }
                    
                }
                else if (ext == SupportedImportSound_Adpcm)
                {
                    samplebank->setSampleData(entry.first, audio::ReadADPCMDump(entry.second.toString()));
                    if (psmplinfo)
                    {
                        psmplinfo->smplfmt = eDSESmplFmt::ima_adpcm4;
                        //psmplinfo->loopbeg = (psmplinfo->loopbeg / 4) - ::audio::IMA_ADPCM_PreambleLen;
                        //psmplinfo->looplen = (psmplinfo->loopbeg / 4) - ::audio::IMA_ADPCM_PreambleLen;
                    }
                }
            }
        }

        return bnk;
    }

    void ExportPresetBank( const std::string & bnkxmlfile, const DSE::PresetBank & bnk, bool samplesonly, bool hexanumbers, bool noconvert )
    {
        auto smplptr = bnk.smplbank().lock();

        if (!samplesonly)
            PresetBankToXML(bnk, bnkxmlfile);
        
        if (smplptr == nullptr)
        {
            clog << "\tNo samples to export!\n";
            return;
        }

        //Create a directory named after the bank to put the samples in
        const std::string sampledir = Poco::Path::transcode(Poco::Path(bnkxmlfile).setExtension("").makeDirectory().makeAbsolute().toString());
        if(!utils::DoCreateDirectory(sampledir))
            throw runtime_error( "ExportPresetBank(): Couldn't create sample directory ! " + sampledir);

        //Then export samples
        const size_t nbslots = smplptr->NbSlots();
        for( size_t cntsmpl = 0; cntsmpl < nbslots; ++cntsmpl )
        {
            auto * ptrinfo = smplptr->sampleInfo( cntsmpl );
            auto * ptrdata = smplptr->sample    ( cntsmpl );

            if( ptrinfo != nullptr && ptrdata != nullptr )
            {
                wave::smpl_chunk_content             smplchnk;
                stringstream                         sstrname;
                wave::smpl_chunk_content::SampleLoop loopinfo;
                DSESampleConvertionInfo              cvinf;

                //Set the loop data
                smplchnk.MIDIUnityNote_ = ptrinfo->rootkey;        //#FIXME: Most of the time, the correct root note is only stored in the preset, not the sample data!
                smplchnk.samplePeriod_  = (1 / ptrinfo->smplrate);

                if( hexanumbers )
                    sstrname <<"0x" <<hex <<uppercase << cntsmpl <<nouppercase;
                else
                    sstrname <<right <<setw(4) <<setfill('0') << cntsmpl;

                if( ptrinfo->smplfmt == eDSESmplFmt::ima_adpcm4 && noconvert )
                {
                    //We only allow mono samples
                    cvinf.loopbeg_ = (cvinf.loopbeg_ - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
                    cvinf.loopend_ = ::audio::ADPCMSzToPCM16Sz( ptrdata->size() );

                    loopinfo.start_ = cvinf.loopbeg_;
                    loopinfo.end_   = cvinf.loopend_;

                    //Add loopinfo!
                    sstrname<<".adpcm";
                    audio::DumpADPCM( Poco::Path(sampledir).append(sstrname.str()).makeFile().absolute().toString(), *ptrdata );
                }
                else if( ptrinfo->smplfmt == eDSESmplFmt::pcm8 && noconvert )
                {
                    wave::PCM8WaveFile outwave;

                    //We only allow mono samples
                    outwave.GetSamples().resize(1);
                    outwave.SampleRate( ptrinfo->smplrate );

                    auto backins = std::back_inserter(outwave.GetSamples().front());
                    //#TODO: Check wtf is going on here?
                    for( const auto asample : *ptrdata )
                        (*backins) = asample ^ 0x80; //Flip the first bit, to turn from 2's complement to offset binary(excess-K)

                    loopinfo.start_ = cvinf.loopbeg_;
                    loopinfo.end_   = cvinf.loopend_;

                    //Add loopinfo!
                    smplchnk.loops_.push_back( loopinfo );
                    outwave.AddRIFFChunk( smplchnk );
                    sstrname<<".wav";
                    outwave.WriteWaveFile( Poco::Path(sampledir).append(sstrname.str()).makeFile().absolute().toString() );
                }
                else
                {
                    wave::PCM16sWaveFile outwave;

                    //We only have mono samples
                    outwave.GetSamples().resize(1);
                    outwave.SampleRate( ptrinfo->smplrate );
                    auto & outsamp = outwave.GetSamples().front();

                    switch( ConvertDSESample(static_cast<uint16_t>(ptrinfo->smplfmt), ptrinfo->loopbeg, *ptrdata, cvinf, outsamp) )
                    {
                        case eDSESmplFmt::ima_adpcm4:
                        {
                            sstrname <<"_adpcm";
                            break;
                        }
                        case eDSESmplFmt::pcm8:
                        {
                            sstrname <<"_pcm8";
                            break;
                        }
                        case eDSESmplFmt::pcm16:
                        {
                            //sstrname <<"_pcm16";
                            break;
                        }
                        case eDSESmplFmt::ima_adpcm3:
                        {
                            clog <<"<!>- Sample# " <<cntsmpl <<" is an unsported PSG sample and was skipped!\n";
                            break;
                        }
                        case eDSESmplFmt::invalid:
                        {
                            clog <<"<!>- Sample# " <<cntsmpl <<" is in an unknown unsported format and was skipped!\n";
                        }
                    }

                    loopinfo.start_ = cvinf.loopbeg_;
                    loopinfo.end_   = cvinf.loopend_;

                    //Add loopinfo!
                    smplchnk.loops_.push_back( loopinfo );
                    outwave.AddRIFFChunk( smplchnk );
                    sstrname<<".wav";
                    outwave.WriteWaveFile( Poco::Path(sampledir).append(sstrname.str()).makeFile().absolute().toString() );
                }
            }
            else if( ptrinfo == nullptr || ptrdata == nullptr )
                clog<<"\n<!>- Sample + Sample info mismatch detected for index " <<cntsmpl <<"!\n";
        }
    }


};
