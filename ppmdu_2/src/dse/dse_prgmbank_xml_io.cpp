#include <dse/dse_conversion.hpp>
#include <pugixml.hpp>
#include <utils/parse_utils.hpp>
#include <utils/pugixml_utils.hpp>
#include <ext_fmts/adpcm.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace pugi;
using namespace pugixmlutils;
using namespace std;

namespace DSE
{
//====================================================================================================
//  Constants
//====================================================================================================
    const string DEF_ProgramsFname = "programs.xml"s;
    const string DEF_WavInfoFname  = "samples.xml"s;
    const string DEF_KeygroupFname = "keygroups.xml"s;

    namespace PrgmXML
    {
        //Root nodes
        const string ROOT_Programs     = "Programs"s;
        const string ROOT_WavInfo      = "WavInfo"s;
        const string ROOT_KeyGroups    = "KeyGroups"s;

        //Attributes
        const string ATTR_DSE_Version  = "dse_version"s;
        const string ATTR_DSE_Unk1     = "dse_unk1"s;
        const string ATTR_DSE_Unk2     = "dse_unk2"s;
        const string ATTR_DSE_Unk17    = "dse_unk17"s;

        //Common
        const string PROP_Volume       = "Volume"s;
        const string PROP_Pan          = "Pan"s;
        const string PROP_ID           = "ID"s;
        const string PROP_FTune        = "FineTune"s;
        const string PROP_CTune        = "CoarseTune"s;
        const string PROP_RootKey      = "RootKey"s;
        const string PROP_KeyTrans     = "KeyTransposition"s;

        //Nodes
        const string NODE_Program      = "Program"s;
        const string PROP_PrgmUnk4     = "Unk4"s;
        const string PROP_PrgmUnkPoly  = "UnkPoly"s;

        const string NODE_LFOTable     = "LFOTable"s;

        const string NODE_LFOEntry     = "LFOEntry"s;
        const string PROP_LFOEnabled   = "Enabled"s;
        const string PROP_LFOModDest   = "ModulationDestination"s;
        const string PROP_LFOWaveShape = "WaveformShape"s;
        const string PROP_LFORate      = "Rate"s;
        const string PROP_LFOUnk29     = "Unk29"s;
        const string PROP_LFODepth     = "Depth"s;
        const string PROP_LFODelay     = "Delay"s;
        const string PROP_LFOUnk32     = "Unk32"s;
        const string PROP_LFOUnk33     = "Unk33"s;

        const string NODE_SplitTable   = "SplitTable"s;

        const string NODE_Split        = "Split"s;
        const string PROP_SplitUnk11   = "PitchBendRange"s;
        const string PROP_SplitUnk25   = "Unk25"s;
        const string PROP_SplitLowKey  = "LowestKey"s;
        const string PROP_SplitHighKey = "HighestKey"s;
        const string PROP_SplitLowVel  = "LowestVelocity"s;
        const string PROP_SplitHighVel = "HighestVelocity"s;
        const string PROP_SplitSmplID  = "SampleID"s;
        const string PROP_SplitKGrp    = "KeyGroupID"s;
        const string PROP_SplitUnk22   = "Unk22"s;
        const string PROP_SplitUnk23   = "Unk23"s;
        
        const string NODE_KGrp         = "KeyGroup"s;
        const string NODE_KGrpPoly     = "Polyphony"s;
        const string NODE_KGrpPrio     = "Priority"s;
        const string NODE_KGrVcLow     = "LowestVoiceChannel"s;
        const string NODE_KGrVcHi      = "HighestVoiceChannel"s;


        const string NODE_Sample       = "Sample"s;
        const string PROP_SmplUnk5     = "Unk5"s;
        const string PROP_SmplUnk58    = "Unk58"s;
        const string PROP_SmplUnk6     = "Unk6"s;
        const string PROP_SmplUnk7     = "Unk7"s;
        const string PROP_SmplUnk59    = "Unk59"s;
        //const string PROP_SmplFmt = "SampleFormat"s;
        const string PROP_SmplUnk9     = "Unk9"s;
        const string PROP_SmplLoop     = "LoopOn"s;
        const string PROP_SmplUnk10    = "Unk10"s;
        //const string PROP_Smplsperdw = "NbSamplesPerDWord"s;
        const string PROP_SmplUnk11    = "Unk11"s;
        //const string PROP_BitsPerSmpl = "BitsPerSample"s;
        const string PROP_SmplUnk12    = "Unk12"s;
        const string PROP_SmplUnk62    = "Unk62"s;
        const string PROP_SmplUnk13    = "Unk13"s;
        const string PROP_SmplRate     = "SampleRate"s;
        const string PROP_LoopBeg      = "LoopBegin"s;
        const string PROP_LoopLen      = "LoopLen"s;

        const string NODE_Envelope     = "Envelope"s;
        const string PROP_EnvOn        = "EnableEnveloppe"s;
        const string PROP_EnvMulti     = "EnveloppeDurationMultiplier"s;
        const string PROP_EnvUnk19     = "Unk19"s;
        const string PROP_EnvUnk20     = "Unk20"s;
        const string PROP_EnvUnk21     = "Unk21"s;
        const string PROP_EnvUnk22     = "Unk22"s;
        const string PROP_EnvAtkVol    = "AttackVolume"s;
        const string PROP_EnvAtk       = "Attack"s;
        const string PROP_EnvHold      = "Hold"s;
        const string PROP_EnvDecay     = "Decay"s;
        const string PROP_EnvSustain   = "Sustain"s;
        const string PROP_EnvDecay2    = "FadeOut"s;
        const string PROP_EnvRelease   = "Release"s;
        const string PROP_EnvUnk57     = "Unk57"s;
    };

//====================================================================================================
//  ProgramBankXMLParser
//====================================================================================================
class ProgramBankXMLParser
{
public:
    ProgramBankXMLParser( const string & dirpath )
        :m_path(dirpath)
    {}

    PresetBank Parse()
    {
        vector< unique_ptr<ProgramInfo> > prgmbank;
        vector<KeyGroup>                  kgrp;
        vector<SampleBank::smpldata_t>    sampledata;
        DSE_MetaDataSWDL                  meta;
        try
        {
            //3 different files for these
            Poco::Path       programpath(m_path);
            xml_document     prgmdoc;
            xml_document     keygrpdoc;
            xml_document     smpldoc;
            xml_parse_result loadres;
            string           transcoded_path = Poco::Path::transcode(programpath.setFileName(DEF_ProgramsFname).absolute().toString());
            
            meta.fname     = Poco::Path::transcode(programpath.getBaseName());
            meta.origfname = meta.fname;
            meta.createtime.SetTimeToNow();

            if((loadres = prgmdoc.load_file(transcoded_path.c_str())))
                ParsePrograms(prgmbank, prgmdoc, meta);
            else
            {
                stringstream sstr;
                sstr << "Can't load XML document \"" << transcoded_path << "\"! Pugixml returned an error : \"" << loadres.description() << "\"";
                throw std::runtime_error(sstr.str());
            }

            transcoded_path = Poco::Path::transcode(programpath.setFileName(DEF_KeygroupFname).absolute().toString());
            if ((loadres = keygrpdoc.load_file(transcoded_path.c_str())))
                ParseKeygroups(kgrp, keygrpdoc);
            else
            {
                stringstream sstr;
                sstr << "Can't load XML document \"" << transcoded_path << "\"! Pugixml returned an error : \"" << loadres.description() << "\"";
                throw std::runtime_error(sstr.str());
            }

            transcoded_path = Poco::Path::transcode(programpath.setFileName(DEF_WavInfoFname).absolute().toString());
            if ((loadres = smpldoc.load_file(transcoded_path.c_str())))
                ParseSampleInfos(sampledata, smpldoc);
            else
            {
                stringstream sstr;
                sstr << "Can't load XML document \"" << transcoded_path << "\"! Pugixml returned an error : \"" << loadres.description() << "\"";
                throw std::runtime_error(sstr.str());
            }
        }
        catch (exception& e)
        {
            stringstream sstr;
            sstr << "Got Exception while parsing XML from directory \"" << m_path << "\" : " << e.what();
            throw runtime_error(sstr.str());
        }

        return move( PresetBank( move(meta),
                                 move(unique_ptr<ProgramBank>(new ProgramBank(move(prgmbank), 
                                                                              move(kgrp)))),
                                 move(unique_ptr<SampleBank>(new SampleBank(move(sampledata)))) 
                                 ) );
    }

private:

    void ParsePrograms( vector< unique_ptr<ProgramInfo> > & prgmbank, xml_document & doc, DSE_MetaDataSWDL & meta)
    {
        using namespace PrgmXML;
        //Check version info and etc..
        xml_node      rootnode = doc.child(ROOT_Programs.c_str());
        xml_attribute dsever   = rootnode.attribute(ATTR_DSE_Version.c_str());
        xml_attribute unk1     = rootnode.attribute(ATTR_DSE_Unk1.c_str());
        xml_attribute unk2     = rootnode.attribute(ATTR_DSE_Unk2.c_str());
        xml_attribute unk17    = rootnode.attribute(ATTR_DSE_Unk17.c_str());

        meta.origversion = DSE::intToDseVer(utils::parseHexaValToValue<uint16_t>(dsever.value()));
        utils::parseHexaValToValue(unk1.value(),  meta.unk1);
        utils::parseHexaValToValue(unk2.value(),  meta.unk2);
        utils::parseHexaValToValue(unk17.value(), meta.unk17);
        
        //Parse programs
        for (auto curnode : rootnode.children(NODE_Program.c_str()))
            prgmbank.push_back(ParseAProgram(curnode));
    }

    //Parse a single program from the xml document
    unique_ptr<ProgramInfo> ParseAProgram(xml_node programnode)
    {
        using namespace PrgmXML;
        unique_ptr<ProgramInfo> info(new ProgramInfo);

        //Read program data
        for (auto entry : programnode.children())
        {
            string nodename(entry.name());
            if (nodename == PROP_ID)
                utils::parseHexaValToValue(entry.value(), info->id);
            else if (nodename == PROP_Volume)
                utils::parseHexaValToValue(entry.value(), info->prgvol);
            else if (nodename == PROP_Pan)
                utils::parseHexaValToValue(entry.value(), info->prgpan);
            else if (nodename == PROP_PrgmUnkPoly)
                utils::parseHexaValToValue(entry.value(), info->unkpoly);
            else if (nodename == PROP_PrgmUnk4)
                utils::parseHexaValToValue(entry.value(), info->unk4);
            else
                clog << "Unsupported program data node \"" << nodename << "\"\n";
        }

        //Grab subnodes
        xml_node lfotbl = programnode.child(NODE_LFOTable.c_str());
        for(auto lfo : lfotbl)
            info->m_lfotbl.push_back(ParseALFO(lfo));

        xml_node splittbl = programnode.child(NODE_SplitTable.c_str());
        for(auto split : splittbl.children(NODE_Split.c_str()))
            info->m_splitstbl.push_back(ParseASplit(split));

        return info; //Should move
    }

    LFOTblEntry ParseALFO(xml_node lfonode)
    {
        using namespace PrgmXML;
        LFOTblEntry lfoentry;

        for (auto node : lfonode.children())
        {
            string nodename(node.name());
            if (nodename == PROP_LFOEnabled)
                utils::parseHexaValToValue(node.value(), lfoentry.unk52);
            else if (nodename == PROP_LFOModDest)
                utils::parseHexaValToValue(node.value(), lfoentry.dest);
            else if (nodename == PROP_LFOWaveShape)
                utils::parseHexaValToValue(node.value(), lfoentry.wshape);
            else if (nodename == PROP_LFORate)
                utils::parseHexaValToValue(node.value(), lfoentry.rate);
            else if (nodename == PROP_LFOUnk29)
                utils::parseHexaValToValue(node.value(), lfoentry.unk29);
            else if (nodename == PROP_LFODepth)
                utils::parseHexaValToValue(node.value(), lfoentry.depth);
            else if (nodename == PROP_LFODelay)
                utils::parseHexaValToValue(node.value(), lfoentry.delay);
            else if (nodename == PROP_LFOUnk32)
                utils::parseHexaValToValue(node.value(), lfoentry.unk32);
            else if (nodename == PROP_LFOUnk33)
                utils::parseHexaValToValue(node.value(), lfoentry.unk33);
            else
                clog << "Unsupported lfo node \"" << nodename << "\"\n";
        }
        return lfoentry;
    }

    SplitEntry ParseASplit(xml_node splitnode)
    {
        using namespace PrgmXML;
        SplitEntry splitentry;
        for (auto node : splitnode.children())
        {
            string nodename(node.name());
            if (nodename == PROP_ID)
                utils::parseHexaValToValue(node.value(), splitentry.id);
            else if (nodename == PROP_SplitUnk11)
                utils::parseHexaValToValue(node.value(), splitentry.unk11);
            else if (nodename == PROP_SplitUnk25)
                utils::parseHexaValToValue(node.value(), splitentry.unk25);
            else if (nodename == PROP_SplitLowKey)
                utils::parseHexaValToValue(node.value(), splitentry.lowkey);
            else if (nodename == PROP_SplitHighKey)
                utils::parseHexaValToValue(node.value(), splitentry.hikey);
            else if (nodename == PROP_SplitLowVel)
                utils::parseHexaValToValue(node.value(), splitentry.lovel);
            else if (nodename == PROP_SplitHighVel)
                utils::parseHexaValToValue(node.value(), splitentry.hivel);
            else if (nodename == PROP_SplitSmplID)
                utils::parseHexaValToValue(node.value(), splitentry.smplid);
            else if (nodename == PROP_FTune)
                utils::parseHexaValToValue(node.value(), splitentry.ftune);
            else if (nodename == PROP_CTune)
                utils::parseHexaValToValue(node.value(), splitentry.ctune);
            else if (nodename == PROP_RootKey)
                utils::parseHexaValToValue(node.value(), splitentry.rootkey);
            else if (nodename == PROP_KeyTrans)
                utils::parseHexaValToValue(node.value(), splitentry.ktps);
            else if (nodename == PROP_Volume)
                utils::parseHexaValToValue(node.value(), splitentry.smplvol);
            else if (nodename == PROP_Pan)
                utils::parseHexaValToValue(node.value(), splitentry.smplpan);
            else if (nodename == PROP_SplitKGrp)
                utils::parseHexaValToValue(node.value(), splitentry.kgrpid);
            else if (nodename == PROP_EnvOn)
                utils::parseHexaValToValue(node.value(), splitentry.envon);
            else if (nodename == PROP_SplitUnk22)
                assert(false); //utils::parseHexaValToValue(node.value(), splitentry.env.unk22);
            else if (nodename == PROP_SplitUnk23)
                assert(false); //utils::parseHexaValToValue(node.value(), splitentry.env.unk23);
            else if (nodename == NODE_Envelope)
                splitentry.env = ParseAnEnvelope(node);
            else
                clog << "Unsupported split node \"" << nodename << "\"\n";
        }
        return splitentry;
    }


    void ParseSampleInfos( vector<SampleBank::smpldata_t> & sampledata, xml_document& doc)
    {
        using namespace PrgmXML;
        //Check version info and etc..
        xml_node      rootnode = doc.child(ROOT_WavInfo.c_str());
        xml_attribute dsever = rootnode.attribute(ATTR_DSE_Version.c_str());

        //#TODO: Version stuff
        const string  dsever_str = dsever.value();

        for (auto node : rootnode.children(NODE_Sample.c_str()))
            sampledata.push_back(ParseASample(node));
    }

    SampleBank::smpldata_t ParseASample(xml_node samplenode)
    {
        using namespace PrgmXML;
        SampleBank::smpldata_t smpl;
        SampleBank::wavinfoptr_t wavinf(new WavInfo);
        
        for (auto node : samplenode.children())
        {
            string nodename(node.name());

            if (nodename == PROP_ID)
                utils::parseHexaValToValue(node.value(), wavinf->id);
            else if(nodename == PROP_FTune)
                utils::parseHexaValToValue(node.value(), wavinf->ftune);
            else if (nodename == PROP_CTune)
                utils::parseHexaValToValue(node.value(), wavinf->ctune);
            else if (nodename == PROP_RootKey)
                utils::parseHexaValToValue(node.value(), wavinf->rootkey);
            else if (nodename == PROP_KeyTrans)
                utils::parseHexaValToValue(node.value(), wavinf->ktps);
            else if (nodename == PROP_Volume)
                utils::parseHexaValToValue(node.value(), wavinf->vol);
            else if (nodename == PROP_Pan)
                utils::parseHexaValToValue(node.value(), wavinf->pan);
            else if (nodename == PROP_SmplLoop)
                utils::parseValToValue(node.value(), wavinf->smplloop);
            else if (nodename == PROP_SmplRate)
                utils::parseValToValue(node.value(), wavinf->smplrate);

            //Sample loop points are considered to be in amount of "samples", and will need to be converted to match, depending on what type of sample we load
            else if (nodename == PROP_LoopBeg)
                utils::parseHexaValToValue(node.value(), wavinf->loopbeg); //#TODO: Properly translate from number of samples to number of bytes
            else if (nodename == PROP_LoopLen)
                utils::parseHexaValToValue(node.value(), wavinf->looplen); //#TODO: Properly translate from number of samples to number of bytes

            else if (nodename == PROP_EnvOn)
                utils::parseHexaValToValue(node.value(), wavinf->envon);
            else if (nodename == PROP_EnvMulti)
                utils::parseHexaValToValue(node.value(), wavinf->envmult);
            else if (nodename == PROP_EnvAtkVol)
                utils::parseHexaValToValue(node.value(), wavinf->atkvol);
            else if (nodename == PROP_EnvAtk)
                utils::parseHexaValToValue(node.value(), wavinf->attack);
            else if (nodename == PROP_EnvDecay)
                utils::parseHexaValToValue(node.value(), wavinf->decay);
            else if (nodename == PROP_EnvSustain)
                utils::parseHexaValToValue(node.value(), wavinf->sustain);
            else if (nodename == PROP_EnvHold)
                utils::parseHexaValToValue(node.value(), wavinf->hold);
            else if (nodename == PROP_EnvDecay2)
                utils::parseHexaValToValue(node.value(), wavinf->decay2);
            else if (nodename == PROP_EnvRelease)
                utils::parseHexaValToValue(node.value(), wavinf->release);
            else
                clog << "Unsupported wave node \"" << nodename << "\"\n";
        }

        smpl.pinfo_ = std::move(wavinf);
        return smpl;
    }

    void ParseKeygroups( vector<KeyGroup> & kgrp, xml_document& doc)
    {
        using namespace PrgmXML;
        //Check version info and etc..
        xml_node      rootnode = doc.child(ROOT_KeyGroups.c_str());
        xml_attribute dsever = rootnode.attribute(ATTR_DSE_Version.c_str());

        //#TODO: Version stuff
        const string  dsever_str = dsever.value();

        //Parse programs
        for (auto curnode : rootnode.children(NODE_KGrp.c_str()))
            kgrp.push_back(ParseAKeygroup(curnode));
    }

    KeyGroup ParseAKeygroup(xml_node keygroupnode)
    {
        using namespace PrgmXML;
        KeyGroup kgrp;
        for (auto node : keygroupnode.children())
        {
            string nodename = node.name();
            if (nodename == PROP_ID)
                utils::parseHexaValToValue(node.value(), kgrp.id);
            else if (nodename == NODE_KGrpPoly)
                utils::parseHexaValToValue(node.value(), kgrp.poly);
            else if (nodename == NODE_KGrpPrio)
                utils::parseHexaValToValue(node.value(), kgrp.priority);
            else if (nodename == NODE_KGrVcLow)
                utils::parseHexaValToValue(node.value(), kgrp.vclow);
            else if (nodename == NODE_KGrVcHi)
                utils::parseHexaValToValue(node.value(), kgrp.vchigh);
            else
                clog << "Unsupported keygroup node \"" << nodename << "\"\n";
        }
        return kgrp;
    }

    DSEEnvelope ParseAnEnvelope(xml_node envnode)
    {
        using namespace PrgmXML;
        DSEEnvelope env;
        for (auto node : envnode.children())
        {
            string nodename(node.name());
            if (nodename == PROP_EnvMulti)
                utils::parseHexaValToValue(node.value(), env.envmulti);
            else if (nodename == PROP_EnvAtkVol)
                utils::parseHexaValToValue( node.value(), env.atkvol);
            else if (nodename == PROP_EnvAtk)
                utils::parseHexaValToValue(node.value(), env.attack);
            else if (nodename == PROP_EnvDecay)
                utils::parseHexaValToValue(node.value(), env.decay);
            else if (nodename == PROP_EnvSustain)
                utils::parseHexaValToValue(node.value(), env.sustain);
            else if (nodename == PROP_EnvHold)
                utils::parseHexaValToValue(node.value(), env.hold);
            else if (nodename == PROP_EnvDecay2)
                utils::parseHexaValToValue(node.value(), env.decay2);
            else if (nodename == PROP_EnvRelease)
                utils::parseHexaValToValue(node.value(), env.release);
            else
                clog << "Unsupported envelope node \"" << nodename << "\"\n";
        }
        return env;
    }

private:
    string m_path;

};

//====================================================================================================
//  ProgramBankXMLWriter
//====================================================================================================
class ProgramBankXMLWriter
{
public:
    ProgramBankXMLWriter( const PresetBank & presbnk )
        :m_presbnk(presbnk)
    {
    }

    void Write( const std::string & destdir )
    {
        WriteWavInfo(destdir);
        WritePrograms(destdir);
        WriteKeyGroups(destdir);
    }

private:

    void WritePrograms( const std::string & destdir )
    {
        using namespace PrgmXML;
        xml_document  doc;
        xml_node      prgmsnode = doc.append_child( ROOT_Programs.c_str() );
        auto          ptrprgms  = m_presbnk.prgmbank().lock(); 

        AppendAttribute(prgmsnode, ATTR_DSE_Version, DseVerToInt(m_presbnk.metadata().origversion));
        AppendAttribute(prgmsnode, ATTR_DSE_Unk1,    m_presbnk.metadata().unk1);
        AppendAttribute(prgmsnode, ATTR_DSE_Unk2,    m_presbnk.metadata().unk2);
        AppendAttribute(prgmsnode, ATTR_DSE_Unk17,   m_presbnk.metadata().unk17);

        if( ptrprgms != nullptr )
        {
            for( const auto & aprog : ptrprgms->PrgmInfo() )
            {
                if( aprog != nullptr )
                    WriteAProgram( prgmsnode, *aprog );
            }

            stringstream sstrfname;
            sstrfname << utils::TryAppendSlash(destdir) << DEF_ProgramsFname;
            if( ! doc.save_file( sstrfname.str().c_str() ) )
                throw std::runtime_error("Can't write xml file " + sstrfname.str());
        }
        else
            clog << "ProgramBankXMLWriter::WritePrograms(): no program data available!";
    }

    void WriteAProgram( xml_node & parent, const ProgramInfo & curprog )
    {
        using namespace PrgmXML;

        WriteCommentNode( parent, "ProgramID : " + std::to_string( curprog.id ) );
        xml_node prgnode = parent.append_child( NODE_Program.c_str() );

        //Write program header stuff
        WriteNodeWithValue( prgnode, PROP_ID,           curprog.id );
        WriteNodeWithValue( prgnode, PROP_Volume,       curprog.prgvol );
        WriteNodeWithValue( prgnode, PROP_Pan,          curprog.prgpan );
        WriteNodeWithValue( prgnode, PROP_PrgmUnkPoly,  curprog.unkpoly );
        WriteNodeWithValue( prgnode, PROP_PrgmUnk4,     curprog.unk4);

        WriteCommentNode( prgnode, "LFO Settings" );
        {
            xml_node lfotblnode = prgnode.append_child( NODE_LFOTable.c_str() );
            for( const auto & lfoentry : curprog.m_lfotbl )
                WriteALFO( lfotblnode, lfoentry );
        }

        WriteCommentNode( prgnode, "Splits" );
        {
            xml_node splittblnode = prgnode.append_child( NODE_SplitTable.c_str() );
            for( const auto & splitentry : curprog.m_splitstbl )
                WriteASplit( splittblnode, splitentry );
        }
        
    }

    void WriteALFO( xml_node & parent, const LFOTblEntry & curlfo )
    {
        using namespace PrgmXML;

        if( curlfo.isLFONonDefault() )
        {
            stringstream sstrunkcv;

            WriteNodeWithValue( parent, PROP_LFOEnabled,    curlfo.unk52  );
            WriteNodeWithValue( parent, PROP_LFOModDest,    curlfo.dest   );
            WriteNodeWithValue( parent, PROP_LFOWaveShape,  curlfo.wshape );
            WriteNodeWithValue( parent, PROP_LFORate,       curlfo.rate   );

            //unk29 is different
            sstrunkcv <<hex <<showbase <<curlfo.unk29;
            parent.append_child(PROP_LFOUnk29.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );

            WriteNodeWithValue( parent, PROP_LFODepth,      curlfo.depth  );
            WriteNodeWithValue( parent, PROP_LFODelay,      curlfo.delay  );
            WriteNodeWithValue( parent, PROP_LFOUnk32,      curlfo.unk32  );
            WriteNodeWithValue( parent, PROP_LFOUnk33,      curlfo.unk33  );
        }
    }

    void WriteASplit( xml_node & parent, const SplitEntry & cursplit )
    {
        using namespace PrgmXML;
        WriteCommentNode( parent, "Split Sample " + to_string(cursplit.smplid) );

        WriteNodeWithValue( parent, PROP_ID,                cursplit.id );
        WriteNodeWithValue( parent, PROP_SplitUnk11,        cursplit.unk11 );
        WriteNodeWithValue( parent, PROP_SplitUnk25,        cursplit.unk25 );
        WriteNodeWithValue( parent, PROP_SplitLowKey,       cursplit.lowkey );
        WriteNodeWithValue( parent, PROP_SplitHighKey,      cursplit.hikey );
        WriteNodeWithValue( parent, PROP_SplitLowVel,       cursplit.lovel );
        WriteNodeWithValue( parent, PROP_SplitHighVel,      cursplit.hivel );
        WriteNodeWithValue( parent, PROP_SplitSmplID,       cursplit.smplid );
        WriteNodeWithValue( parent, PROP_FTune,             cursplit.ftune );
        WriteNodeWithValue( parent, PROP_CTune,             cursplit.ctune );
        WriteNodeWithValue( parent, PROP_RootKey,           cursplit.rootkey );
        WriteNodeWithValue( parent, PROP_KeyTrans,          cursplit.ktps );
        WriteNodeWithValue( parent, PROP_Volume,            cursplit.smplvol );
        WriteNodeWithValue( parent, PROP_Pan,               cursplit.smplpan );
        WriteNodeWithValue( parent, PROP_SplitKGrp,         cursplit.kgrpid );
        //WriteNodeWithValue( parent, PROP_SplitUnk22,        cursplit.unk22 );
        //WriteNodeWithValue( parent, PROP_SplitUnk23,        cursplit.unk23 );
        WriteNodeWithValue( parent, PROP_EnvOn,             cursplit.envon );
        WriteEnvelope(parent, cursplit.env);

        //stringstream sstrunkcv;

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(cursplit.unk37);
        //parent.append_child(PROP_EnvUnk19.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(cursplit.unk38);
        //parent.append_child(PROP_EnvUnk20.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(cursplit.unk39);
        //parent.append_child(PROP_EnvUnk21.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(cursplit.unk40);
        //parent.append_child(PROP_EnvUnk22.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());
    }

    void WriteEnvelope(xml_node parent, const DSEEnvelope& envelope)
    {
        using namespace PrgmXML;
        xml_node envnode = parent.append_child(NODE_Envelope.c_str());
        WriteNodeWithValue(envnode, PROP_EnvMulti,   envelope.envmulti);
        WriteNodeWithValue(envnode, PROP_EnvAtkVol,  envelope.atkvol);
        WriteNodeWithValue(envnode, PROP_EnvAtk,     envelope.attack);
        WriteNodeWithValue(envnode, PROP_EnvDecay,   envelope.decay);
        WriteNodeWithValue(envnode, PROP_EnvSustain, envelope.sustain);
        WriteNodeWithValue(envnode, PROP_EnvHold,    envelope.hold);
        WriteNodeWithValue(envnode, PROP_EnvDecay2,  envelope.decay2);
        WriteNodeWithValue(envnode, PROP_EnvRelease, envelope.release);
    }

    void WriteKeyGroups( const std::string & destdir )
    {
        using namespace PrgmXML;
        xml_document doc;
        xml_node     kgrpsnode = doc.append_child( ROOT_KeyGroups.c_str() );
        auto         ptrprgms  = m_presbnk.prgmbank().lock(); 

        if( ptrprgms != nullptr )
        {
            for( const auto & grp : ptrprgms->Keygrps() )
                WriteKeyGroup( kgrpsnode, grp );

            stringstream sstrfname;
            sstrfname << utils::TryAppendSlash(destdir) << DEF_KeygroupFname;
            if( ! doc.save_file( sstrfname.str().c_str() ) )
                throw std::runtime_error("Can't write xml file " + sstrfname.str());
        }
        else
            clog << "ProgramBankXMLWriter::WriteKeyGroups(): no program/keygroup data available!";
    }

    void WriteKeyGroup( xml_node & parent, const KeyGroup & grp )
    {
        using namespace PrgmXML;

        WriteCommentNode( parent, "Keygroup ID : " + std::to_string( grp.id ) );
        xml_node kgrpnode = parent.append_child( NODE_KGrp.c_str() );

        WriteNodeWithValue( kgrpnode, PROP_ID,        grp.id );
        WriteNodeWithValue( kgrpnode, NODE_KGrpPoly,  grp.poly );
        WriteNodeWithValue( kgrpnode, NODE_KGrpPrio,  grp.priority );
        WriteNodeWithValue( kgrpnode, NODE_KGrVcLow,  grp.vclow );
        WriteNodeWithValue( kgrpnode, NODE_KGrVcHi,   grp.vchigh );
    }


    void WriteWavInfo( const std::string & destdir )
    {
        using namespace PrgmXML;
        auto ptrwavs = m_presbnk.smplbank().lock();

        if( ptrwavs != nullptr )
        {
            xml_document doc;
            xml_node     infonode = doc.append_child( ROOT_WavInfo.c_str() );

            for( size_t cptwav = 0; cptwav < ptrwavs->NbSlots(); ++cptwav )
            {
                auto ptrwinf = ptrwavs->sampleInfo(cptwav);
                if( ptrwinf != nullptr )
                    WriteAWav( infonode, *ptrwinf );
            }

            stringstream sstrfname;
            sstrfname << utils::TryAppendSlash(destdir) << DEF_WavInfoFname;
            if( ! doc.save_file( sstrfname.str().c_str() ) )
                throw std::runtime_error("Can't write xml file " + sstrfname.str());
        }
        else
            clog << "ProgramBankXMLWriter::WriteWavInfo(): No Sample Data";
    }

    void WriteAWav( xml_node & parent, const WavInfo & winfo )
    {
        using namespace PrgmXML;
        WriteCommentNode( parent, "Sample #" + std::to_string( winfo.id ) );

        xml_node infonode = parent.append_child( NODE_Sample.c_str() );
        WriteNodeWithValue( infonode, PROP_ID,          winfo.id ); 
        WriteCommentNode( infonode, "Tuning Data" );
        WriteNodeWithValue( infonode, PROP_FTune,       winfo.ftune );
        WriteNodeWithValue( infonode, PROP_CTune,       winfo.ctune );
        WriteNodeWithValue( infonode, PROP_RootKey,     winfo.rootkey );
        WriteNodeWithValue( infonode, PROP_KeyTrans,    winfo.ktps );
        WriteCommentNode( infonode, "Misc" );
        WriteNodeWithValue( infonode, PROP_Volume,      winfo.vol );
        WriteNodeWithValue( infonode, PROP_Pan,         winfo.pan );
        //WriteNodeWithValue( infonode, PROP_SmplUnk5,    winfo.unk5 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk58,   winfo.unk58 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk6,    winfo.unk6 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk59,   winfo.unk59 );

        //WriteNodeWithValue( infonode, PROP_SmplUnk9,    winfo.unk9 );
        WriteNodeWithValue( infonode, PROP_SmplLoop,    winfo.smplloop );
        WriteNodeWithValue( infonode, PROP_SmplRate,    winfo.smplrate );
        //WriteNodeWithValue( infonode, PROP_SmplUnk12,   winfo.unk12 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk62,   winfo.unk62 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk13,   winfo.unk13 );

        WriteCommentNode( infonode, "Loop Data (in PCM16 sample points)" );
        //Correct the sample loop points because the samples were exported to PCM16
        if( winfo.smplfmt == eDSESmplFmt::pcm8 )
        {
            WriteNodeWithValue( infonode, PROP_LoopBeg,     winfo.loopbeg * 2 );
            WriteNodeWithValue( infonode, PROP_LoopLen,     winfo.looplen * 2 );
        }
        else if( winfo.smplfmt == eDSESmplFmt::ima_adpcm )
        {
            WriteNodeWithValue( infonode, PROP_LoopBeg,     (winfo.loopbeg * 4) + ::audio::IMA_ADPCM_PreambleLen );
            WriteNodeWithValue( infonode, PROP_LoopLen,     (winfo.looplen * 4) + ::audio::IMA_ADPCM_PreambleLen );
        }
        else
        {
            WriteNodeWithValue( infonode, PROP_LoopBeg,     winfo.loopbeg );
            WriteNodeWithValue( infonode, PROP_LoopLen,     winfo.looplen );
        }

        WriteCommentNode( infonode, "Volume Envelope" );
        WriteNodeWithValue( infonode, PROP_EnvOn,         winfo.envon );
        WriteNodeWithValue( infonode, PROP_EnvMulti,      winfo.envmult );

        //stringstream sstrunkcv;

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(winfo.unk19);
        //infonode.append_child(PROP_EnvUnk19.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(winfo.unk20);
        //infonode.append_child(PROP_EnvUnk20.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(winfo.unk21);
        //infonode.append_child(PROP_EnvUnk21.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(winfo.unk22);
        //infonode.append_child(PROP_EnvUnk22.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        
        WriteNodeWithValue( infonode, PROP_EnvAtkVol,     winfo.atkvol );
        WriteNodeWithValue( infonode, PROP_EnvAtk,        winfo.attack );
        WriteNodeWithValue( infonode, PROP_EnvDecay,      winfo.decay );
        WriteNodeWithValue( infonode, PROP_EnvSustain,    winfo.sustain );
        WriteNodeWithValue( infonode, PROP_EnvHold,       winfo.hold );
        WriteNodeWithValue( infonode, PROP_EnvDecay2,     winfo.decay2 );
        WriteNodeWithValue( infonode, PROP_EnvRelease,    winfo.release );
        //WriteNodeWithValue( infonode, PROP_EnvUnk57,      winfo.unk57 );

    }

private:
    const PresetBank & m_presbnk;
};

//====================================================================================================
//  
//====================================================================================================

    /*
        PresetBankToXML
            Write the 3 XML files for a given set of presets and samples.
    */
    void PresetBankToXML( const DSE::PresetBank & srcbnk, const std::string & destdir )
    {
        ProgramBankXMLWriter(srcbnk).Write(destdir);
    }

    /*
        XMLToPresetBank
            Read the 3 XML files for a given set of presets and samples.
    */
    DSE::PresetBank XMLToPresetBank( const std::string & srcdir )
    {
        return ProgramBankXMLParser(srcdir).Parse();
    }
};
