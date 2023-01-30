#ifndef PMD2_AUDIO_DATA_HPP
#define PMD2_AUDIO_DATA_HPP
/*
dse_conversion.hpp
2015/05/20
psycommando@gmail.com
Description: Containers and utilities for data parsed from PMD2's audio, and sequencer files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/dse_conversion_info.hpp>
#include <dse/containers/dse_music_sequence.hpp>
#include <dse/containers/dse_preset_db.hpp>
#include <dse/containers/dse_se_sequence.hpp>


#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <future>
#include <map>
#include <sstream>
#include <optional>

namespace sf2{ class SoundFont; class Instrument; };

namespace DSE
{
//====================================================================================================
//  Constants
//====================================================================================================
    const std::string SMDL_FileExtension = "smd";
    const std::string SWDL_FileExtension = "swd";
    const std::string SEDL_FileExtension = "sed";

//====================================================================================================
// Structs
//====================================================================================================

    /// <summary>
    /// Helper struct for gathering statistics from dse conversion.
    /// </summary>
    struct audiostats;


    /// <summary>
    /// Sample processing options commonly passed as arguments.
    /// </summary>
    struct sampleProcessingOptions 
    {
        bool bBakeSamples  = false;
        bool bLfoFxEnabled = true;
    };

    /// <summary>
    /// Sequence processing options commonly passed as arguments.
    /// </summary>
    struct sequenceProcessingOptions
    {
        int nbloops = 1;
        std::optional<DSE::SMDLConvInfoDB> cvinfo;
    };

//====================================================================================================
// Class
//====================================================================================================

    /// <summary>
    /// A transition container used when doing extra processing on the sample data from the game.
    /// Since the envelope, LFO, etc, are "baked" into the sample themselves, we need to change a lot about them.
    /// </summary>
    class ProcessedPresets
    {
    public:
        struct PresetEntry
        {
            PresetEntry() noexcept
            {}

            PresetEntry( PresetEntry && mv ) noexcept
                :prginf(std::move(mv.prginf)), splitsmplinf(std::move( mv.splitsmplinf )), splitsamples( std::move(mv.splitsamples) )
            {
            }

            DSE::ProgramInfo                    prginf;       //
            std::vector< DSE::WavInfo >         splitsmplinf; //Modified sample info for a split's sample.
            std::vector< std::vector<int16_t> > splitsamples; //Sample for each split of a preset.
        };

        typedef std::map< int16_t, PresetEntry >::iterator       iterator;
        typedef std::map< int16_t, PresetEntry >::const_iterator const_iterator;

        inline void AddEntry( PresetEntry && entry )
        {
            m_smpldata.emplace( std::make_pair( entry.prginf.id, std::move(entry) ) );
        }

        inline PresetEntry&       at(int16_t index) { return m_smpldata.at(index); }
        inline const PresetEntry& at(int16_t index)const { return m_smpldata.at(index); }

        inline bool contains(int16_t index)const { return m_smpldata.contains(index); }

        inline iterator       begin()      {return m_smpldata.begin();}
        inline const_iterator begin()const {return m_smpldata.begin();}

        inline iterator       end()        {return m_smpldata.end();}
        inline const_iterator end()const   {return m_smpldata.end();}

        inline iterator       find( int16_t presid ) { return m_smpldata.find(presid); }
        inline const_iterator find( int16_t presid )const { return m_smpldata.find(presid); }

        inline size_t         size()const  { return m_smpldata.size(); }

    private:
        std::map< int16_t, PresetEntry > m_smpldata;
    };


    /// <summary>
    /// Loads dse files to allow properly exporting/importing other dse files.
    /// </summary>
    class DSELoader
    {
    public:
        DSELoader();
        ~DSELoader();

        //-----------------------------
        // Loading Methods
        //-----------------------------
        /*
        * If the path is a file, loads a single swdl file. If its a directory load all swdl files in the directory non-recursively.
        * origfilename is the name of the file the raw bytes were pulled from. Its used for the convertion info stuff to keep track of original file names of songs.
        */
        void LoadSWDL(const std::string& path);
        void LoadSWDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {});

        /*
        * If the path is a file, loads a single smdl file. If its a directory load all smdl files in the directory non-recursively.
        * origfilename is the name of the file the raw bytes were pulled from. Its used for the convertion info stuff to keep track of original file names of songs.
        */
        void LoadSMDL(const std::string& path);
        void LoadSMDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {});

        /*
        * If the path is a file, loads a single sedl file. If its a directory load all sedl files in the directory non-recursively.
        * origfilename is the name of the file the raw bytes were pulled from. Its used for the convertion info stuff to keep track of original file names of songs.
        */
        void LoadSEDL(const std::string& path);
        void LoadSEDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {});

        /*
        * origfilename is the name of the file the raw bytes were pulled from. Its used for the convertion info stuff to keep track of original file names of songs.
        */
        void LoadSIR0Container(const std::string& filepath);
        void LoadSIR0Container(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {});

        /*
        */
        void LoadSIR0Containers(const std::string& dirpath, const std::string& fileext);

        /*
        */
        void LoadFromBlobFile(const std::string& blobpath);
        void LoadFromBlobFile(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend);

        //-----------------------------
        // Exporting Methods
        //-----------------------------
        SMDLConvInfoDB ExportSoundfont(const std::string& despath, sampleProcessingOptions options, bool bSingleSf2 = true);

        void ExportXMLPrograms(const std::string& destpath, bool bConvertSamples = false);
        void ExportXMLMusic(const std::string& destdirpath);
        void ExportMIDIs(const std::string& destdirpath, sequenceProcessingOptions options);

        void ImportChangesToGame(const std::string& swdlpath, std::string smdlpath = {}, std::string sedlpath = {});

        //-----------------------------
        // Import Methods
        //-----------------------------
        /*
        * Methods for loading exported data back in
        */
        DSE::PresetBank&           ImportBank          (const std::string& bnkxmlfile);
        DSE::MusicSequence&        ImportMusicSeq      (const std::string& midipath); //Also works with xml sequences
        DSE::SoundEffectSequences& ImportSoundEffectSeq(const std::string& seqdir);
        void                       ImportDirectory     (const std::string& pathdir); //Batch import everything contained in a directory. XML for banks, midi, SE sequences folders

        /// <summary>
        /// Import a file to DSE without knowing its type in advance.
        /// </summary>
        /// <param name="pathfile">The path to the file to try to import.</param>
        void ImportAFile(const std::string& pathfile);

    private:
        class DSELoaderImpl;
        std::unique_ptr<DSELoaderImpl> m_pimpl;
    };

//====================================================================================================
// Functions
//====================================================================================================

    //-------------------
    //  Sample Handling
    //-------------------

    /*
        ConvertDSESample
            Converts the given raw samples from a DSE compatible format to a signed pcm16 sample.

                * smplfmt    : The DSE sample type ID.
                * origloopbeg: The begining pos of the loop, straight from the WavInfo struct !
                * in_smpl    : The raw sample data as bytes.
                * out_cvinfo : The conversion info struct containing details on the resulting sample.
                * out_smpl   : The resulting signed pcm16 sample.

            Return the sample type.
    */
    eDSESmplFmt ConvertDSESample( int16_t                                smplfmt, 
                                  size_t                                 origloopbeg,
                                  const std::vector<uint8_t>           & in_smpl,
                                  DSESampleConvertionInfo              & out_cvinfo,
                                  std::vector<int16_t>                 & out_smpl );

    /*
        ConvertAndLoopDSESample
            Converts the given raw samples from a DSE compatible format to a signed pcm16 sample.

                * smplfmt    : The DSE sample type ID.
                * origloopbeg: The begining pos of the loop, straight from the WavInfo struct !
                * origlooplen: The lenght of the loop, straight from the WavInfo struct !
                * nbloops    : The nb of times to loop the sample.
                * in_smpl    : The raw sample data as bytes.
                * out_cvinfo : The conversion info struct containing details on the resulting sample.
                * out_smpl   : The resulting signed pcm16 sample.

            Return the sample type.
    */
    eDSESmplFmt ConvertAndLoopDSESample( int16_t                                smplfmt, 
                                         size_t                                 origloopbeg,
                                         size_t                                 origlooplen,
                                         size_t                                 nbloops,
                                         const std::vector<uint8_t>           & in_smpl,
                                         DSESampleConvertionInfo              & out_cvinfo,
                                         std::vector<int16_t>                 & out_smpl );


    /*
        ProcessDSESamples
            Takes the samples used in the programbank, convert them, and bake the envelope into them.

            * srcsmpl         : The samplebank containing all the raw samples used by the programbank.
            * prestoproc      : The programbank containing all the program whose samples needs to be processed.
            * desiredsmplrate : The desired sample rate in hertz to resample all samples to! (-1 means no resampling)
            * bakeenv         : Whether the envelopes should be baked into the samples.

            Returns a ProcessedPresets object, contining the new program data, along with the new samples.
    */
    DSE::ProcessedPresets ProcessDSESamples( const DSE::SampleBank  & srcsmpl, 
                                             const DSE::ProgramBank & prestoproc, 
                                             int                      desiredsmplrate = -1, 
                                             bool                     bakeenv         = true );

    //-------------------
    //  Audio Exporters
    //-------------------

    /*
        Export all sequences 
    */

    /*
        Exports a Sequence as MIDI and a corresponding SF2 file if the PresetBank's samplebank ptr is not null.
    */
    bool ExportSeqAndBank( const std::string & filename, const DSE::MusicSequence & seq, const DSE::PresetBank & bnk );
    bool ExportSeqAndBank( const std::string & filename, const std::pair<DSE::MusicSequence,DSE::PresetBank> & seqandbnk );

    /*
        Export the PresetBank to a directory as XML and WAV samples.
    */
    void ExportPresetBank( const std::string & bnkxmlfile, const DSE::PresetBank & bnk, bool samplesonly = true, bool hexanumbers = true, bool noconvert = true );

    /*
        To use the ExportSequence,
    */

    //Import

    DSE::PresetBank ImportPresetBank(const std::string& directory);

    /// <summary>
    /// Returns true if the directory contains importable sets of a midi/mml and xml sound bank
    /// </summary>
    /// <param name="dirpath"></param>
    /// <returns></returns>
    bool IsDirImportableSequences(const std::string& dirpath);

//=================================================================================================
//  XML
//=================================================================================================

    /// <summary>
    /// Returns whether the directory contains what's needed for assembling a SEDL.
    /// </summary>
    /// <param name="destdir"></param>
    /// <returns>Returns true if the directory has all it needs to be imported as a SEDL, and false if something is missing, or the directory is definitely not containing the data files needed for a SEDL.</returns>
    bool IsSESequenceXmlDir(const std::string& destdir);

    /// <summary>
    /// Returns whether the xml file contains data for a swdl bank.
    /// </summary>
    /// <param name="xmlfilepath">The path to the xml file to be anazlysed.</param>
    /// <returns>Returns true if the xml file contains data for a SWDL bank, false if it doesn't.</returns>
    bool IsXMLPresetBank   (const std::string& xmlfilepath);

    /// <summary>
    /// Returns whether the xml file is a full blown xml music sequence with track nodes, or not.
    /// </summary>
    /// <param name="xmlfilepath">The path to the xml file to be anazlysed.</param>
    /// <returns>Returns true if the xml contains a single music sequence with track nodes, and false if it doesn't contain tracks, or a sequence node.</returns>
    bool IsXMLMusicSequence(const std::string& xmlfilepath);

    /// <summary>
    /// Returns whether the xml file contains only sequence meta-data and no track event data.
    /// </summary>
    /// <param name="filepath">The path to the xml file to be analyzed.</param>
    /// <returns>True if the xml file contains only sequence metadata. false if it contains any track nodes, or is missing the metadata nodes.</returns>
    bool IsXMLMusicSequenceInfoOnly(const std::string& xmlfilepath);

    void PresetBankToXML          (const DSE::PresetBank&           srcbnk,   const std::string& bnkxmlfile);
    void SoundEffectSequencesToXML(const DSE::SoundEffectSequences& srcseq,   const std::string& destdir);
    void MusicSequenceToXML       (const DSE::MusicSequence&        seq,      const std::string& filepath);
    

    DSE::PresetBank           XMLToPresetBank     (const std::string& bnkxmlfile);
    DSE::SoundEffectSequences XMLToEffectSequences(const std::string& srcdir);
    DSE::MusicSequence        XMLToMusicSequence  (const std::string& filepath);

};


//Ostream operators


#endif 