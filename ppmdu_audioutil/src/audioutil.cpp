#include "audioutil.hpp"
#include <utils/utility.hpp>
#include <utils/cmdline_util.hpp>
#include <dse/dse_conversion.hpp>
#include <types/content_type_analyser.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <utils/library_wide.hpp>
#include <dse/dse_conversion_info.hpp>
#include <dse/dse_interpreter.hpp>
#include <dse/fmts/smdl.hpp>
#include <dse/fmts/swdl.hpp>
#include <dse/fmts/sedl.hpp>
#include <dse/bgm_container.hpp>
#include <ext_fmts/midi_fmtrule.hpp>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

using namespace ::utils::cmdl;
using namespace ::utils::io;
using namespace ::std;
using namespace ::DSE;

/*
#TODO:
    - Make a single function for loading DSE data.
*/

namespace audioutil
{
//=================================================================================================
//  CAudioUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CAudioUtil::Exe_Name            = "ppmd_audioutil.exe";
    const string CAudioUtil::Title               = "Music and sound import/export tool.";
#ifdef _DEBUG
    const string CAudioUtil::Version             = AUDIOUTIL_VER" debug";
#else
    const string CAudioUtil::Version             = AUDIOUTIL_VER;
#endif
    const string CAudioUtil::Short_Description   = "A utility to export and import music and sounds from the PMD2 games.";
    const string CAudioUtil::Long_Description    = 
        "#TODO";
    const string CAudioUtil::Misc_Text           = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically Creative Commons 0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

    const int   CAudioUtil::MaxNbLoops           = 1200;


//------------------------------------------------
//  Arguments Info
//------------------------------------------------
    /*
        Data for the automatic argument parser to work with.
    */
    const vector<argumentparsing_t> CAudioUtil::Arguments_List =
    {{
        //Input Path argument
        { 
            0,      //first arg
            true,  //false == mandatory
            true,   //guaranteed to appear in order
            "input path", 
            "Path to the file/directory to export, or the directory to assemble.",
#ifdef WIN32
            "\"c:/pmd_romdata/data.bin\"",
#elif __linux__
            "\"/pmd_romdata/data.bin\"",
#endif
            std::bind( &CAudioUtil::ParseInputPath,       &GetInstance(), placeholders::_1 ),
            std::bind( &CAudioUtil::ShouldParseInputPath, &GetInstance(), placeholders::_1, placeholders::_2, placeholders::_3 ),
        },
        //Output Path argument
        { 
            1,      //second arg
            true,   //true == optional
            true,   //guaranteed to appear in order
            "output path", 
            "Output path. The result of the operation will be placed, and named according to this path!",
#ifdef WIN32
            "\"c:/pmd_romdata/data\"",
#elif __linux__
            "\"/pmd_romdata/data\"",
#endif
            std::bind( &CAudioUtil::ParseOutputPath,       &GetInstance(), placeholders::_1 ),
            std::bind( &CAudioUtil::ShouldParseOutputPath, &GetInstance(), placeholders::_1, placeholders::_2, placeholders::_3 ),
        },
    }};


//------------------------------------------------
//  Options Info
//------------------------------------------------



    /*
        Information on all the switches / options to allow the automated parser 
        to parse them.
    */
    const vector<optionparsing_t> CAudioUtil::Options_List=
    {{
        //Tweak for General Midi format
        {
            "gm",
            0,
            "Specifying this will modify the MIDIs to fit as much as possible with the General Midi standard. "
            "Instruments remapped, and etc.. In short, the resulting MIDI will absolutely not "
            "be a 1:1 copy, and using the original samples with this MIDI won't sound good either.. "
            "WARNING: General MIDI offers 16 tracks, with one forced to play drums. But there isn't a track "
            "forced to play drums in the DSE system. So, some songs with just melody tracks simply can't be "
            "converted to GM and play correctly!!",
            "-gm",
            std::bind( &CAudioUtil::ParseOptionGeneralMidi, &GetInstance(), placeholders::_1 ),
        },

        //Force Loops
        {
            "fl",
            1,
            "This will export to the MIDI file the \"intro\" once, followed with the notes "
            "in-between the loop and end marker the specified number of times. Loop markers will also be omitted!",
            "-fl (nbofloops)",
            std::bind( &CAudioUtil::ParseOptionForceLoops, &GetInstance(), placeholders::_1 ),
        },

        //ExportPMD2
        {
            OPTION_PMD2,
            1,
            "Specifying this will tell the program the root of the extracted ROM's \"data\" directory. Which will automatically set all the needed dse paths."
            "The utility will export the audio content from the entire game's \"/SOUND\" directory, to a bunch of MIDIs and some soundfonts!",
            "-" + OPTION_PMD2 + " \"path/to/pmd2/rom/root\"",
            std::bind( &CAudioUtil::ParseOptionPMD2, &GetInstance(), placeholders::_1 ),
        },

        //SetPathToCvInfo
        {
            "cvinfo",
            1,
            "The cvinfo file is an XML file containing information on how to convert SMDL files to MIDI. "
            "It allows to change the original presets/bank to something else automatically."
            " It also allows to transpose notes by a certain preset, or change what key/preset "
            "is used when a certain key for a given original preset is played. See the example file for more details!",
            "-cvinfo \"path/to/conversion/info/file.xml\"",
            std::bind( &CAudioUtil::ParseOptionPathToCvInfo, &GetInstance(), placeholders::_1 ),
        },

        //MakeCvInfo
        {
            OPTION_MkCvInfo,
            0,
            "This will build a blank populated cvinfo.xml file from all the loaded swdl files specified using the " + OPTION_SwdlPathSym + " option."
            " or the " + OPTION_BgmCntPath + ". The first parameter specified on the commandline will be the name of the outputed cvinfo.xml file!",
            "-" + OPTION_MkCvInfo,
            std::bind( &CAudioUtil::ParseOptionMakeCvinfo, &GetInstance(), placeholders::_1 ),
        },

        //ForceMidi
        {
            "forcemidi",
            0,
            "Specifying this will force a MIDI only export, regardless of the parameters specified."
            "This is useful when loading bgm containers for example, and wanting to export them to MIDIs"
            "according or not to the rules specified in a cvinfo XML file.",
            "-forcemidi",
            std::bind( &CAudioUtil::ParseOptionForceMidi, &GetInstance(), placeholders::_1 ),
        },

        //Export Main Bank And Tracks
        //{
        //    "mbat",
        //    0,
        //    "Specifying this will tell the program that that input path is the root of the directory containing a main bank and its tracks."
        //    "The utility will export everything to MIDI files, and to a Sounfont file.",
        //    "-mbat",
        //    std::bind( &CAudioUtil::ParseOptionMBAT, &GetInstance(), placeholders::_1 ),
        //},


        //!#TODO : New Implementation for loading DSE stuff. Work in progress!
        //#################################################

        //Set Main Bank
        {
            "mbank",
            1,
            "Use this to specify the path to the main sample bank that the SMDL to export will use, if applicable!"
            "Is also used to specify where to put the assembled ",
            "-mbank \"SOUND/BGM/bgm.swd\"",
            std::bind( &CAudioUtil::ParseOptionMBank, &GetInstance(), placeholders::_1 ),
        },

        //Set SWDLPath
        {
            OPTION_SwdlPathSym,
            1,
            "Use this to specify the path to the folder where the SWDLs matching the SMDL to export are stored."
            "Is also used to specify where to put assembled DSE Preset during import.",
            "-swdlpath \"SOUND/BGM\"",
            std::bind( &CAudioUtil::ParseOptionSWDLPath, &GetInstance(), placeholders::_1 ),
        },

        //Set SMDLPath
        {
            OPTION_SmdlPathSym,
            1,
            "Use this to specify the path to the folder where the SMDLs to export are stored."
            "Is also used to specify where to put MIDI files converted to SMDL format.",
            "-smdlpath \"SOUND/BGM\"",
            std::bind( &CAudioUtil::ParseOptionSMDLPath, &GetInstance(), placeholders::_1 ),
        },

        //Set bgmcnt path
        {
            OPTION_BgmCntPath,
            2,
            "Use this to specify where to load bgm containers from. Aka, SMDL + SWDL wrapped together in a SIR0 wrapper."
            "The second argument is the file extension to look for, as most bgm containers have different file extensions!",
            "-bgmcntpath \"Path/to/BGMsDir\" \"bgm\"",
            std::bind( &CAudioUtil::ParseOptionBGMCntPath, &GetInstance(), placeholders::_1 ),
        },

        //Set BlobBGMPath
        {
            OPTION_BgmBlobPath,
            1,
            "Use this to specify the path to a blob of DSE containers to process. AKA a file that contains a bunch of"
            "SMDL, SWDL, SEDL containers one after the other with no particular structure."
            "You can also specify a folder or use several -blobpath to designate several files to be handled."
			"When using a folder, all files within that directory with no exception will be scanned and converted, so use with caution!",
            "-blobpath \"Path/to/blob/file.whatever\"",
            std::bind( &CAudioUtil::ParseOptionBlobPath, &GetInstance(), placeholders::_1 ),
        },

        //Export SWDL Preset + Sample List
        {
            "listpres",
            0,
            "Use this to make the program export a list of all the presets used by the SWDL contained within"
            "the swdlpath specified, or the input path if doing it for a single SWDL. "
            "If a swdlpath was specified, the path to the outputed list is the first argument."
            "If no swdlpath is specified the first argument is the path to the swdl to use, and"
            "optionally the second argument the path to the outputed list!",
            "-listpres",
            std::bind( &CAudioUtil::ParseOptionListPresets, &GetInstance(), placeholders::_1 ),
        },

        //HexNumbers
        {
            "hexnum",
            0,
            "If this is present, whenever possible, the program will export filenames with hexadecimal numbers in them!",
            "-hexnum",
            std::bind( &CAudioUtil::ParseOptionUseHexNumbers, &GetInstance(), placeholders::_1 ),
        },

        //sf2
        {
            "sf2",
            0,
            "Specifying this will export a swd soundbank to a soundfont if applicable.",
            "-sf2",
            std::bind( &CAudioUtil::ParseOptionOutputSF2, &GetInstance(), placeholders::_1 ),
        },

        //xml -> This is mainly useful when reimporting music tracks into the game!
        {
            "xml",
            0,
            "Specifying this will force outputing a swd soundbank to XML data and pcm16 samples(.wav files).",
            "-xml",
            std::bind( &CAudioUtil::ParseOptionOutputXML, &GetInstance(), placeholders::_1 ),
        },

        //force_trk_xml -> Forces tracks to be exported to pure xml
        {
            "force_trk_xml",
            0,
            "Specifying this will force tracks to be exported to pure xml instead of midi.",
            "-force_trk_xml",
            std::bind(&CAudioUtil::ParseOptionForceTrackXML, &GetInstance(), placeholders::_1),
        },

        //bake -> This enable sample enveloppe baking
        {
            "bake",
            0,
            "Specifying this will enable the pre-rendering of individual samples envelopes for every preset split. In short, it'll render longer version of all samples, and apply the volume envelope directly to them.",
            "-bake",
            std::bind( &CAudioUtil::ParseOptionSampleBake, &GetInstance(), placeholders::_1 ),
        },

        //nofx
        {
            "nofx",
            0,
            "Specifying this will disable the processing of LFO effects data.",
            "-nofx",
            std::bind( &CAudioUtil::ParseOptionNoFX, &GetInstance(), placeholders::_1 ),
        },

        //noconvert
        {
            "noconvert",
            0,
            "Specifying this will disable the convertion of samples to pcm16 when exporting a bank. They'll be exported in their original format.",
            "-noconvert",
            std::bind( &CAudioUtil::ParseOptionConvertSamplesToPCM16, &GetInstance(), placeholders::_1 ),
        },

        //match blob scanned containers by internal name
        {
            "nmatchoff",
            0,
            "Specifying this will make the program match pairs in a blob by order instead of by internal name! Needed for some games with strange naming conventions.",
            "-nmatchoff",
            std::bind(&CAudioUtil::ParseOptionBlobMatchByName, &GetInstance(), placeholders::_1),
        },

        //#################################################

        //Redirect clog to file
        {
            "log",
            1,
            "This option writes a log to the file specified as parameter.",
            "-log \"logfilename.txt\"",
            std::bind( &CAudioUtil::ParseOptionLog, &GetInstance(), placeholders::_1 ),
        },

        //Verbose output
        {
            "v",
            0,
            "This enables the writing of a lot more info to the logfile!",
            "-v",
            std::bind( &CAudioUtil::ParseOptionVerbose, &GetInstance(), placeholders::_1 ),
        },

        //Import Mode
        {
            "i",
            0,
            "Import files and directories to the DSE format.",
            "-i",
            std::bind(&CAudioUtil::ParseOptionImport, &GetInstance(), placeholders::_1),
        },

        //Export Mode
        {
            "e",
            0,
            "Export DSE files to other formats.",
            "-e",
            std::bind(&CAudioUtil::ParseOptionExport, &GetInstance(), placeholders::_1),
        },
    }};


//------------------------------------------------
// Misc Methods
//------------------------------------------------

    CAudioUtil & CAudioUtil::GetInstance()
    {
        static CAudioUtil s_util;
        return s_util;
    }

    CAudioUtil::CAudioUtil()
        :CommandLineUtility()
    {
        m_operationMode   = eOpMode::Invalid;
        m_bGM             = false;
        m_isListPresets   = false;
        m_useHexaNumbers  = false;
        m_bBakeSamples    = false;
        m_bUseLFOFx       = true;
        m_bMakeCvinfo     = false;
        m_bConvertSamples = false;
        m_bmatchbyname    = true;
        m_nbloops         = 0;
        m_seqExportFmt    = eExportSequenceFormat::Midi_GS;
        m_smplsExportFmt  = eExportSamplesFormat::SF2;
    }

    const vector<argumentparsing_t> & CAudioUtil::getArgumentsList   ()const { return Arguments_List;    }
    const vector<optionparsing_t>   & CAudioUtil::getOptionsList     ()const { return Options_List;      }
    const argumentparsing_t         * CAudioUtil::getExtraArg        ()const { return nullptr;           } //No extra args
    const string                    & CAudioUtil::getTitle           ()const { return Title;             }
    const string                    & CAudioUtil::getExeName         ()const { return Exe_Name;          }
    const string                    & CAudioUtil::getVersionString   ()const { return Version;           }
    const string                    & CAudioUtil::getShortDescription()const { return Short_Description; }
    const string                    & CAudioUtil::getLongDescription ()const { return Long_Description;  }
    const string                    & CAudioUtil::getMiscSectionText ()const { return Misc_Text;         }

//------------------------------------------------
//  Utility
//------------------------------------------------




//------------------------------------------------
//  Parse Args
//------------------------------------------------
    /*
        When we have some specific options specified, we don't need the input path argument!
    */
    bool CAudioUtil::ShouldParseInputPath( const vector<vector<string>> & options,  
                                           const deque<string>          & priorparams, 
                                           size_t                         nblefttoparse )
    {
        auto itfoundinswitch = std::find_if( options.begin(), 
                                             options.end(), 
                                             [](const vector<string>& curopt)
                                             { 
                                                return (curopt.front() == OPTION_SwdlPathSym) ||
                                                       (curopt.front() == OPTION_SmdlPathSym) ||
                                                       (curopt.front() == OPTION_BgmCntPath)  ||
                                                       (curopt.front() == OPTION_BgmBlobPath) ||
                                                       (curopt.front() == OPTION_MkCvInfo); 

                                             } );

        //If we have an input path option, we do not need the input path parameter!
        return ( itfoundinswitch == options.end() );
    }

    bool CAudioUtil::ShouldParseOutputPath( const vector<vector<string>> & options, 
                                            const deque<string>          & priorparams, 
                                            size_t                         nblefttoparse )
    {
        auto itfoundinswitch = std::find_if( options.begin(), 
                                             options.end(), 
                                             [](const vector<string>& curopt)
                                             { 
                                                return ( curopt.front() == OPTION_SwdlPathSym ) ||
                                                       ( curopt.front() == OPTION_SmdlPathSym ) ||
                                                       ( curopt.front() == OPTION_BgmCntPath )  ||
                                                       ( curopt.front() == OPTION_BgmBlobPath ) ||
                                                       ( curopt.front() == OPTION_MkCvInfo ); 
                                             } );

        //If we have an input path option or parameter, and we still have parameters left to parse, this param should be parsed!
        return ( (itfoundinswitch != options.end()) || (priorparams.size() >= 1) ) 
                && (nblefttoparse > 0);
    }
    
    bool CAudioUtil::ParseInputPath(const string& path)
    {
        Poco::Path inputfile(path);

        //check if path exists
        if ((inputfile.isFile() || inputfile.isDirectory()))
        {
            m_inputPath = path;
            return true;
        }

        return false;
    }

    bool CAudioUtil::ParseOutputPath( const string & path )
    {
        Poco::Path outpath(path);

        if( outpath.isFile() || outpath.isDirectory() )
        {
            m_outputPath = path;
            return true;
        }
        return false;
    }


//------------------------------------------------
//  Parse Options
//------------------------------------------------

    bool CAudioUtil::ParseOptionExport(const std::vector<std::string>& optdata)
    {
        m_operationMode = eOpMode::Export;
        return true;
    }

    bool CAudioUtil::ParseOptionImport(const std::vector<std::string>& optdata)
    {
        m_operationMode = eOpMode::Import;
        return true;
    }

    bool CAudioUtil::ParseOptionGeneralMidi( const std::vector<std::string> & optdata )
    {
        m_bGM = true;
        return true;
    }

    bool CAudioUtil::ParseOptionForceLoops ( const std::vector<std::string> & optdata )
    {
        stringstream conv;
        conv << optdata[1];
        conv >> m_nbloops;

        if( m_nbloops >= 0 && m_nbloops < MaxNbLoops )
            return true;
        else
        {
            if( m_nbloops > MaxNbLoops )
                cerr <<"Too many loops requested! " <<m_nbloops <<" loops is a little too much to handle !! Use a number below " <<MaxNbLoops <<" please !\n";
            return false;
        }
    }

    bool CAudioUtil::ParseOptionPMD2( const std::vector<std::string> & optdata )
    {
        Poco::File root(Poco::Path(optdata[1]).makeAbsolute());
        if (root.exists() && root.isDirectory())
            m_pmd2root = optdata[1];
        else
        {
            cerr << "<!>- ERROR: Path to PMD2 rom root \"" <<Poco::Path::transcode(root.path()) <<"\" doesn't exist!\n";
            return false;
        }
        return true;
    }

    //bool CAudioUtil::ParseOptionMBAT( const std::vector<std::string> & optdata )
    //{
    //    m_operationMode = eOpMode::ExportSWDLBank;
    //    return true;
    //}

    bool CAudioUtil::ParseOptionLog( const std::vector<std::string> & optdata )
    {
        Poco::Path outpath(optdata[1]);
        if( outpath.isFile() )
        {
            Poco::File OutputDir( outpath.parent().makeAbsolute() );
            if( !OutputDir.exists() )
            {
                if( !OutputDir.createDirectory() )
                {
                    throw runtime_error( "Couldn't create output directory for log file!");
                }
            }
                

            m_redirectClog.Redirect(optdata[1]);
            utils::LibWide().isLogOn(true);
            return true;
        }
        else
        {
            cerr << "<!>- ERROR: Invalid path to log file specified! Path is not a file!\n";
            return false;
        }
    }

    bool CAudioUtil::ParseOptionVerbose( const std::vector<std::string> & optdata )
    {
        utils::LibWide().setVerbose(true);
        return true;
    }

    bool CAudioUtil::ParseOptionPathToCvInfo( const std::vector<std::string> & optdata )
    {
        Poco::File cvinfof( Poco::Path( optdata[1] ).makeAbsolute() );
        
        if( cvinfof.exists() && cvinfof.isFile() )
            m_convinfopath = optdata[1];
        else
        {
            cerr << "<!>- ERROR: Invalid path to cvinfo file specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionMBank( const std::vector<std::string> & optdata )
    {
        Poco::File bank( Poco::Path( optdata[1] ).makeAbsolute() );
        
        if( bank.exists() && bank.isFile() )
            m_mbankpath = optdata[1];
        else
        {
            cerr << "<!>- ERROR: Invalid path to main swdl bank specified !\n";
            return false;
        }
        return true;
    }
    
    bool CAudioUtil::ParseOptionSWDLPath( const std::vector<std::string> & optdata )
    {
        Poco::File swdldir( Poco::Path( optdata[1] ).makeAbsolute() );
        
        if( swdldir.exists() && swdldir.isDirectory() )
            m_swdlpath = optdata[1];
        else
        {
            cerr << "<!>- ERROR: Invalid path to swdl directory specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionSMDLPath( const std::vector<std::string> & optdata )
    {
        Poco::File smdldir( Poco::Path( optdata[1] ).makeAbsolute() );
        
        if( smdldir.exists() && smdldir.isDirectory() )
            m_smdlpath = optdata[1];
        else
        {
            cerr << "<!>- ERROR: Invalid path to smdl directory specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionListPresets( const std::vector<std::string> & optdata )
    {
        m_isListPresets = true;
        return true;
    }

    bool CAudioUtil::ParseOptionUseHexNumbers( const std::vector<std::string> & optdata )
    {
        m_useHexaNumbers = true;
        return true;
    }

    bool CAudioUtil::ParseOptionSampleBake( const std::vector<std::string> & optdata )
    {
        m_bBakeSamples = true;
        return true;
    }

    bool CAudioUtil::ParseOptionNoFX( const std::vector<std::string> & optdata )
    {
        m_bUseLFOFx = false;
        return true;
    }

    bool CAudioUtil::ParseOptionMakeCvinfo( const std::vector<std::string> & optdata )
    {
        m_operationMode = eOpMode::MakeCvInfo;
        //m_bMakeCvinfo = true;
        return true; 
    }

    bool CAudioUtil::ParseOptionBGMCntPath( const std::vector<std::string> & optdata )
    {
        Poco::File bgmcntpath( Poco::Path( optdata[1] ).makeAbsolute() );

        if( bgmcntpath.exists() && bgmcntpath.isDirectory() )
        {
            m_bgmcntpath = optdata[1];
            m_bgmcntext  = optdata[2];
        }
        else
        {
            cerr << "<!>- ERROR: Invalid path to bgm container directory specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionBlobPath( const std::vector<std::string> & optdata )
    {
        Poco::File bgmblobpath( Poco::Path( optdata[1] ).makeAbsolute() );

        if( bgmblobpath.exists() )
        {
            m_bgmblobpath.push_back(optdata[1]);
        }
        else
        {
            cerr << "<!>- ERROR: Invalid path to bgm blob file specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionConvertSamplesToPCM16( const std::vector<std::string> & optdata )
    {
        return (m_bConvertSamples = true);
    }

    bool CAudioUtil::ParseOptionBlobMatchByName(const std::vector<std::string>& optdata)
    {
        m_bmatchbyname = false;
        return true;
    }

//Output type
    bool CAudioUtil::ParseOptionOutputSF2(const std::vector<std::string>& optdata)
    {
        m_smplsExportFmt = eExportSamplesFormat::SF2;
        return true;
    }

    bool CAudioUtil::ParseOptionOutputXML(const std::vector<std::string>& optdata)
    {
        m_smplsExportFmt = eExportSamplesFormat::XML;
        return true;
    }

    bool CAudioUtil::ParseOptionForceTrackXML(const std::vector<std::string>& optdata)
    {
        m_seqExportFmt = eExportSequenceFormat::XML;
        return true;
    }

    bool CAudioUtil::ParseOptionForceMidi(const std::vector<std::string>& optdata)
    {
        m_seqExportFmt   = eExportSequenceFormat::Midi_GS;
        m_smplsExportFmt = eExportSamplesFormat::None;
        return true;
    }

//------------------------------------------------
//  Program Setup and Execution
//------------------------------------------------
    int CAudioUtil::GatherArgs( int argc, const char * argv[] )
    {
        int returnval = 0;
        //Parse arguments and options
        try
        {
            if(SetArguments(argc,argv))
                DetermineOperation();
            returnval = 0;
        }
        catch(const Poco::Exception & pex)
        {
            cerr <<"\n<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<"\n"
                 <<"You can print the readme by calling the program with no arguments!\n" <<endl;
            returnval = pex.code();
        }
        catch(const exception & e)
        {
            cerr <<"\n<!>-Exception: " << e.what() <<"\n"
                 <<"You can print the readme by calling the program with no arguments!\n" <<endl;
            returnval = -3;
        }
        return returnval;
    }

    bool CAudioUtil::CheckAndUpdateDseSystemPaths()
    {
        //Fill up system paths if specified
        bool bdseSystemDirSpecified = false; //Whether we defined any dir paths to the dse files

        //When PMD2 is specified, we just auto-set the target paths
        if (!m_pmd2root.empty())
        {
            const Poco::Path pmd2root(m_pmd2root);
            const Poco::Path soundroot(Poco::Path(pmd2root).append(pmd2::DirName_SOUND));
            if (!Poco::File(pmd2root).exists())
            {
                stringstream sstr;
                sstr << "PMD2 rom root path \"" << pmd2root.toString() << "\" doesn't exist!\n";
                throw runtime_error(sstr.str());
            }
            if (!Poco::File(soundroot).exists())
            {
                stringstream sstr;
                sstr << "PMD2 SOUND directory \"" << pmd2root.toString() << "\" doesn't exist!\n";
                throw runtime_error(sstr.str());
            }

            const Poco::Path PathBGM = Poco::Path(soundroot).append("BGM").makeDirectory();
            m_targetPaths.masterSwdlPath = Poco::Path::transcode(Poco::Path(PathBGM).setFileName("bgm").setExtension(DSE::SWDL_FileExtension).toString());
            m_targetPaths.smdlDirPath = Poco::Path::transcode(PathBGM.toString());
            m_targetPaths.swdlDirPath = Poco::Path::transcode(PathBGM.toString());
            m_targetPaths.sedlDirPath = Poco::Path::transcode(Poco::Path(soundroot).append("SE").toString());
            //PMD2 specifics
            m_targetPaths.pmd2SwdDirPath = Poco::Path::transcode(Poco::Path(soundroot).append("SWD").toString());
            m_targetPaths.pmd2MeDirPath = Poco::Path::transcode(Poco::Path(soundroot).append("ME").toString());
            m_targetPaths.pmd2SystemDirPath = Poco::Path::transcode(Poco::Path(soundroot).append("SYSTEM").toString());
            bdseSystemDirSpecified = true;
        }
        else
        {
            //Populate paths to dse files
            if (!m_mbankpath.empty())
            {
                m_targetPaths.masterSwdlPath = Poco::Path::transcode(Poco::Path(m_mbankpath).absolute().toString());
                bdseSystemDirSpecified = true;
            }
            if (!m_smdlpath.empty())
            {
                m_targetPaths.smdlDirPath = Poco::Path::transcode(Poco::Path(m_smdlpath).absolute().toString());
                bdseSystemDirSpecified = true;
            }
            if (!m_swdlpath.empty())
            {
                m_targetPaths.swdlDirPath = Poco::Path::transcode(Poco::Path(m_swdlpath).absolute().toString());
                bdseSystemDirSpecified = true;
            }
            if (!m_sedlpath.empty())
            {
                m_targetPaths.sedlDirPath = Poco::Path::transcode(Poco::Path(m_sedlpath).absolute().toString());
                bdseSystemDirSpecified = true;
            }
        }
        return bdseSystemDirSpecified;
    }

    void CAudioUtil::DetermineOperation()
    {
        using namespace pmd2::filetypes;
        using namespace filetypes;
        const Poco::Path inpath(m_inputPath);
        const Poco::File infile(inpath.absolute());
        const Poco::Path outpath(m_outputPath);

        if( !m_outputPath.empty() && !Poco::File( Poco::Path( m_outputPath ).makeAbsolute().parent() ).exists() )
            throw runtime_error("Specified output path does not exists!");

        //No mode forced, so try to guess
        if (m_operationMode == eOpMode::Invalid)
        {
            const std::string extension = inpath.getExtension();
            if (extension == DSE::SWDL_FileExtension || extension == DSE::SMDL_FileExtension || extension == DSE::SEDL_FileExtension)
                m_operationMode = eOpMode::Export;
            else if (extension == "mid" || extension == "xml" || inpath.isDirectory())
                m_operationMode = eOpMode::Import;
        }

        //Fill up system paths if specified
        bool bdseSystemDirSpecified = CheckAndUpdateDseSystemPaths(); // Whether we defined any dir paths to the dse files

        //
        // Setup in/out paths for our operation
        //
        if(m_operationMode == eOpMode::Import)
        {
            cout << "<*>- Import Mode\n";

            //Set input
            m_targetPaths.paths.insert(Poco::Path::transcode(infile.path()));

            //Set output
            if (!bdseSystemDirSpecified)
            {
                //If we didn't sepcify any of the dse locations, fallback to single dir/file mode
                if(m_outputPath.empty())
                    m_targetPaths.outpath = Poco::Path::transcode(inpath.absolute().makeParent().toString());
                else
                    m_targetPaths.outpath = Poco::Path::transcode(outpath.absolute().toString());
            }

            //Report unused paramters on import
            if (!m_bgmblobpath.empty())
                cerr << "<!>- Blob path parameter is unsupported on import!";
            if (!m_bgmcntpath.empty())
                cerr << "<!>- Bgm container path parameter is unsupported on import!!";

        }
        else if (m_operationMode == eOpMode::Export)
        {
            cout << "<*>- Export Mode\n";

            //If we didn't sepcify any of the dse locations, fallback to single dir/file mode
            if (!bdseSystemDirSpecified)
            {
                m_targetPaths.paths.insert(Poco::Path::transcode(infile.path()));
                m_targetPaths.outpath = Poco::Path::transcode(outpath.absolute().makeParent().toString());
            }
            else
                m_targetPaths.outpath = Poco::Path::transcode(infile.path());  //If we have system paths specified, we use the input path as output

            //Additionally, in export mode, we can specify bgm container or blob path
            if (!m_bgmblobpath.empty())
            {
                cout << "<*>- Adding blob paths:\n";
                for (const string& p : m_bgmblobpath)
                {
                    if (!Poco::File(p).exists())
                    {
                        stringstream sstr;
                        sstr << "File \"" << p << "\" does not exist!";
                        throw runtime_error(sstr.str());
                    }
                    else
                    {
                        cout << "\t\"" << Poco::Path::transcode(Poco::Path(p).absolute().toString()) << "\".\n";
                        m_targetPaths.paths.insert(p);
                    }
                }
            }
            if (!m_bgmcntpath.empty())
            {
                if (!Poco::File(m_bgmcntpath).exists())
                {
                    stringstream sstr;
                    sstr << "File \"" << m_bgmcntpath << "\" does not exist!";
                    throw runtime_error(sstr.str());
                }
            }
        }
    }

    int CAudioUtil::LoadFromArgPaths()
    {
        if (m_operationMode == eOpMode::Import)
            return 0; //Don't load anything in import mode
        //
        //Fill up the batch loader
        //
        if (!m_bgmcntpath.empty())
        {
            cout << "<*>- Loading SIR0 Container(s)...\n";
            Poco::File cntpath(m_bgmcntpath);
            if (cntpath.isDirectory())
            {
                m_loader.LoadSIR0Containers(Poco::Path::transcode(Poco::Path(m_bgmcntpath).absolute().toString()), m_bgmcntext);
            }
            else if (cntpath.isFile())
            {
                m_loader.LoadSIR0Container(Poco::Path::transcode(Poco::Path(m_bgmcntpath).absolute().toString()));
            }
            else
                throw std::runtime_error("Error parsing path to sir0 container.");
        }

        if (!m_bgmblobpath.empty())
        {
            cout << "<*>- Scanning and Loading Blob Container(s)...\n";
            for (const string& path : m_bgmblobpath)
            {
                m_loader.LoadFromBlobFile(Poco::Path::transcode(Poco::Path(path).absolute().toString()));
            }
        }

        if (m_targetPaths.hasDseSystemPaths())
        {
            cout << "<*>- Loading DSE System Directories...\n";
            //Load specified system files
            if (!m_targetPaths.masterSwdlPath.empty())
            {
                m_loader.LoadSWDL(m_targetPaths.masterSwdlPath);
                cout << "<*>- Loaded Master Bank!\n";
            }

            if (!m_targetPaths.smdlDirPath.empty())
            {
                m_loader.LoadSMDL(m_targetPaths.smdlDirPath);
                cout << "<*>- Loaded SMDL Dir!\n";
            }
            if (!m_targetPaths.swdlDirPath.empty())
            {
                m_loader.LoadSWDL(m_targetPaths.smdlDirPath);
                cout << "<*>- Loaded SWDL Dir!\n";
            }

            //TODO: load the extra paths for ME and etc
            //if (!m_targetPaths.sedlDirPath.empty())
            //    m_loader.LoadSingleSEDLs(m_targetPaths.sedlDirPath);

            //if (!m_targetPaths.pmd2MeDirPath.empty())
            //{
            //    m_pmd2MeLoader.LoadMasterBank(Poco::Path::transcode(Poco::Path(m_targetPaths.pmd2MeDirPath).setFileName("me").setExtension(DSE::SWDL_FileExtension).toString()));
            //    m_pmd2MeLoader.LoadSingleSMDLs(m_targetPaths.pmd2MeDirPath);
            //}
        }
        return 0;
    }

    int CAudioUtil::Execute()
    {
        int returnval = -1;
        utils::MrChronometer chronoexecuter("Total time elapsed");
        try
        {
            //Always need to load these anyways
            LoadFromArgPaths();

            //Then run import/export from what data we got
            switch (m_operationMode)
            {
            case eOpMode::Import:
                ExecuteImport();
                break;
            case eOpMode::Export:
                ExecuteExport(m_targetPaths, m_seqExportFmt, m_smplsExportFmt);
                break;
            };
        }
        catch(const exception &e)
        {
            cerr << "\n<!>- ";
            utils::PrintNestedExceptions(cerr, e);
        }
        return returnval;
    }

    void CAudioUtil::ExportAFile(const std::string & path, eExportSequenceFormat seqfmt, eExportSamplesFormat smplfmt)
    {
        Poco::File curfile(path);
        if (curfile.isFile())
        {
            //Handle single file operations
            vector<uint8_t> fdata = utils::io::ReadFileToByteVector(path);
            auto            cntty = filetypes::DetermineCntTy(fdata.begin(), fdata.end(), Poco::Path(path).getExtension());

            if (cntty._type == static_cast<unsigned int>(filetypes::CnTy_SMDL))
                ExportSMDL(path, fdata);
            else if (cntty._type == static_cast<unsigned int>(filetypes::CnTy_SWDL))
                ExportSWDL(path, fdata);
            else if (cntty._type == static_cast<unsigned int>(filetypes::CnTy_SEDL))
                ExportSEDL(path, fdata);
            else if (cntty._type == static_cast<unsigned int>(filetypes::CnTy_MIDI))
                assert(false); //TODO: This should import a midi to the dse format more or less as-is.
            else
                clog << "Unknown file type for path:\""  << path << "\", skipping..";
        }
        else if (curfile.isDirectory())
        {
            //If directory we assume we wanna export everything in it
            Poco::DirectoryIterator dirit(curfile);
            Poco::DirectoryIterator dirend;
            while (dirit != dirend)
            {
                ExportAFile(dirit.path().absolute().toString(), seqfmt, smplfmt);
                ++dirit;
            }
        }
    }

    int CAudioUtil::ExecuteExport(importExportTarget & exportpaths, eExportSequenceFormat seqfmt, eExportSamplesFormat smplfmt)
    {
        //We dump the presets for whatever swdl we got loaded in this case
        if (m_bMakeCvinfo)
            return MakeCvinfo();

        //Setup output directory
        Poco::File outpath(exportpaths.outpath);
        outpath.createDirectories();

        //If a specific input file was specified
        if (m_targetPaths.paths.empty() || (!m_bgmblobpath.empty()) || (!m_bgmcntpath.empty()))
        {
            if (seqfmt == eExportSequenceFormat::Midi_GS)
            {
                //Export everything in the dse paths that were specified
                if (smplfmt == eExportSamplesFormat::SF2)
                {
                    cout << "<*>- Exporting soundfont and MIDI files to " << exportpaths.outpath << "..\n";
                    auto cvinf = m_loader.ExportSoundfont(exportpaths.outpath, DSE::sampleProcessingOptions{ m_bBakeSamples , m_bUseLFOFx}, false);
                    m_loader.ExportMIDIs(exportpaths.outpath, DSE::sequenceProcessingOptions{ m_nbloops, cvinf });
                }
                //else if (smplfmt == eExportSamplesFormat::SF2_multiple)
                //{
                //    cout << "<*>- Exporting soundfont and MIDI files to " << exportpaths.outpath << "..\n";
                //    m_loader.ExportSoundfontAndMIDIs(exportpaths.outpath, m_nbloops, m_bBakeSamples, true);
                //}
                else if (smplfmt == eExportSamplesFormat::XML)
                {
                    cout << "<*>- Exporting sample + instruments presets data and MIDI files to " << exportpaths.outpath << "..\n";
                    m_loader.ExportXMLPrograms(exportpaths.outpath, m_bConvertSamples);
                    m_loader.ExportMIDIs(exportpaths.outpath, DSE::sequenceProcessingOptions{ m_nbloops });
                }
                else if (smplfmt == eExportSamplesFormat::None)
                {
                    cout << "<*>- Exporting MIDI files only to " << exportpaths.outpath << "..\n";
                    m_loader.ExportMIDIs(exportpaths.outpath, DSE::sequenceProcessingOptions{ m_nbloops, DSE::SMDLConvInfoDB(m_convinfopath) });
                }
                else
                    assert(false);
            }
            else if (seqfmt == eExportSequenceFormat::XML)
            {
                if (smplfmt == eExportSamplesFormat::XML)
                {
                    cout << "<*>- Exporting sample, instruments presets data, and music sequence XML files to " << exportpaths.outpath << "..\n";
                    m_loader.ExportXMLPrograms(exportpaths.outpath, m_bConvertSamples);
                }
                else if (smplfmt == eExportSamplesFormat::None)
                {
                    cout << "<*>- Exporting music sequence XML files to " << exportpaths.outpath << "..\n";
                }
                else if (smplfmt == eExportSamplesFormat::SF2)
                    throw std::runtime_error("Imcompatible options selected. When exporting music tracks to xml, soundfont export is disabled!"s);
                else
                    assert(false);

                m_loader.ExportXMLMusic(exportpaths.outpath);
            }
            else if(seqfmt == eExportSequenceFormat::None)
            {
                //Export everything in the dse paths that were specified
                if (smplfmt == eExportSamplesFormat::SF2)
                {
                    cout << "<*>- Exporting soundfont and MIDI files to " << exportpaths.outpath << "..\n";
                    m_loader.ExportSoundfont(exportpaths.outpath, sampleProcessingOptions{ m_bBakeSamples, m_bUseLFOFx });
                }
                //else if (smplfmt == eExportSamplesFormat::SF2_multiple)
                //{
                //    cout << "Exporting soundfont and MIDI files to " << exportpaths.outpath << "..\n";
                //    if (m_bBakeSamples)
                //        m_loader.ExportSoundfontBakedSamples(exportpaths.outpath);
                //    else
                //        m_loader.ExportSoundfont(exportpaths.outpath);
                //}
                else if (smplfmt == eExportSamplesFormat::XML)
                {
                    cout << "<*>- Exporting sample + instruments presets data and MIDI files to " << exportpaths.outpath << "..\n";
                    m_loader.ExportXMLPrograms(exportpaths.outpath, m_bConvertSamples);
                }
                else if (smplfmt == eExportSamplesFormat::None)
                    cout << "<!>- Exporting nothing???";
                else
                    assert(false);
            }
        }
        else
        {
            //Process all paths
            for (const string& path : m_targetPaths.paths)
            {
                ExportAFile(path, seqfmt, smplfmt);
            }
        }

        return 0;
    }

    int CAudioUtil::ExecuteImport()
    {
        if (m_targetPaths.paths.empty())
            throw std::runtime_error("Nothing to import was specified!");

        //Process all paths
        for (const string& path : m_targetPaths.paths)
        {
            Poco::File curfile(path);
            if (curfile.isFile())
            {
                const std::string fext = Poco::Path(path).getExtension();
                //Handle single file operations
                if (fext == "xml")
                {
                    //A single xml file is usually a bank
                    if (IsXMLPresetBank(path))
                        m_loader.ImportBank(path);
                    else if (IsXMLMusicSequence(path))
                        m_loader.ImportMusicSeq(path);
                    else
                        throw std::runtime_error("Unknown XML file:\""s + path + "\""s);
                }
                else if (fext == "mid")
                {
                    //If we find some midi file, import them as sequences
                    m_loader.ImportMusicSeq(path);
                }
                else
                    throw std::runtime_error("Unknown file type for path:\""s + path + "\""s);
            }
            else if (curfile.isDirectory())
            {
                //Try to batch import everything in there
                m_loader.ImportDirectory(path);
            }
        }

        //Next convert into dse files what we loaded
        const std::string outputswdl = m_targetPaths.swdlDirPath.empty() ? m_targetPaths.outpath : m_targetPaths.swdlDirPath;
        const std::string outputsmdl = m_targetPaths.smdlDirPath.empty() ? m_targetPaths.outpath : m_targetPaths.smdlDirPath;
        m_loader.ImportChangesToGame(outputswdl, outputsmdl);
        return 0;
    }

//--------------------------------------------
//  Main Methods
//--------------------------------------------
    int CAudioUtil::Main(int argc, const char * argv[])
    {
        int returnval = -1;
        PrintTitle();

        //Handle arguments
        returnval = GatherArgs( argc, argv );
        if( returnval != 0 )
            return returnval;
        
        //Execute the utility
        returnval = Execute();

#ifdef _DEBUG
        utils::PortablePause();
#endif

        return returnval;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================

//#TODO: Move the main function somewhere else !
int main( int argc, const char * argv[] )
{
    using namespace audioutil;
    try
    {
        return CAudioUtil::GetInstance().Main(argc,argv);
    }
    catch(const exception & e )
    {
        cout<< "<!>-ERROR:" <<e.what()<<"\n"
            << "If you get this particular error output, it means an exception got through, and the programmer should be notified!\n";
    }

    return 0;
}