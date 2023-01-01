/*
* Export handling for audioutil
*/
#include "audioutil.hpp"
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

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>

using namespace ::utils::cmdl;
using namespace ::utils::io;
using namespace ::std;
using namespace ::DSE;

namespace audioutil
{

    //--------------------------------------------
    //  Utilities
    //--------------------------------------------
    /*
        ExportASequenceToMidi
            Convenience function for exporting SMDL to MIDI
    */
    void ExportASequenceToMidi(const MusicSequence& seq,
        const string          pairname,
        Poco::Path            outputfile,
        const std::string& convinfo,
        int                   nbloops,
        bool                  asGM)
    {
        DSE::eMIDIMode convmode = (asGM) ? DSE::eMIDIMode::GM : DSE::eMIDIMode::GS;

        if (asGM)
            clog << "<*>- Conversion mode set to General MIDI instead of the default Roland GS!\n";
        else
            clog << "<*>- Conversion mode set to Roland GS!\n";

        //Check if we have conversion info supplied
        if (!convinfo.empty())
        {
            DSE::SMDLConvInfoDB cvinf(convinfo);
            auto itfound = cvinf.FindConversionInfo(pairname);

            if (itfound != cvinf.end())
            {
                clog << "<*>- Got conversion info for this track! MIDI will be remapped accordingly!\n";
                DSE::SequenceToMidi(outputfile.toString(), seq, itfound->second, nbloops, convmode);
                return;
            }
            else
                clog << "<!>- Couldn't find an entry for this SMD + SWD pair! Falling back to converting as-is..\n";
        }
        else
            clog << "<*>- Received no conversion info for this track! The SMDL will be exported as-is!\n";

        DSE::SequenceToMidi(outputfile.toString(), seq, nbloops, convmode);
    }

    inline bool SameFileExt(std::string ext1, std::string ext2, std::locale curloc = locale::classic())
    {
        auto lambdacmpchar = [&curloc](char c1, char c2)->bool
        {
            return (std::tolower(c1, curloc) == std::tolower(c2, curloc));
        };

        //Pre-check to make sure we even have the same lengths
        if (ext1.length() != ext2.length())
            return false;

        //std::transform(ext1.begin(), ext1.end(), ext1.begin(), ::tolower);
        //std::transform(ext2.begin(), ext2.end(), ext2.begin(), ::tolower);
        //return ext1 == ext2;
        return std::equal(ext1.begin(), ext1.end(), ext2.begin(), lambdacmpchar);
    }

    /*
        ProcessAllFilesWithExtInDir
            Run the specified function on each files in the directory with the matching extension!
    */
    void ProcessAllFilesWithExtInDir(const Poco::Path& dir,
        const std::string& ext,
        const std::string& desc,       //Word displayed on the console to describle the action. Like "Exporting"
        std::function<void(const Poco::Path&)>&& fun)
    {
        stringstream            sstrdesc;
        sstrdesc << "\r" << desc << " ";
        const std::string       descstr = sstrdesc.str(); //We cut on rebuilding the string each turns
        Poco::DirectoryIterator itdir(dir);
        Poco::DirectoryIterator itend;

        for (; itdir != itend; ++itdir)
        {
            if (itdir->isFile())
            {
                Poco::Path curpath(itdir->path());
                if (SameFileExt(curpath.getExtension(), ext))
                {
                    cout << right << setw(60) << setfill(' ') << descstr << curpath.getBaseName() << "..\n";
                    fun(curpath);
                }
            }
        }
    }

    /*
        MakeOutputDirectory
            Create the output directory if necessary.
    */
    void CreateOutputDir(const std::string& outputdir)
    {
        Poco::File outdir(outputdir);
        Poco::Path outpath(outputdir);
        if (!outdir.exists())
        {
            if (outdir.createDirectory())
                cout << "<*>- Created output directory \"" << outpath.absolute().toString() << "\" !\n";
            else
                cout << "<!>- Couldn't create output directory \"" << outpath.absolute().toString() << "\" !\n";
        }
        else if (!outdir.isDirectory())
            throw std::runtime_error("Error, output path " + outpath.absolute().toString() + " already exists, but not as a directory!");
    }

    //--------------------------------------------
    //  Setup
    //--------------------------------------------
    //void CAudioUtil::DoExportLoader(DSE::BatchAudioLoader& bal, const std::string& outputpath)
    //{
    //    cout << "-------------------------------------------------------------\n";
    //    if (m_outtype == eOutputType::SF2)
    //    {
    //        cout << "Exporting soundfont and MIDI files to " << outputpath << "..\n";
    //        bal.ExportSoundfontAndMIDIs(outputpath, m_nbloops, m_bBakeSamples);
    //    }
    //    else if (m_outtype == eOutputType::DLS)
    //    {
    //        cout << "Exporting DLS and MIDI files to " << outputpath << "..\n";
    //        cerr << "DLS support not implemented!\n";
    //        assert(false);
    //    }
    //    else if (m_outtype == eOutputType::XML)
    //    {
    //        cout << "Exporting sample + instruments presets data and MIDI files to " << outputpath << "..\n";
    //        bal.ExportXMLAndMIDIs(outputpath, m_nbloops);
    //    }
    //    else if (m_outtype == eOutputType::MIDI_Only)
    //    {
    //        cout << "Exporting MIDI files only to " << outputpath << "..\n";
    //        bal.ExportMIDIs(outputpath, m_convinfopath, m_nbloops);
    //    }
    //    else
    //    {
    //        cerr << "Internal Error: Output type is invalid!\n"
    //            << "Report this bug please!\n";
    //        assert(false);
    //    }
    //}

    //--------------------------------------------
    //  Operations
    //--------------------------------------------
    /*
        Extract the content of the PMD2
    */
    int CAudioUtil::ExportPMD2Audio()
    {
        ////using namespace pmd2::audio;
        //Poco::Path inputdir(m_inputPath);

        //if (m_bGM)
        //    clog << "<!>- Warning: Commandlin parameter GM specified, but GM conversion of PMD2 is currently unsuported! Falling back to Roland GS conversion!\n";

        ////validate the /SOUND/BGM directory
        //Poco::File bgmdir(Poco::Path(inputdir).append("SOUND").append("BGM"));

        //if (bgmdir.exists() && bgmdir.isDirectory())
        //{
        //    Poco::Path outputdir;
        //    if (!m_outputPath.empty())
        //        outputdir = Poco::Path(m_outputPath);
        //    else
        //        outputdir = inputdir;

        //    //Export the /BGM tracks
        //    Poco::Path mbankpath(Poco::Path(inputdir).append("SOUND").append("BGM").append("bgm.swd").makeFile().toString());
        //    BatchAudioLoader bal(true, m_bUseLFOFx);

        //    //  1. Grab the main sample bank.
        //    cout << "\n<*>- Loading master bank " << mbankpath.toString() << "..\n";
        //    bal.LoadMasterBank(mbankpath.toString());
        //    cout << "..done\n";

        //    Poco::Path bgmdirpath(Poco::Path(inputdir).append("SOUND").append("BGM"));
        //    const string bgmdir = bgmdirpath.toString();

        //    //  2. Grab all the swd and smd pairs in the folder
        //    bal.LoadMatchedSMDLSWDLPairs(bgmdir, bgmdir);

        //    const string outdir = outputdir.toString();
        //    CreateOutputDir(outdir);
        //    DoExportLoader(bal, outdir);
        //    cout << "..done\n";

        //}
        //else
        //    cout << "Skipping missing /SOUND/BGM directory\n";

        ////validate the /SOUND/ME directory
        //Poco::File medir(Poco::Path(inputdir).append("SOUND").append("ME"));

        ////Export the /ME tracks
        //if (medir.exists() && medir.isDirectory())
        //{
        //    //!#TODO
        //}
        //else
        //    cout << "Skipping missing /SOUND/ME directory\n";

        ////validate the /SOUND/SE directory
        //Poco::File sedir(Poco::Path(inputdir).append("SOUND").append("SE"));
        ////Export the /SE sounds
        //if (sedir.exists() && sedir.isDirectory())
        //{
        //    //!#TODO
        //}
        //else
        //    cout << "Skipping missing /SOUND/SE directory\n";

        ////validate the /SOUND/SWD directory
        //Poco::File swddir(Poco::Path(inputdir).append("SOUND").append("SWD"));
        ////Export the /SWD samples
        //if (swddir.exists() && swddir.isDirectory())
        //{
        //    //!#TODO
        //}
        //else
        //    cout << "Skipping missing /SOUND/SWD directory\n";

        ////validate the /SOUND/SYSTEM directory
        //Poco::File sysdir(Poco::Path(inputdir).append("SOUND").append("SYSTEM"));

        ////Export the /SYSTEM sounds
        //if (sysdir.exists() && sysdir.isDirectory())
        //{
        //    //!#TODO
        //}
        //else
        //    cout << "Skipping missing /SOUND/SYSTEM directory\n";

        //cout << "All done!\n";

        return 0;
    }

    int CAudioUtil::ExportSWDL(const std::string& path, const std::vector<uint8_t>& fdata)
    {
        //using namespace pmd2::audio;
        Poco::Path inputfile(path);
        Poco::Path outputfile;
        string     outfname;

        if (!m_outputPath.empty())
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append(inputfile.getBaseName()).makeDirectory();

        outfname = outputfile.getBaseName();

        // The only thing we can do with a single swd file is to output its content to a directory

        //Create directory
        const string outNewDir = outputfile.toString();
        CreateOutputDir(outNewDir);

        cout << "Exporting SWDL:\n"
            << "\t\"" << inputfile.toString() << "\"\n"
            << "To:\n"
            << "\t\"" << outNewDir << "\"\n";

        //Load SWDL
        PresetBank swd(DSE::ParseSWDL(fdata.begin(), fdata.end()));
        ExportPresetBank(outNewDir, swd, true, m_useHexaNumbers, !m_bConvertSamples);

        return 0;
    }

    int CAudioUtil::ExportSMDL(const std::string& path, const std::vector<uint8_t>& fdata)
    {
        //using namespace pmd2::audio;
        Poco::Path inputfile(path);
        Poco::Path outputfile;
        string     outfname;

        if (!m_outputPath.empty())
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append(inputfile.getBaseName()).makeFile();

        outfname = outputfile.getBaseName();

        cout << "Exporting SMDL:\n"
            << "\t\"" << inputfile.toString() << "\"\n"
            << "To:\n"
            << "\t\"" << outfname << "\"\n";
        cout << "<*>- Exporting SMDL to MIDI !\n";

        //By default export a sequence only!
        MusicSequence smd(DSE::ParseSMDL(fdata.begin(), fdata.end()));
        outputfile.setExtension("mid");

        ExportASequenceToMidi(smd, inputfile.getBaseName(), outputfile.toString(), m_convinfopath, m_nbloops, m_bGM);
        return 0;
    }

    int CAudioUtil::ExportSEDL(const std::string& path, const std::vector<uint8_t>& fdata)
    {
        cout << "Not implemented!\n";
        assert(false);
        return 0;
    }


    int CAudioUtil::ExportBatchPairsAndBank()
    {
        ////using namespace pmd2::audio;
        //BatchAudioLoader bal(true, m_bUseLFOFx);

        //Poco::Path outputDir;

        //if (!m_outputPath.empty())
        //    outputDir = Poco::Path(m_outputPath);
        //else
        //    outputDir = Poco::Path(m_smdlpath);

        //if (m_bGM)
        //    clog << "<!>- Warning: Commandline parameter GM specified, but GM conversion of is currently unsuported in this mode! Falling back to Roland GS conversion!\n";

        ////  1. Grab the main sample bank.
        //cout << "\n<*>- Loading master bank " << m_mbankpath << "..\n";
        //bal.LoadMasterBank(m_mbankpath);
        //cout << "..done\n";

        //bal.LoadMatchedSMDLSWDLPairs(m_swdlpath, m_smdlpath);

        //const string stroutpath = outputDir.toString();
        //CreateOutputDir(stroutpath);
        //DoExportLoader(bal, stroutpath);

        //cout << "..done\n";

        return 0;
    }

    int CAudioUtil::ExportBatchPairs()
    {
        ////using namespace pmd2::audio;
        //BatchAudioLoader bal(false, m_bUseLFOFx);

        //Poco::Path   outputDir;

        //if (!m_outputPath.empty())
        //    outputDir = Poco::Path(m_outputPath);
        //else
        //{
        //    if (!m_bgmcntpath.empty() && !m_bgmcntext.empty())
        //        outputDir = Poco::Path(m_bgmcntpath);
        //    else
        //        outputDir = Poco::Path(m_smdlpath);
        //}

        //if (m_bGM)
        //    clog << "<!>- Warning: Commandline parameter GM specified, but GM conversion of is currently unsuported in this mode! Falling back to Roland GS conversion!\n";

        //if (!m_bgmcntpath.empty() && !m_bgmcntext.empty())
        //    bal.LoadBgmContainers(m_bgmcntpath, m_bgmcntext);
        //else if (!m_bgmblobpath.empty())
        //{
        //    for (const auto& blobpath : m_bgmblobpath)
        //        bal.LoadSMDLSWDLSPairsFromBlob(blobpath, m_bmatchbyname);
        //}
        //else
        //    bal.LoadMatchedSMDLSWDLPairs(m_swdlpath, m_smdlpath);

        //const string stroutpath = outputDir.toString();
        //CreateOutputDir(stroutpath);
        //DoExportLoader(bal, stroutpath);
        //cout << "..done\n";

        return 0;
    }


    int CAudioUtil::ExportBatchSWDL()
    {
        //using namespace pmd2::audio;
        Poco::Path   inputfile(m_swdlpath);
        Poco::Path   outputDir;

        if (!m_outputPath.empty())
            outputDir = Poco::Path(m_outputPath);
        else
            outputDir = inputfile;

        cout << "Exporting SWDL:\n"
            << "\t\"" << inputfile.toString() << "\"\n"
            << "To:\n"
            << "\t\"" << outputDir.toString() << "\"\n";

        //Create parent output dir
        CreateOutputDir(outputDir.toString());

        auto lambdaExport = [&](const Poco::Path& curpath)
        {
            string       infilename = curpath.getBaseName();
            const string outNewDir = Poco::Path(outputDir).append(infilename).toString();
            //Load SWDL
            PresetBank swd = move(DSE::ParseSWDL(curpath.toString()));
            {
                auto ptrsmpls = swd.smplbank().lock();

                if (ptrsmpls != nullptr)
                    CreateOutputDir(outNewDir);  //Create sub directory
            }
            ExportPresetBank(outNewDir, swd, true, m_useHexaNumbers, !m_bConvertSamples);
        };

        ProcessAllFilesWithExtInDir(m_swdlpath, SWDL_FileExtension, "Exporting", lambdaExport);


        //Poco::DirectoryIterator itdir(m_swdlpath);
        //Poco::DirectoryIterator itend;

        //for( ; itdir != itend; ++itdir )
        //{
        //    if( itdir->isFile() )
        //    {
        //        Poco::Path curpath(itdir->path());
        //        if( !SameFileExt( curpath.getExtension(), SWDL_FileExtension ) )
        //            continue;

        //        string       infilename = curpath.getBaseName();
        //        const string outNewDir  = Poco::Path(outputDir).append(infilename).toString();

        //        cout <<right <<setw(60) <<setfill(' ') << "\rExporting " << infilename << "..";

        //        //Load SWDL
        //        PresetBank swd = move( DSE::ParseSWDL( curpath.toString() ) );
        //        {
        //            auto ptrsmpls = swd.smplbank().lock();

        //            if( ptrsmpls != nullptr )
        //                CreateOutputDir( outNewDir );  //Create sub directory
        //        }
        //        ExportPresetBank( outNewDir, swd, true, m_useHexaNumbers );
        //    }
        //}
        cout << "\n\n<*>- Done !\n";
        return 0;
    }

    //#TODO: Replace this so it uses the batch converter class!!
    int CAudioUtil::ExportBatchSMDL()
    {
        //using namespace pmd2::audio;
        Poco::Path   inputfile(m_smdlpath);
        Poco::Path   outputDir;

        if (!m_outputPath.empty())
            outputDir = Poco::Path(m_outputPath);
        else
            outputDir = inputfile;

        cout << "Exporting SMDL:\n"
            << "\t\"" << inputfile.toString() << "\"\n"
            << "To:\n"
            << "\t\"" << outputDir.toString() << "\"\n";

        //Create parent output dir
        CreateOutputDir(outputDir.toString());

        auto lambdaExport = [&](const Poco::Path& curpath)
        {
            string       infilename = curpath.getBaseName();
            const string outNewDir = Poco::Path(outputDir).append(infilename).toString();

            MusicSequence smd = move(DSE::ParseSMDL(curpath.toString()));
            ExportASequenceToMidi(smd, infilename, Poco::Path(outputDir).append(infilename).setExtension("mid").makeFile(), m_convinfopath, m_nbloops, m_bGM);
        };

        ProcessAllFilesWithExtInDir(m_smdlpath, SMDL_FileExtension, "Exporting", lambdaExport);

        //Poco::DirectoryIterator itdir(m_smdlpath);
        //Poco::DirectoryIterator itend;

        //for( ; itdir != itend; ++itdir )
        //{
        //    if( itdir->isFile() )
        //    {
        //        Poco::Path curpath(itdir->path());
        //        if( !SameFileExt( curpath.getExtension(), SMDL_FileExtension ) )
        //            continue;

        //        string        infilename = curpath.getBaseName();
        //        cout <<right <<setw(60) <<setfill(' ')  << "\rExporting " << infilename << "..";

        //        MusicSequence smd = move( DSE::ParseSMDL( curpath.toString() ) );
        //        ExportASequenceToMidi( smd, infilename, Poco::Path(outputDir).append(infilename).setExtension("mid").makeFile(), m_convinfopath, m_nbloops, m_bGM );
        //    }
        //}
        cout << "\n\n<*>- Done !\n";
        return 0;
    }

    void WriteSWDLPresetAndSampleList(const std::string& infilename, const DSE::PresetBank& swd, std::ofstream& lstfile, bool bhexnumbers)
    {
        auto prgmptr = swd.prgmbank().lock();

        if (prgmptr != nullptr)
        {
            size_t cntprg = 0;
            lstfile << "\n======================================================================\n"
                << "*" << infilename << "\n"
                << "======================================================================\n"
                << "Uses the following presets : \n";

            if (bhexnumbers)
                lstfile << hex << showbase;
            else
                lstfile << dec << noshowbase;

            for (const auto& prgm : prgmptr->PrgmInfo())
            {
                if (prgm != nullptr)
                {
                    lstfile << "\n\t*Preset #" << static_cast<int>(prgm->id) << "\n"
                        << "\t------------------------------------\n";

                    for (const auto& split : prgm->m_splitstbl)
                    {
                        lstfile << "\t\t->Split " << right << setw(4) << setfill(' ') << static_cast<int>(split.id) << ": "
                            << "Sample ID( " << right << setw(6) << setfill(' ') << static_cast<int>(split.smplid)
                            << " ), MIDI KeyRange( " << right << setw(4) << setfill(' ') << static_cast<int>(split.lowkey) << " to "
                            << right << setw(4) << setfill(' ') << static_cast<int>(split.hikey) << " ),"
                            << " \n";
                    }
                    ++cntprg;
                }
            }

            if (bhexnumbers)
                lstfile << dec << noshowbase;
            lstfile << "\nTotal " << cntprg << " Presets!\n\n";
        }
    }


    int CAudioUtil::BatchListSWDLPrgm(const string& SrcPath)
    {
        //using namespace pmd2::audio;
        Poco::Path   inputfile(SrcPath);
        Poco::Path   outputfile;

        if (!m_outputPath.empty())
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append(inputfile.getBaseName()).makeFile();

        cout << "Listing SWDL Files Presets From Directory:\n"
            << "\t\"" << inputfile.toString() << "\"\n"
            << "To:\n"
            << "\t\"" << outputfile.toString() << "\"\n";


        std::ofstream lstfile(outputfile.toString());
        if (!lstfile.is_open() || lstfile.bad())
            throw runtime_error("CAudioUtil::BatchListSWDLPrgm(): Couldn't open output file " + outputfile.toString() + " for writing!");

        //Work differently depending on what the input is
        Poco::File infile(inputfile);
        if (infile.exists() && infile.isFile())
        {
            if (!SameFileExt(inputfile.getExtension(), SWDL_FileExtension))
            {
                cerr << "<!>- ERROR: Input file is not a SWDL file!\n";
                return -1;
            }

            string infilename = inputfile.getBaseName();
            cout << right << setw(60) << setfill(' ') << "\rAnalyzing " << infilename << "..";

            lstfile << "\n======================================================================\n"
                << "*" << infilename << "\n"
                << "======================================================================\n\n";

            PresetBank swd = move(DSE::ParseSWDL(inputfile.toString()));
            WriteSWDLPresetAndSampleList(infilename, swd, lstfile, m_useHexaNumbers);
        }
        else
        {

            auto lambdaExport = [&](const Poco::Path& curpath)
            {
                string     infilename = curpath.getBaseName();
                PresetBank swd = move(DSE::ParseSWDL(curpath.toString()));
                WriteSWDLPresetAndSampleList(infilename, swd, lstfile, m_useHexaNumbers);
            };

            ProcessAllFilesWithExtInDir(SrcPath, SWDL_FileExtension, "Analyzing", lambdaExport);

            //Poco::DirectoryIterator itdir(SrcPath);
            //Poco::DirectoryIterator itend;

            //for( ; itdir != itend; ++itdir )
            //{
            //    if( itdir->isFile() )
            //    {
            //        Poco::Path curpath(itdir->path());
            //        if( !SameFileExt( curpath.getExtension(), SWDL_FileExtension ) )
            //            continue;

            //        string infilename = curpath.getBaseName();
            //        cout <<right <<setw(60) <<setfill(' ') << "\rAnalyzing " << infilename << "..";

            //        PresetBank swd = move( DSE::ParseSWDL( curpath.toString() ) );
            //        WriteSWDLPresetAndSampleList( infilename, swd, lstfile, m_useHexaNumbers );
            //    }
            //}
        }
        cout << "\n\n<*>- Done !\n";
        return 0;
    }

    void MakeInitialCvInfoForPrgmBank(DSE::SMDLPresetConversionInfo& dest, const DSE::ProgramBank& curbank)
    {
        for (const auto& aprgm : curbank.PrgmInfo())
        {
            if (aprgm != nullptr)
            {
                uint16_t prgid = aprgm->id;
                DSE::SMDLPresetConversionInfo::PresetConvData mycvdat(static_cast<presetid_t>(prgid));
                mycvdat.midipres = static_cast<presetid_t>(prgid);

                if (prgid == 0x7F)
                {
                    mycvdat.midibank = 127;
                    mycvdat.idealchan = 10;
                }

                for (const auto& split : aprgm->m_splitstbl)
                {
                    mycvdat.origsmplids.push_back(split.smplid);

                    if ((split.hikey - split.lowkey) == 0) //Only care about single key splits
                    {
                        DSE::SMDLPresetConversionInfo::NoteRemapData nrmap;
                        nrmap.destnote = split.hikey;
                        nrmap.origsmplid = split.smplid;
                        mycvdat.remapnotes.insert(make_pair(split.hikey, move(nrmap)));
                    }
                }

                dest.AddPresetConvInfo(prgid, move(mycvdat));
            }
        }
    }

    int CAudioUtil::MakeCvinfo()
    {
        Poco::Path   inputfile(m_swdlpath);
        Poco::Path   outputfile;

        if (!m_outputPath.empty())
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = Poco::Path(inputfile).setFileName("cvinfo.xml");

        cout << "Exporting cvinfo file To:\n"
            << "\t\"" << outputfile.toString() << "\"\n";

        DSE::SMDLConvInfoDB cvdb;

        auto lambdaParseSwdl = [&](const Poco::Path& curpath)
        {
            try
            {
                string     infilename = curpath.getBaseName();
                PresetBank swd = move(DSE::ParseSWDL(curpath.toString()));
                {
                    auto ptrprgms = swd.prgmbank().lock();

                    if (ptrprgms != nullptr)
                    {
                        DSE::SMDLPresetConversionInfo prgscvinf;
                        MakeInitialCvInfoForPrgmBank(prgscvinf, *ptrprgms);
                        //Make cvinfo data entry
                        cvdb.AddConversionInfo(infilename, move(prgscvinf));
                    }
                }
            }
            catch (const exception& e)
            {
                cerr << "\n<!>- Error: " << e.what() << " Skipping file and attempting to continue!\n";
            }
        };

        auto lambdaParseBgmCnt = [&](const Poco::Path& curpath)
        {
            try
            {
                string infilename = curpath.getBaseName();
                auto   bgmcnt = move(DSE::ReadBgmContainer(curpath.toString()));
                {
                    auto ptrprgms = bgmcnt.first.prgmbank().lock();

                    if (ptrprgms != nullptr)
                    {
                        DSE::SMDLPresetConversionInfo prgscvinf;
                        MakeInitialCvInfoForPrgmBank(prgscvinf, *ptrprgms);
                        //Make cvinfo data entry
                        cvdb.AddConversionInfo(infilename, move(prgscvinf));
                    }
                }
            }
            catch (const exception& e)
            {
                cerr << "\n<!>- Error: " << e.what() << " Skipping file and attempting to continue!\n";
            }
        };

        if (!m_swdlpath.empty())
            ProcessAllFilesWithExtInDir(m_swdlpath, SWDL_FileExtension, "Analyzing", lambdaParseSwdl);
        else if (!m_bgmcntpath.empty() && !m_bgmcntext.empty())
            ProcessAllFilesWithExtInDir(m_bgmcntpath, m_bgmcntext, "Analyzing", lambdaParseBgmCnt);
        else
            throw runtime_error("CAudioUtil::MakeCvinfo(): No SWDL path or bgm container path specified!");

        //for( ; itdir != itend; ++itdir )
        //{

        //    Poco::Path curpath(itdir->path());
        //    if( !SameFileExt( curpath.getExtension(), SWDL_FileExtension ) )
        //        continue;

        //    string       infilename = curpath.getBaseName();

        //    cout <<right <<setw(60) <<setfill(' ') << "\rParsing " << infilename << "..";

        //    //Load SWDL
        //    PresetBank swd = move( DSE::ParseSWDL( curpath.toString() ) );
        //    {
        //        auto ptrprgms = swd.prgmbank().lock();

        //        if( ptrprgms != nullptr )
        //        {
        //            DSE::SMDLPresetConversionInfo prgscvinf;
        //            MakeInitialCvInfoForPrgmBank( prgscvinf, *ptrprgms );
        //            //Make cvinfo data entry
        //            cvdb.AddConversionInfo( infilename, move(prgscvinf) );
        //        }
        //    }
        //}

        //Write cvinfo db
        cvdb.Write(outputfile.toString());

        cout << "\n\n<*>- Done !\n";
        return 0;
    }
};
