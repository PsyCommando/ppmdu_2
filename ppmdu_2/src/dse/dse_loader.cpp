#include <dse/dse_conversion.hpp>
#include <dse/dse_interpreter.hpp>
#include <dse/containers/dse_preset_db.hpp>
#include <dse/containers/dse_music_sequence.hpp>
#include <dse/containers/dse_se_sequence.hpp>
#include <dse/bgm_blob.hpp>
#include <dse/bgm_container.hpp>
#include <dse/dse_to_sf2.hpp>

#include <dse/fmts/swdl.hpp>
#include <dse/fmts/smdl.hpp>
#include <dse/fmts/sedl.hpp>

#include <ext_fmts/sf2.hpp>
#include <ext_fmts/adpcm.hpp>

#include <utils/audio_utilities.hpp>

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <optional>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>

using namespace std;

namespace DSE 
{
///////////////////////////////////////////////////////////////////////////////////
// DSELoaderImpl Implementation
///////////////////////////////////////////////////////////////////////////////////
    class DSELoader::DSELoaderImpl
    {
    private:
        friend class DSELoader;
        using presetid_t = uint8_t;
        
        //#TODO: Should link pairs together instead of loading separately, because handling things possibly being or not in 3 list is really problematic

        //Loaded stuff
        DSE::PresetDatabase                                        m_bankdb;
        std::unordered_map<std::string, DSE::MusicSequence>        m_mseqs;
        std::unordered_map<std::string, DSE::SoundEffectSequences> m_eseqs;
        std::string                                                m_masterbankid; //If we have a master sample bank, store its id in here

        //Pair Ordering
        size_t                        m_pairCnt;
        std::map<std::string, size_t> m_loadedPairOrdering;

        size_t GetOrSetLoadOrder(const std::string& pairname)
        {
            if (pairname.empty())
                assert(false);
            size_t order = 0;

            if (!m_loadedPairOrdering.contains(pairname))
            {
                //Assign new order
                order  = m_pairCnt;
                m_loadedPairOrdering.insert(make_pair(pairname, m_pairCnt));
                ++m_pairCnt;
            }
            else
            {
                //Assign existing order
                order = m_loadedPairOrdering.at(pairname);
            }
            return order;
        }

        //Wrapper for adding new banks to the loader so we can set them up properly
        DSE::PresetBank& AddBank(DSE::PresetBank&& bnk)
        {
            if (bnk.isMasterSampleBank())
                m_masterbankid = bnk.metadata().get_original_file_name_no_ext();
            return m_bankdb.AddBank(std::forward<DSE::PresetBank>(bnk));
        }

        DSE::MusicSequence& AddMusicSeq(DSE::MusicSequence&& seq)
        {
            auto res = m_mseqs.insert(std::make_pair(seq.metadata().get_original_file_name_no_ext(), std::forward<DSE::MusicSequence>(seq)));
            return (res.first)->second;
        }

        DSE::SoundEffectSequences& AddSoundEffectSeq(DSE::SoundEffectSequences&& seq)
        {
            auto res = m_eseqs.insert(std::make_pair(seq.m_meta.get_original_file_name_no_ext(), std::forward<DSE::SoundEffectSequences>(seq)));
            return (res.first)->second;
        }

    public:
        DSELoaderImpl()
            :m_pairCnt(0)
        {}

        ~DSELoaderImpl()
        {}

        DSE::PresetBank & LoadSWDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {})
        {
            const std::string filenameonly = Poco::Path(origfilename).getBaseName();
            if (!origfilename.empty() && m_bankdb.contains(filenameonly)) //Prevents loading several times the same bank file
                return m_bankdb.at(filenameonly);

            PresetBank bnk(DSE::ParseSWDL(itbeg, itend));
            bnk.metadata().origfname = origfilename; //Make sure the original filename is preserved

            //Assign load order depending if we loaded another part of the given pair or not
            bnk.metadata().origloadorder = GetOrSetLoadOrder(bnk.metadata().get_original_file_name_no_ext());
            
            return AddBank(std::move(bnk));
        }

        DSE::MusicSequence& LoadSMDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {})
        {
            MusicSequence seq(DSE::ParseSMDL(itbeg, itend));
            seq.metadata().origfname = origfilename;

            //Assign load order depending if we loaded another part of the given pair or not
            seq.metadata().origloadorder = GetOrSetLoadOrder(seq.metadata().get_original_file_name_no_ext());
            return AddMusicSeq(std::move(seq));
        }

        DSE::SoundEffectSequences& LoadSEDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {})
        {
            SoundEffectSequences seq(DSE::ParseSEDL(itbeg, itend));
            seq.metadata().origfname = origfilename;

            //Assign load order depending if we loaded another part of the given pair or not
            seq.metadata().origloadorder = GetOrSetLoadOrder(seq.metadata().get_original_file_name_no_ext());
            return AddSoundEffectSeq(std::move(seq));
        }

        void LoadSIR0Container(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {})
        {
            if (utils::LibWide().isLogOn() && !origfilename.empty())
                clog << "--------------------------------------------------------------------------\n"
                << "Parsing SIR0 container \"" << Poco::Path(origfilename).getFileName() << "\"\n"
                << "--------------------------------------------------------------------------\n";
            auto pairdata = ReadBgmContainer(itbeg, itend, origfilename);

            //Tag our files with their original file name, for cvinfo lookups to work!
            if (!origfilename.empty())
            {
                pairdata.first.metadata().origfname  = Poco::Path(origfilename).getBaseName();
                pairdata.second.metadata().origfname = Poco::Path(origfilename).getBaseName();
            }

            AddBank(std::move(pairdata.first));
            AddMusicSeq(std::move(pairdata.second));
        }

        void LoadSIR0Containers(const std::string& dirpath, const std::string& fileext)
        {
            //Grab all the bgm containers in here
            Poco::DirectoryIterator dirit(dirpath);
            Poco::DirectoryIterator diritend;
            cout << "<*>- Loading bgm containers *." << fileext << " in the " << dirpath << " directory..\n";

            stringstream sstrloadingmesage;
            sstrloadingmesage << " *." << fileext << " loaded] - Currently loading : ";
            const string loadingmsg = sstrloadingmesage.str();

            unsigned int cntparsed = 0;
            while (dirit != diritend)
            {
                if (dirit->isFile())
                {
                    string fext = dirit.path().getExtension();
                    std::transform(fext.begin(), fext.end(), fext.begin(), ::tolower);

                    //Check all smd/swd file pairs
                    try
                    {
                        if (fext == fileext)
                        {
                            cout << "\r[" << setfill(' ') << setw(4) << right << cntparsed << loadingmsg << dirit.path().getFileName() << "..";
                            std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(dirit.path().absolute().toString());
                            LoadSIR0Container(fdata.begin(), fdata.end(), dirit.path().absolute().toString());
                            ++cntparsed;
                        }
                    }
                    catch (std::exception& e)
                    {
                        cerr << "\nLoadSIR0Containers(): Exception : " << e.what() << "\n"
                            << "\nSkipping file and attemnpting to recover!\n\n";
                        if (utils::LibWide().isLogOn())
                        {
                            clog << "\nLoadSIR0Containers(): Exception : " << e.what() << "\n"
                                << "\nSkipping file and attemnpting to recover!\n\n";
                        }
                    }
                }
                ++dirit;
            }
            cout << "\n..done\n\n";
        }

        void LoadFromBlobFile(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend)
        {
            BlobScanner<vector<uint8_t>::const_iterator> blobscan(itbeg, itend);
            blobscan.Scan();
            auto foundpairs = blobscan.ListAllMatchingSMDLPairs();

            size_t cntpairs = 0;
            cout << "-----------------------------------------\n"
                << "Loading pairs from blob " << "..\n"
                << "-----------------------------------------\n";
            for (const auto & apair : foundpairs)
            {
                if (utils::LibWide().isLogOn())
                {
                    clog << "====================================================================\n"
                        << "Parsing SWDL " << apair.first._name << "\n"
                        << "====================================================================\n";
                }
                DSE::PresetBank & bank = LoadSWDL(apair.first._beg, apair.first._end);

                if (utils::LibWide().isLogOn())
                {
                    clog << "====================================================================\n"
                        << "Parsing SMDL " << apair.second._name << "\n"
                        << "====================================================================\n";
                }
                DSE::MusicSequence & seq = LoadSMDL(apair.second._beg, apair.second._end);

                //Tag our files with their original file name, for cvinfo lookups to work!
                seq .metadata().origfname = apair.second._name;
                bank.metadata().origfname = apair.first._name;
                ++cntpairs;
                cout << "\r[" << setfill(' ') << setw(4) << right << cntpairs << " of " << foundpairs.size() << " loaded]...";
            }

            cout << "\n..done!\n";
        }

        //Export

        SMDLConvInfoDB ExportSoundfonts(const std::string& destdir, sampleProcessingOptions options)
        {
            using namespace sf2;
            Poco::Path           fpath(destdir);
            SMDLConvInfoDB       convinf;
            DSE_SoundFontBuilder sfb(m_bankdb, !options.bLfoFxEnabled, options.bBakeSamples);

            //#TODO: Make soundfonts have the same name as the exported midis
            //#TODO: Try to cache some of the sample conversion stuff, since it's redundant to do it for every single soundfont when it's always the same.

            cout << "Writing soundfont...\n";
            int cnt = 0;
            const size_t nbbanks = m_bankdb.size();
            size_t longestname = 0;
            for (const auto& entry : m_bankdb)
            {
                //Make a soundfont for the current Bank
                const DSE::PresetBank& bnk      = entry.second;
                const Poco::Path  curfpath = Poco::Path(fpath).append(std::to_string(bnk.metadata().origloadorder) + "_" + bnk.metadata().fname).makeFile().setExtension("sf2");
                const std::string curfname = Poco::Path::transcode(curfpath.getFileName());

                if (!bnk.prgmbank().lock())
                    continue;
                
                sfb(bnk, curfname).Write(Poco::Path::transcode(curfpath.toString()));
                convinf.AddConversionInfo(bnk.metadata().origfname, sfb._convertionInfo);

                ++cnt;
                longestname = std::max(curfname.size(), longestname);
                cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << nbbanks << " written] - " << curfname << std::string(longestname - -curfname.size(), ' ');
            }
            cout << "\n";
            return convinf;
        }

        SMDLConvInfoDB ExportSingleSoundfont(const std::string& destdir, sampleProcessingOptions options)
        {
            using namespace sf2;
            Poco::Path                              fpath(destdir);
            SMDLConvInfoDB  convinf;
            DSE_SoundFontBuilder                    sfb(m_bankdb, !options.bLfoFxEnabled, options.bBakeSamples);

            //Move all the presets to the banks they're on

            assert(false);
            cout << "Writing soundfont...\n";
            int cnt = 0;
            const size_t nbbanks = m_bankdb.size();
            for (const auto& entry : m_bankdb)
            {
                //Re-order presets so we can jam as many presets as possible in the soundfont over all the 128 banks
                ++cnt;
                cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << nbbanks << " merged]";
            }

            cout << "\n";
            return convinf;
        }

        void ExportXMLPrograms(const std::string& destpath, bool bConvertSamples = false)
        {
            const size_t nbbanks = m_bankdb.size();
            size_t cnt = 0;
            size_t longestname = 0;
            cout << "Writing xml...\n";

            //Then the MIDIs + presets + optionally samples contained in the swd of the pair
            for (const auto & bnk : m_bankdb)
            {
                const Poco::Path  curfile  = Poco::Path(destpath).append(std::to_string(bnk.second.metadata().origloadorder) + "_" + bnk.second.metadata().fname).setExtension("xml");
                const std::string curfpath = Poco::Path::transcode(curfile.toString());
                const std::string curfname = Poco::Path::transcode(curfile.getBaseName());

                ExportPresetBank(curfpath, bnk.second, false, false, !bConvertSamples);

                ++cnt;
                longestname = std::max(curfname.size(), longestname);
                cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << nbbanks << " written] - " << curfname << std::string(longestname - curfname.size(), ' ');
            }
            cout << "\n";
        }

        void ExportMIDIs(const std::string& destdirpath, sequenceProcessingOptions options)
        {
            const size_t nbmidis = m_mseqs.size();
            size_t cnt = 0;
            size_t longestname = 0;
            cout << "Writing midis...\n";
            for (const auto & entry : m_mseqs)
            {
                const MusicSequence& seq = entry.second;
                Poco::Path fpath(destdirpath);
                fpath.append(std::to_string(seq.metadata().origloadorder) + "_" + seq.metadata().fname).makeFile().setExtension("mid");
                const std::string curpath  = fpath.toString();
                const std::string curfname = fpath.getBaseName();

                if (utils::LibWide().isLogOn())
                    clog << "<*>- Currently exporting smd to " << curpath << "\n";
                
                //Lookup cvinfo with the original filename from the game filesystem!
                auto foundinfo = getCvInfForSequence(options.cvinfo, seq.metadata().origfname);
                if (foundinfo.has_value())
                {
                    if (utils::LibWide().isLogOn())
                        clog << "\n<*>- Got conversion info for this track! MIDI will be remapped accordingly!\n";
                    DSE::SequenceToMidi(curpath,
                        seq,
                        foundinfo.value(),
                        options.nbloops,
                        utils::eMIDIMode::GS);  //This will disable the drum channel, since we don't need it at all!
                }
                else
                {
                    if (utils::LibWide().isLogOn())
                        clog << "\n<!>- Couldn't find a conversion info entry for this SMDL! Falling back to converting as-is..\n";
                    DSE::SequenceToMidi(curpath,
                        seq,
                        options.nbloops,
                        utils::eMIDIMode::GS);  //This will disable the drum channel, since we don't need it at all!
                }

                ++cnt;
                longestname = std::max(curfname.size(), longestname);
                cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << nbmidis << " written] - " << curfname << std::string(longestname - -curfname.size(), ' ');
            }
            cout << "\n";
        }

        //Import
        void ImportChangesToGame(const std::string& swdlpath, std::string smdlpath = {}, std::string sedlpath = {})
        {
            //Since we don't support differential saving right now, just import everything
            bool hasmaster = !m_masterbankid.empty();
            const std::string outswdlpath = Poco::Path::transcode(Poco::Path(swdlpath).makeDirectory().makeAbsolute().toString());
            const std::string outsmdlpath = (!smdlpath.empty()) ? (Poco::Path::transcode(Poco::Path(smdlpath).makeDirectory().makeAbsolute().toString())) : std::string();
            const std::string outsedlpath = (!sedlpath.empty()) ? (Poco::Path::transcode(Poco::Path(sedlpath).makeDirectory().makeAbsolute().toString())) : std::string();
            size_t cnt = 0;
            size_t longestname = 0;

            Poco::File outdir(outswdlpath);
            if (!outdir.exists())
            {
                cout << "Creating output directory \"" << outswdlpath <<"\"...\n";
                outdir.createDirectories();
            }

            {
                cout << "Importing Program Banks...\n";
                const size_t nbbanks = m_bankdb.size();
                for (const auto& entry : m_bankdb)
                {
                    const std::string& pairname = entry.first;
                    const PresetBank& pbank = entry.second;
                    if (pbank.metadata().origfname.empty())
                        throw std::runtime_error("SWDL file to import is missing its original file name");

                    WriteSWDL(
                        Poco::Path::transcode(Poco::Path(outswdlpath).append(pbank.metadata().origfname).makeFile().toString()),
                        pbank);

                    ++cnt;
                    longestname = std::max(pbank.metadata().origfname.size(), longestname);
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << nbbanks << " written] - " << pbank.metadata().origfname << std::string(longestname - -pbank.metadata().origfname.size(), ' ');
                }
                cout << "\n";
            }

            if (outsmdlpath.empty())
                return;

            outdir = Poco::File(outsmdlpath);
            if (!outdir.exists())
            {
                cout << "Creating output directory \"" << outsmdlpath << "\"...\n";
                outdir.createDirectories();
            }

            {
                cout << "Importing Sequences...\n";
                cnt = 0;
                longestname = 0;
                const size_t nbseqs = m_mseqs.size();
                for (const auto& entry : m_mseqs)
                {
                    const MusicSequence& seq = entry.second;
                    if (seq.metadata().origfname.empty())
                        throw std::runtime_error("SMDL file to import is missing its original file name");
                    WriteSMDL(
                        Poco::Path::transcode(Poco::Path(outsmdlpath).append(seq.metadata().origfname).makeFile().toString()),
                        seq
                    );
                    ++cnt;
                    longestname = std::max(seq.metadata().origfname.size(), longestname);
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << nbseqs << " written] - " << seq.metadata().origfname << std::string(longestname - -seq.metadata().origfname.size(), ' ');
                }
                cout << "\n";
            }

            if (outsedlpath.empty())
                return;

            outdir = Poco::File(outsedlpath);
            if (!outdir.exists())
            {
                cout << "Creating output directory \"" << outsedlpath << "\"...\n";
                outdir.createDirectories();
            }
            
            {
                cout << "Importing Sound Effect Sequences...\n";
                cnt = 0;
                longestname = 0;
                const size_t nbeseqs = m_eseqs.size();
                for (const auto& entry : m_eseqs)
                {
                    const SoundEffectSequences& seq = entry.second;
                    if (seq.metadata().origfname.empty())
                        throw std::runtime_error("SEDL file to import is missing its original file name");
                    WriteSEDL(
                        Poco::Path::transcode(Poco::Path(outsedlpath).append(seq.metadata().origfname).makeFile().toString()),
                        seq
                    );
                    ++cnt;
                    longestname = std::max(seq.metadata().origfname.size(), longestname);
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << nbeseqs << " written] - " << seq.metadata().origfname << std::string(longestname - -seq.metadata().origfname.size(), ' ');
                }
                cout << "\n";
            }
        }

        //Import

        void ImportDirectory(const std::string& pathdir)
        {
            Poco::DirectoryIterator itdir(pathdir);
            Poco::DirectoryIterator itend;

            cout << "Scanning directory \"" << pathdir << "\"...\n";
            try
            {
                size_t filecount = 0;
                for (; itdir != itend; ++itdir, ++filecount);
                itdir = Poco::DirectoryIterator(pathdir);

                size_t cnt = 0;
                for (; itdir != itend; ++itdir)
                {
                    const std::string fpath = Poco::Path::transcode(itdir.path().absolute().toString());
                    if (itdir->isFile())
                    {

                        //Tell if its a set of sequence or a bank
                        if (itdir.path().getExtension() == "xml" && IsXMLPresetBank(fpath))
                            ImportBank(fpath);
                        else if (itdir.path().getExtension() == "mid")
                            ImportMusicSeq(fpath);
                    }
                    else if (itdir->isDirectory() && IsSESequenceXmlDir(fpath))
                        ImportSoundEffectSeq(fpath);

                    ++cnt;
                    const std::string& fname = itdir.name();
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << filecount << "] - " <<left << setw(32) << ((fname.size() > 32)? fname.substr(0, 32) : fname);
                }
                cout << "\n";
            }
            catch (...)
            {
                std::throw_with_nested(std::runtime_error("Error importing file " + Poco::Path::transcode(itdir.path().toString())));
            }
        }

        //Wrapper for adding new banks to the loader so we can set them up properly
        DSE::PresetBank& ImportBank(const std::string& bnkxmlfile)
        {
            DSE::PresetBank bnk = ImportPresetBank(bnkxmlfile);
            if (bnk.isMasterSampleBank())
                m_masterbankid = bnk.metadata().get_original_file_name_no_ext();
            return m_bankdb.AddBank(std::forward<DSE::PresetBank>(bnk));
        }

        DSE::MusicSequence& ImportMusicSeq(const std::string& midipath)
        {
            DSE::MusicSequence seq = MidiToSequence(midipath);
            return (m_mseqs[seq.metadata().get_original_file_name_no_ext()] = std::move(seq));
        }

        DSE::SoundEffectSequences& ImportSoundEffectSeq(const std::string& seqdir)
        {
            DSE::SoundEffectSequences seq;
            if (!IsSESequenceXmlDir(seqdir))
                throw runtime_error("The \"" + seqdir + "\" directory is missing the required xml files to assemble a sound effect set from.");

            assert(false);

            auto res = m_eseqs.insert(std::make_pair(seq.m_meta.get_original_file_name_no_ext(), std::forward<DSE::SoundEffectSequences>(seq)));
            return (res.first)->second;
        }

    private:

        std::optional<SMDLPresetConversionInfo> getCvInfForSequence(const std::optional<DSE::SMDLConvInfoDB>& cvinf, const std::string& origfname)
        {
            SMDLConvInfoDB::const_iterator itcvend = cvinf.has_value() ? cvinf.value().end() : SMDLConvInfoDB::const_iterator{};
            SMDLConvInfoDB::const_iterator itfound = itcvend;
            
            if (cvinf.has_value())
            {
                itfound = cvinf->FindConversionInfo(origfname);
                if (itfound != itcvend)
                {
                    return itfound->second;
                }
            }
            return nullopt;
        }
    };

///////////////////////////////////////////////////////////////////////////////////
// DSELoader Interface
///////////////////////////////////////////////////////////////////////////////////
    DSELoader::DSELoader()
        :m_pimpl(new DSELoader::DSELoaderImpl)
    {}

    DSELoader::~DSELoader()
    {}

    void DSELoader::LoadSWDL(const std::string& path)
    {
        Poco::File fpath(path);
        if (fpath.isDirectory())
        {
            cout << "Loading SWDL Dir..\n";
            utils::ProcessAllFileWithExtension(path, SWDL_FileExtension, 
                [&](const std::string& filepath, size_t cnt, size_t total) 
                {
                    this->LoadSWDL(filepath);
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << total << " loaded]...";
                });
            cout << "\n";
        }
        else if (fpath.isFile())
        {
            const std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(path);
            LoadSWDL(fdata.begin(), fdata.end(), Poco::Path::transcode(Poco::Path(path).getFileName()));
        }
    }
    void DSELoader::LoadSWDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename)
    {
        m_pimpl->LoadSWDL(itbeg, itend, origfilename);
    }

    /*
    */
    void DSELoader::LoadSMDL(const std::string& path)
    {
        Poco::File fpath(path);
        if (fpath.isDirectory())
        {
            cout << "Loading SMDL Dir..\n";
            utils::ProcessAllFileWithExtension(path, SMDL_FileExtension, 
                [&](const std::string& filepath, size_t cnt, size_t total) 
                {
                    LoadSMDL(filepath);
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << total << " loaded]...";
                });
            cout << "\n";
        }
        else if (fpath.isFile())
        {
            const std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(path);
            LoadSMDL(fdata.begin(), fdata.end(), Poco::Path::transcode(Poco::Path(path).getFileName()));
        }

    }
    void DSELoader::LoadSMDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename)
    {
        m_pimpl->LoadSMDL(itbeg, itend, origfilename);
    }

    /*
    */
    void DSELoader::LoadSEDL(const std::string& path)
    {
        Poco::File fpath(path);
        if (fpath.isDirectory())
        {
            cout << "Loading SEDL Dir..\n";
            utils::ProcessAllFileWithExtension(path, SEDL_FileExtension, 
                [&](const std::string& filepath, size_t cnt, size_t total) 
                {
                    LoadSEDL(filepath);
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << total << " loaded]...";
                });
            cout << "\n";
        }
        else if (fpath.isFile())
        {
            const std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(path);
            LoadSEDL(fdata.begin(), fdata.end(), Poco::Path::transcode(Poco::Path(path).getFileName()));
        }

    }
    void DSELoader::LoadSEDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename)
    {
        m_pimpl->LoadSEDL(itbeg, itend, origfilename);
    }

    /*
    */
    void DSELoader::LoadSIR0Container(const std::string& filepath)
    {
        const std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(filepath);
        LoadSIR0Container(fdata.begin(), fdata.end());
    }

    void DSELoader::LoadSIR0Container(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename)
    {
        m_pimpl->LoadSIR0Container(itbeg, itend, origfilename);
    }

    /*
    */
    void DSELoader::LoadSIR0Containers(const std::string& dirpath, const std::string& fileext)
    {
        m_pimpl->LoadSIR0Containers(dirpath, fileext);
    }

    /*
    */
    void DSELoader::LoadFromBlobFile(const std::string& blobpath)
    {
        const std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(blobpath);
        LoadFromBlobFile(fdata.begin(), fdata.end());
    }

    void DSELoader::LoadFromBlobFile(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend)
    {
        m_pimpl->LoadFromBlobFile(itbeg, itend);
    }

    DSE::PresetBank& DSELoader::ImportBank(const std::string& bnkxmlfile)
    {
        return m_pimpl->ImportBank(bnkxmlfile);
    }
    DSE::MusicSequence& DSELoader::ImportMusicSeq(const std::string& midipath)
    {
        return m_pimpl->ImportMusicSeq(midipath);
    }
    DSE::SoundEffectSequences& DSELoader::ImportSoundEffectSeq(const std::string& seqdir)
    {
        return m_pimpl->ImportSoundEffectSeq(seqdir);
    }

    SMDLConvInfoDB DSELoader::ExportSoundfont(const std::string& despath, sampleProcessingOptions options, bool bSingleSf2)
    {
        if(!bSingleSf2)
            return m_pimpl->ExportSoundfonts(despath, std::move(options));
        else
            return m_pimpl->ExportSingleSoundfont(despath, std::move(options));
    }

    void DSELoader::ExportXMLPrograms(const std::string& destpath, bool bConvertSamples)
    {
        m_pimpl->ExportXMLPrograms(destpath, bConvertSamples);
    }

    void DSELoader::ExportMIDIs(const std::string& destdirpath, sequenceProcessingOptions options)
    {
        m_pimpl->ExportMIDIs(destdirpath, std::move(options));
    }

    void DSELoader::ImportChangesToGame(const std::string& swdlpath, std::string smdlpath, std::string sedlpath)
    {
        m_pimpl->ImportChangesToGame(swdlpath, smdlpath, sedlpath);
    }

    void DSELoader::ImportDirectory(const std::string& pathdir)
    {
        m_pimpl->ImportDirectory(pathdir);
    }

};