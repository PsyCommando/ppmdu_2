#ifndef AUDIO_UTIL_HPP
#define AUDIO_UTIL_HPP
/*
audioutil.hpp
2015/05/20
psycommando@gmail.com
Description: Code for the audioutil utility for Pokemon Mystery Dungeon : Explorers of Time/Darkness/Sky.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <dse/dse_conversion.hpp>
#include <utils/cmdline_util.hpp>
#include <string>
#include <vector>
#include <set>

namespace audioutil
{
    const std::string OPTION_SwdlPathSym = "swdlpath";
    const std::string OPTION_SmdlPathSym = "smdlpath";
    const std::string OPTION_MkCvInfo    = "makecvinfo";
    const std::string OPTION_BgmCntPath  = "bgmcntpath";
    const std::string OPTION_BgmBlobPath = "blobpath";
    const std::string OPTION_PMD2        = "pmd2";

    /// <summary>
    /// The type of file exported for music sequences
    /// </summary>
    enum struct eExportSequenceFormat
    {
        None,
        Midi_GS, //For now only midi supported
        XML, //Tracks will be exported to pure xml
    };

    /// <summary>
    /// The type of files exported for the samples.
    /// </summary>
    enum struct eExportSamplesFormat
    {
        None,
        XML, //XML + samples
        SF2, //Single Soundfont
        SF2_multiple, //Single sf2 per track
    };

    /// <summary>
    /// Contains data on what we're actually importing/exporting
    /// </summary>
    struct importExportTarget
    {
        //For single file/dir op
        std::set<std::string> paths; //Input path(s), the directory or file we want to operate on
        std::string outpath;        //If there is a specific one, the directory or file we want to put our result into

        //For batch op:
        std::string masterSwdlPath; //If there is one, the master swdl
        std::string swdlDirPath;    //If there is one, the directory containing all the individual songs swdls
        std::string sedlDirPath;    //If there is one, the directory containing all the individual songs sedls
        std::string smdlDirPath;    //If there is one, the directory containing all the individual songs smdls

        //PMD2 only
        std::string pmd2SwdDirPath; //Path to the swd dir
        std::string pmd2MeDirPath; //Path to the ME smdl + swdls
        std::string pmd2SystemDirPath; //Path to the system sedl + swdls

        inline bool hasDseSystemPaths()const noexcept
        {
            return !masterSwdlPath.empty() || !swdlDirPath.empty() || !sedlDirPath.empty() || !smdlDirPath.empty() || !pmd2SwdDirPath.empty() || !pmd2MeDirPath.empty() || !pmd2SystemDirPath.empty();
        }
    };

    /// <summary>
    /// The mode of operation for the program
    /// </summary>
    enum struct eOpMode
    {
        Invalid,

        Import,
        Export,

        MakeCvInfo,   //Outputs a blank Cvinfo file for all the swdl loaded !
        ListSWDLPrgm, //Outputs a list of the all the programs contained in the specified swdl/swdl dir and samples they uses

        //ExportSWDLBank, //Export the main bank, and takes the presets of all the swd files accompanying each smd files in the same folder to build a soundfont!
        //ExportSWDL,     //Export a SWDL file to a folder. The folder contains the wav, and anything else is turned into XML
        //ExportSMDL,     //Export a SMDL file as a midi
        //ExportSEDL,     //Export the SEDL as a midi and some XML

        //ExportBatchPairsAndBank,//Export a batch of SMDL files, along with their SWDL, and a main bank !
        //ExportBatchPairs,//Export a batch of SMDL files, along with their SWDL !
        //ExportBatchSMDL, //Export a batch of SMDL files only !
        //ExportBatchSWDL, //Export a batch of SWDL files only !

        //ExportPMD2,     //Export the entire content of the PMD2's "SOUND" folder

        //BuildSWDL,      //Build a SWDL from a folder. Must contain XML info file. If no preset data present builds a simple wav bank.(samples are imported in the slot corresponding to their file name)
        //BuildSMDL,      //Build a SMDL from a midi file and a similarly named XML file. XML file used to remap instruments from GM to the game's format, and to set the 2 unknown variables.
        //BuildSEDL,      //Build SEDL from a folder a midi file and XML.

        //BatchListSWDLPrgm,   //Outputs a list of the all the programs contained in the specified swdls and samples they uses
        //ListSWDLPrgm,   //Outputs a list of the all the programs contained in the specified swdl and samples they uses
        //MakeCvInfo,     //Outputs a blank Cvinfo file for all the swdl loaded !
    };

    //const unsigned int  OpFlag_MatchBlobName = 0b0000'0000'0000'0001;
    //const unsigned int  OpFlag_SampleBake    = 0b0000'0000'0000'0010;
    //const unsigned int  OpFlag_SampleToPCM16 = 0b0000'0000'0000'0100;
    //const unsigned int  OpFlag_NoFx          = 0b0000'0000'0000'1000;
    //const unsigned int  OpFlag_SampleIdAsHex = 0b0000'0000'0001'0000; //Use hexadecimal sample ids
    //const unsigned int  OpFlag_MakeCVInfo    = 0b0000'0000'0010'0000;
    //const unsigned int  OpFlag_ListPresets   = 0b0000'0000'0100'0000;
    //const unsigned int  OpFlag_LogVerbose    = 0b0000'0000'1000'0000;

    /// <summary>
    /// Main class for running the program
    /// </summary>
    class CAudioUtil : public utils::cmdl::CommandLineUtility
    {
    public:
        static CAudioUtil & GetInstance();

        // -- Overrides --
        //Those return their implementation specific arguments, options, and extra parameter lists.
        const std::vector<utils::cmdl::argumentparsing_t> & getArgumentsList()const;
        const std::vector<utils::cmdl::optionparsing_t>   & getOptionsList()const;
        const utils::cmdl::argumentparsing_t              * getExtraArg()const; //Returns nullptr if there is no extra arg. Extra args are args preceeded by a "+" character, usually used for handling files in batch !
       
        //For writing the title and readme!
        const std::string & getTitle()const;            //Name/Title of the program to put in the title!
        const std::string & getExeName()const;          //Name of the executable file!
        const std::string & getVersionString()const;    //Version number
        const std::string & getShortDescription()const; //Short description of what the program does for the header+title
        const std::string & getLongDescription()const;  //Long description of how the program works
        const std::string & getMiscSectionText()const;  //Text for copyrights, credits, thanks, misc..

        //Main method
        int Main(int argc, const char * argv[]);

    private:
        CAudioUtil();

        //Parse Arguments
        bool ParseInputPath  ( const std::string & path );
        bool ParseOutputPath ( const std::string & path );

        bool ShouldParseInputPath ( const std::vector<std::vector<std::string>> & options, 
                                    const std::deque<std::string>               & priorparams, 
                                    size_t                                        nblefttoparse );
        bool ShouldParseOutputPath( const std::vector<std::vector<std::string>> & options, 
                                    const std::deque<std::string>               & priorparams, 
                                    size_t                                        nblefttoparse );

        //Parsing Options
        bool ParseOptionExport(const std::vector<std::string>& optdata);
        bool ParseOptionImport(const std::vector<std::string>& optdata);

       // bool ParseOptionMBAT       ( const std::vector<std::string> & optdata ); //Export Master Bank And Tracks using the specified folder.

        //target directory info
        bool ParseOptionPMD2(const std::vector<std::string>& optdata); //Target the content of the PMD2 "SOUND" directory
        bool ParseOptionMBank      ( const std::vector<std::string> & optdata );
        bool ParseOptionSWDLPath   ( const std::vector<std::string> & optdata );
        bool ParseOptionSMDLPath   ( const std::vector<std::string> & optdata );

        //Special input types
        bool ParseOptionBGMCntPath(const std::vector<std::string>& optdata);
        bool ParseOptionBlobPath(const std::vector<std::string>& optdata);
        bool ParseOptionBlobMatchByName(const std::vector<std::string>& optdata);

        //Output type
        bool ParseOptionOutputSF2  ( const std::vector<std::string> & optdata );
        bool ParseOptionOutputXML  ( const std::vector<std::string> & optdata );
        bool ParseOptionForceTrackXML(const std::vector<std::string>& optdata);
        bool ParseOptionForceMidi(const std::vector<std::string>& optdata);

        //Processing options
        bool ParseOptionSampleBake( const std::vector<std::string> & optdata );
        bool ParseOptionConvertSamplesToPCM16(const std::vector<std::string>& optdata); 
        bool ParseOptionNoFX( const std::vector<std::string> & optdata ); //Ignore all applied LFO, vibrato, pitch bend, etc..
        bool ParseOptionGeneralMidi(const std::vector<std::string>& optdata); //Import/Export from/to general midi format
        bool ParseOptionForceLoops(const std::vector<std::string>& optdata); //loop exported midi tracks the given amount of times, and omit loop markers
        bool ParseOptionUseHexNumbers(const std::vector<std::string>& optdata); //Numbers exported samples by hexadecimal numbers instead of decimal

        //CVInfo stuff
        bool ParseOptionMakeCvinfo(const std::vector<std::string>& optdata);
        bool ParseOptionPathToCvInfo(const std::vector<std::string>& optdata); //Set the path to the cvinfo file to use

        //Logging options
        bool ParseOptionLog(const std::vector<std::string>& optdata); //Redirects clog to the file specified
        bool ParseOptionVerbose(const std::vector<std::string>& optdata); //Write more info to the log file!
        bool ParseOptionListPresets(const std::vector<std::string>& optdata);

        //Execution
        void DetermineOperation();

        /// <summary>
        /// Loads all the specified DSE paths if they were defined.
        /// </summary>
        /// <returns></returns>
        int LoadFromArgPaths();

        int  Execute           ();
        int ExecuteExport(importExportTarget& exportpaths, eExportSequenceFormat seqfmt, eExportSamplesFormat smplfmt);
        void ExportAFile(const std::string& path, eExportSequenceFormat seqfmt, eExportSamplesFormat smplfmt);
        int ExecuteImport();
        int  GatherArgs        ( int argc, const char * argv[] );

        //Returns whether we had any system path defined
        bool CheckAndUpdateDseSystemPaths();


        //Exec methods
        int ExportSWDL(const std::string& path, const std::vector<uint8_t>& fdata);
        int ExportSMDL(const std::string& path, const std::vector<uint8_t>& fdata);
        int ExportSEDL(const std::string& path, const std::vector<uint8_t>& fdata);

        int ExportBatchPairsAndBank();
        int ExportBatchPairs();

        int ImportPMD2Audio();
        int ExportPMD2Audio(); //Export completely the content of a PMD2 ROM's "SOUND" directory

        int ExportBatchSWDL();
        int ExportBatchSMDL();
        int BatchListSWDLPrgm( const std::string & SrcPath );
        int MakeCvinfo();       //Make a blank cvinfo from the swdl loaded!

        int BuildSWDL();
        int BuildSMDL();
        int BuildSEDL();

        //Constants
        static const std::string                                 Exe_Name;
        static const std::string                                 Title;
        static const std::string                                 Version;
        static const std::string                                 Short_Description;
        static const std::string                                 Long_Description;
        static const std::string                                 Misc_Text;
        static const std::vector<utils::cmdl::argumentparsing_t> Arguments_List;
        static const std::vector<utils::cmdl::optionparsing_t>   Options_List;
        static const int                                         MaxNbLoops;

        //Variables
        std::string m_inputPath;        //This is the input path that was parsed 
        std::string m_outputPath;       //This is the output path that was parsed
        std::string m_convinfopath;     //Path to a file containing conversion details for translating preset numbers and etc.
        eOpMode     m_operationMode;    //This holds what the program should do
        bool        m_bGM;              //Whether we export to general midi compatible format or not
        int         m_nbloops;          //The amount of times to loop a track, 0 if should use loop markers instead
        //bool        m_isPMD2;           //Whether we should treat the input path as the PMD2 ROM's root data folder
        bool        m_isListPresets;    //Whether the list preset mode was activated !
        bool        m_useHexaNumbers;   //Whether applicable exported filenames will contain hexadecimal numbers or decimal
        bool        m_bBakeSamples;     //Whether each preset's split should have a sample baked for it.
        bool        m_bUseLFOFx;        //Whether LFO FX are processed
        bool        m_bMakeCvinfo;      //Whether we should export a blank cvinfo file!
        bool        m_bConvertSamples;  //Whether the samples should be converted to pcm16 when exporting
        bool        m_bmatchbyname;     //Whether the containers inside a blob should be matched by internal name or simply matched by order in the blob.

        //Export type selection
        eExportSequenceFormat m_seqExportFmt;
        eExportSamplesFormat  m_smplsExportFmt;

        importExportTarget  m_targetPaths; //When changing this make sure to update CheckAndUpdateDseSystemPaths()
        std::string         m_pmd2root;
        std::string         m_mbankpath;
        std::string         m_swdlpath;
        std::string         m_smdlpath;
        std::string         m_sedlpath;
        std::string         m_bgmcntpath;
        std::string         m_bgmcntext;
        std::vector<std::string>        m_bgmblobpath;
        utils::cmdl::RAIIClogRedirect   m_redirectClog;

        DSE::DSELoader m_loader;
    };
};

#endif 