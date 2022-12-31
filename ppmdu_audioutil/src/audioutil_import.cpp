
/*
* Export handling for audioutil
*/
#include "audioutil.hpp"

#include <ppmdu/fmts/smdl.hpp>
#include <dse/dse_interpreter.hpp>

#include <iostream>

#include <Poco/Path.h>
#include <Poco/File.h>

using namespace std;
using namespace utils;

namespace audioutil
{
//--------------------------------------------
//  Operations
//--------------------------------------------
    int CAudioUtil::ImportPMD2Audio()
    {
        Poco::Path inputdir(m_inputPath);



        //Import whatever we have in the input directory, as long as it's coherent
        return 0;
    }


    int CAudioUtil::BuildSWDL()
    {
        cout << "Not implemented!\n";
        assert(false);
        return 0;
    }

    int CAudioUtil::BuildSMDL()
    {
        cout << "Not implemented!\n";
        assert(false);
        return 0;


        Poco::Path inputfile(m_inputPath);
        Poco::Path outputfile;

        if (!m_outputPath.empty())
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append(inputfile.getBaseName()).makeFile().setExtension("smd");

        if (m_bGM)
            clog << "<!>- Warning: Commandline parameter GM specified, when building a SMDL the GM option does nothing!\n";


        cout << "Exporting SMDL:\n"
            << "\t\"" << inputfile.toString() << "\"\n"
            << "To:\n"
            << "\t\"" << outputfile.getBaseName() << "\"\n";

        DSE::MusicSequence seq = DSE::MidiToSequence(inputfile.toString());
        seq.metadata().fname = inputfile.getFileName();
        DSE::WriteSMDL(outputfile.toString(), seq);
        return 0;
    }

    int CAudioUtil::BuildSEDL()
    {
        cout << "Not implemented!\n";
        assert(false);
        return 0;
    }
}