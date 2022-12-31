#include <dse/dse_conversion.hpp>
#include <dse/dse_interpreter.hpp>
#include <dse/dse_containers.hpp>
#include <dse/bgm_blob.hpp>
#include <dse/bgm_container.hpp>
#include <dse/dse_to_sf2.hpp>

#include <ppmdu/fmts/swdl.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/sedl.hpp>

#include <ext_fmts/sf2.hpp>
#include <ext_fmts/adpcm.hpp>

#include <utils/audio_utilities.hpp>

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>

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
        
        //Loaded stuff
        DSE::PresetDatabase                       m_bankdb;
        std::vector<DSE::MusicSequence>           m_mseqs;
        std::vector<DSE::SoundEffectSequences>    m_eseqs;
        std::map<std::string, bankid_t>           m_loadedPaths; //Keeps track of loaded filenames, so we don't load something twice


        //Wrapper for adding new banks to the loader so we can set them up properly
        DSE::PresetBank& AddBank(DSE::PresetBank && bnk) 
        {
            return m_bankdb.AddBank(std::forward<DSE::PresetBank>(bnk));
        }

        DSE::MusicSequence& AddMusicSeq(DSE::MusicSequence&& seq) 
        {
            m_mseqs.push_back(std::forward<DSE::MusicSequence>(seq));
            return m_mseqs.back();
        }

        DSE::SoundEffectSequences& AddSoundEffectSeq(DSE::SoundEffectSequences&& seq) 
        {
            m_eseqs.push_back(std::forward<DSE::SoundEffectSequences>(seq));
            return m_eseqs.back();
        }

    public:
        DSELoaderImpl()
        {}

        ~DSELoaderImpl()
        {}

        DSE::PresetBank & LoadSWDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {})
        {
            if (!origfilename.empty() && m_loadedPaths.contains(origfilename)) //Prevents loading several times the same bank file
                return m_bankdb.at(m_loadedPaths.at(origfilename));

            PresetBank bnk(DSE::ParseSWDL(itbeg, itend));
            if (!origfilename.empty())
            {
                bnk.metadata().origfname = origfilename;
                m_loadedPaths.insert(std::make_pair(origfilename, bnk.metadata().get_bank_id()));
            }
            return AddBank(std::move(bnk));
        }

        DSE::MusicSequence& LoadSMDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {})
        {
            MusicSequence seq(DSE::ParseSMDL(itbeg, itend));
            if (!origfilename.empty())
                seq.metadata().origfname = origfilename;
            return AddMusicSeq(std::move(seq));
        }

        DSE::SoundEffectSequences& LoadSEDL(std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::string origfilename = {})
        {
            SoundEffectSequences seq(DSE::ParseSEDL(itbeg, itend));
            if (!origfilename.empty())
                seq.m_meta.origfname = origfilename;
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
                            << "Skipping file and attemnpting to recover!\n\n";
                        if (utils::LibWide().isLogOn())
                        {
                            clog << "\nLoadSIR0Containers(): Exception : " << e.what() << "\n"
                                << "Skipping file and attemnpting to recover!\n\n";
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
            for (const auto apair : foundpairs)
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
                cout << "\r[" << setfill(' ') << setw(4) << right << cntpairs << " of " << foundpairs.size() << " loaded]" << "..";
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

            int cnt = 0;
            for (const auto& entry : m_bankdb)
            {
                //Make a soundfont for the current Bank
                const DSE::PresetBank& bnk      = entry.second;
                
                const Poco::Path  curfpath = Poco::Path(fpath).append(std::to_string(cnt) + "_" + bnk.metadata().fname).makeFile().setExtension("sf2");
                const std::string curfname = Poco::Path::transcode(curfpath.getFileName());
                if (!bnk.prgmbank().lock())
                {
                    cout << "Skipping creating soundfont for \"" << bnk.metadata().fname << "\", since it doesn't have any programs to export.\n";
                    continue;
                }
                cout <<"Writing soundfont \"" << curfname << "...\n";
                sfb(bnk, curfname).Write(Poco::Path::transcode(curfpath.toString()));
                convinf.AddConversionInfo(bnk.metadata().origfname, sfb._convertionInfo);
                ++cnt;
            }
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

            return convinf;
        }

        void ExportXMLPrograms(const std::string& destpath, bool bConvertSamples = false)
        {
            //Then the MIDIs + presets + optionally samples contained in the swd of the pair
            for (const auto & bnk : m_bankdb)
            {
                Poco::Path fpath(destpath);
                fpath.append(std::to_string(bnk.second.metadata().get_bank_id()) + "_" + bnk.second.metadata().fname);

                if (!utils::DoCreateDirectory(fpath.toString()))
                {
                    clog << "Couldn't create directory for track " << fpath.toString() << "!\n";
                    continue;
                }

                ExportPresetBank(fpath.toString(), bnk.second, false, false, !bConvertSamples);
            }
        }

        void ExportMIDIs(const std::string& destdirpath, sequenceProcessingOptions options)
        {
            for (size_t i = 0; i < m_mseqs.size(); ++i)
            {
                Poco::Path fpath(destdirpath);
                fpath.append(std::to_string(i) + "_" + m_mseqs[i].metadata().fname).makeFile().setExtension("mid");

                cout << "<*>- Currently exporting smd to " << fpath.toString() << "\n";
                if (utils::LibWide().isLogOn())
                    clog << "<*>- Currently exporting smd to " << fpath.toString() << "\n";
                
                //Lookup cvinfo with the original filename from the game filesystem!
                auto foundinfo = getCvInfForSequence(options.cvinfo, m_mseqs[i].metadata().origfname);
                if (foundinfo.has_value())
                {
                    if (utils::LibWide().isLogOn())
                        clog << "<*>- Got conversion info for this track! MIDI will be remapped accordingly!\n";
                    DSE::SequenceToMidi(fpath.toString(),
                        m_mseqs[i],
                        foundinfo.value(),
                        options.nbloops,
                        DSE::eMIDIMode::GS);  //This will disable the drum channel, since we don't need it at all!
                }
                else
                {
                    if (utils::LibWide().isLogOn())
                        clog << "<!>- Couldn't find a conversion info entry for this SMDL! Falling back to converting as-is..\n";
                    DSE::SequenceToMidi(fpath.toString(),
                        m_mseqs[i],
                        options.nbloops,
                        DSE::eMIDIMode::GS);  //This will disable the drum channel, since we don't need it at all!
                }
            }
        }

    private:

        //Unused??
        const DSE::PresetBank* getBank(bankid_t bankid)const
        {
            auto itfound = m_bankdb.find(bankid);
            return (itfound != m_bankdb.end())? &(itfound->second) : nullptr;
        }

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
                [this](const std::string& filepath, size_t cnt, size_t total) 
                {
                    LoadSWDL(filepath);
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << total << " loaded]" << "..";
                });
            cout << "\n";
        }
        else if (fpath.isFile())
        {
            const std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(path);
            LoadSWDL(fdata.begin(), fdata.end(), Poco::Path(path).getFileName());
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
                [this](const std::string& filepath, size_t cnt, size_t total) 
                {
                    LoadSMDL(filepath);
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << total << " loaded]" << "..";
                });
            cout << "\n";
        }
        else if (fpath.isFile())
        {
            const std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(path);
            LoadSMDL(fdata.begin(), fdata.end());
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
                [this](const std::string& filepath, size_t cnt, size_t total) 
                {
                    LoadSEDL(filepath);
                    cout << "\r[" << setfill(' ') << setw(4) << right << cnt << " of " << total << " loaded]" << "..";
                });
            cout << "\n";
        }
        else if (fpath.isFile())
        {
            const std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(path);
            LoadSEDL(fdata.begin(), fdata.end());
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

};