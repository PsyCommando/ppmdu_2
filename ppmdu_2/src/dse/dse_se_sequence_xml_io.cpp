#include <dse/dse_conversion.hpp>

#include <string>

#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>

using namespace std;

namespace DSE
{
    //====================================================================================================
    //  Constants
    //====================================================================================================
    const std::string DEF_FnameMcrl = "mcrl.xml"s;
    const std::string DEF_FnameBnkl = "bnkl.xml"s;
    const std::string DEF_FnameSeqs = "seq_info.xml"s;

    namespace ESeqXML
    {
        //Root nodes
        const string ROOT_Mcrl = "Mcrl"s;
        const string ROOT_Bnkl = "Bnkl"s;
        const string ROOT_Seq  = "Sequences"s;
    };

//====================================================================================================
//  SESequencesXMLParser
//====================================================================================================
    class SESequencesXMLParser
    {
        std::string m_srcDir;
    public:
        SESequencesXMLParser(const std::string& src) :m_srcDir(src) {}

        DSE::SoundEffectSequences Parse()
        {
            return {}; //#TODO
        }
    };

//====================================================================================================
//  SESequencesXMLWriter
//====================================================================================================
    class SESequencesXMLWriter
    {
        const DSE::SoundEffectSequences& m_srcSeq;
    public:
        SESequencesXMLWriter(const DSE::SoundEffectSequences& srcseq) :m_srcSeq(srcseq) {}

        void Write(const std::string& destdir)
        {
            //#TODO
        }
    };

//====================================================================================================
//  Functions
//====================================================================================================

    void PresetBankToXML(const DSE::SoundEffectSequences& srcseq, const std::string& destdir)
    {
        SESequencesXMLWriter(srcseq).Write(destdir);
    }

    DSE::SoundEffectSequences XMLToEffectSequences(const std::string& srcdir)
    {
        return SESequencesXMLParser(srcdir).Parse();
    }

    bool IsSESequenceXmlDir(const std::string& destdir)
    {
        //Check for the file we want
        Poco::DirectoryIterator dirit(destdir);
        Poco::DirectoryIterator diritend;

        for (; dirit != diritend; ++dirit)
        {
            if(dirit->isFile() && (dirit.name() == DEF_FnameSeqs))
                return true;
        }
        return false;
    }
};