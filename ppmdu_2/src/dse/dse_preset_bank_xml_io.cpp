#include <dse/dse_conversion.hpp>
#include <dse/dse_to_xml.hpp>
#include <dse/dse_common.hpp>

#include <ext_fmts/adpcm.hpp>

#include <utils/parse_utils.hpp>
#include <utils/pugixml_utils.hpp>

#include <pugixml.hpp>

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
    namespace PrgmXML
    {
        //Root nodes
        const string ROOT_Programs     = "Programs"s;
        const string ROOT_WavInfo      = "WavInfo"s;

        //Attributes
        const string ATTR_DSE_Unk17    = "dse_unk17"s;
        const string ATTR_ID           = "id"s;
        const string ATTR_Volume       = "vol"s;
        const string ATTR_Pan          = "pan"s;
        const string ATTR_PrgmUnkPoly  = "unkpoly"s;
        const string ATTR_PrgmUnk4     = "unk4"s;
        const string ATTR_SmplID       = "sample_id";

        const string ATTR_KGrp         = "keygroup"s;
        const string ATTR_FTune        = "fine_tune"s;
        const string ATTR_CTune        = "coarse_tune"s;
        const string ATTR_RootKey      = "root_key"s;
        const string ATTR_KeyTrans     = "transpose"s;
        const string ATTR_BendRng      = "bend_range"s;
        const string ATTR_On           = "is_on"s;


        //Common
        const string PROP_Volume       = "Volume"s;
        const string PROP_Pan          = "Pan"s;
        const string PROP_ID           = "ID"s;
        const string PROP_FTune        = "FineTune"s;
        const string PROP_CTune        = "CoarseTune"s;
        const string PROP_RootKey      = "RootKey"s;
        const string PROP_KeyTrans     = "KeyTransposition"s;
        const string PROP_BendRng      = "BendRange"s;

        const string NODE_Keys         = "KeyRange"s;
        const string NODE_Loop         = "Loop"s;

        //Nodes
        const string NODE_Program      = "Program"s;
        const string NODE_LFOTable     = "LFOTable"s;

        const string NODE_LFOEntry     = "LFOEntry"s;
        const string ATTR_LFOEnabled   = "is_on"s;
        const string ATTR_LFOModDest   = "mod_dest"s;
        const string ATTR_LFOWaveShape = "shape"s;
        const string ATTR_LFORate      = "rate"s;
        const string ATTR_LFOUnk29     = "unk29"s;
        const string ATTR_LFODepth     = "depth"s;
        const string ATTR_LFODelay     = "delay"s;
        const string ATTR_LFOUnk32     = "unk32"s;
        const string ATTR_LFOUnk33     = "unk33"s;

        const string NODE_SplitTable   = "SplitTable"s;

        const string NODE_Split        = "Split"s;
        const string PROP_SplitUnk25   = "Unk25"s;
        const string PROP_SplitUnk22   = "Unk22"s;
        const string PROP_SplitUnk23   = "Unk23"s;
        const string PROP_LowKey       = "LowKey"s;
        const string PROP_HighKey      = "HighKey"s;
        const string PROP_LowVel       = "MinVelocity"s;
        const string PROP_HighVel      = "MaxVelocity"s;
        
        const string NODE_KeyGroups    = "KeyGroups"s;
        const string NODE_KGrp         = "KeyGroup"s;
        const string ATTR_KGrpPoly     = "max_polyphony"s;
        const string ATTR_KGrpPrio     = "priority"s;
        const string ATTR_KGrVcLow     = "voice_chan_low"s;
        const string ATTR_KGrVcHi      = "voice_chan_high"s;
        const string ATTR_KGrUnk50     = "unk50";
        const string ATTR_KGrUnk51     = "unk51";

        const string NODE_Sample       = "Sample"s;
        const string PROP_SmplUnk5     = "Unk5"s;
        const string PROP_SmplUnk58    = "Unk58"s;
        const string PROP_SmplUnk6     = "Unk6"s;
        const string PROP_SmplUnk7     = "Unk7"s;
        const string PROP_SmplUnk59    = "Unk59"s;
        const string PROP_SmplFmt      = "SampleFormat"s;
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
        
        const string ATTR_EnvMulti     = "multiplier"s;
        const string ATTR_EnvUnk19     = "unk19"s;
        const string ATTR_EnvUnk20     = "unk20"s;
        const string ATTR_EnvUnk21     = "unk21"s;
        const string ATTR_EnvUnk22     = "unk22"s;

        const string PROP_EnvAtkVol    = "AttackLevel"s;
        const string PROP_EnvAtk       = "Attack"s;
        const string PROP_EnvHold      = "Hold"s;
        const string PROP_EnvDecay     = "Decay"s;
        const string PROP_EnvSustain   = "Sustain"s;
        const string PROP_EnvDecay2    = "FadeOut"s;
        const string PROP_EnvRelease   = "Release"s;
        const string PROP_EnvUnk57     = "Unk57"s;
    };

    //Write the envelope's data to a XML node
    xml_node WriteDSEEnvelope(xml_node & parent, const DSEEnvelope & envelope, bool envon)
    {
        using namespace PrgmXML;
        xml_node envnode = parent.append_child(NODE_Envelope.c_str());
        AppendAttribute(envnode, ATTR_On,       envon);
        AppendAttribute(envnode, ATTR_EnvMulti, envelope.envmulti);

        WriteNodeWithValue(envnode, PROP_EnvAtkVol,  envelope.atkvol);
        WriteNodeWithValue(envnode, PROP_EnvAtk,     envelope.attack);
        WriteNodeWithValue(envnode, PROP_EnvDecay,   envelope.decay);
        WriteNodeWithValue(envnode, PROP_EnvSustain, envelope.sustain);
        WriteNodeWithValue(envnode, PROP_EnvHold,    envelope.hold);
        WriteNodeWithValue(envnode, PROP_EnvDecay2,  envelope.decay2);
        WriteNodeWithValue(envnode, PROP_EnvRelease, envelope.release);
        return envnode;
    }

    //Read the envelope's data from an xml node
    DSEEnvelope ParseDSEEnvelope(const xml_node& envnode, uint8_t& out_envon)
    {
        using namespace PrgmXML;
        DSEEnvelope env;
        for (xml_attribute node : envnode.attributes())
        {
            const string nodename(node.name());

            if(nodename == ATTR_On)
                out_envon = node.as_bool();
            else if (nodename == ATTR_EnvMulti)
                env.envmulti = ClampInt8(node.as_int());
            else
                clog << "Unsupported envelope attribute \"" << nodename << "\"\n";
        }

        for (xml_node node : envnode.children())
        {
            const string   nodename(node.name());
            const xml_text value = node.text();

            if (nodename == PROP_EnvAtkVol)
                env.atkvol = ClampDSEVolume(value.as_int());
            else if (nodename == PROP_EnvAtk)
                env.attack = ClampInt8From0(value.as_int());
            else if (nodename == PROP_EnvDecay)
                env.decay = ClampInt8From0(value.as_int());
            else if (nodename == PROP_EnvSustain)
                env.sustain = ClampDSEVolume(value.as_int());
            else if (nodename == PROP_EnvHold)
                env.hold = ClampInt8From0(value.as_int());
            else if (nodename == PROP_EnvDecay2)
                env.decay2 = ClampInt8From0(value.as_int());
            else if (nodename == PROP_EnvRelease)
                env.release = ClampInt8From0(value.as_int());
            else
                clog << "Unsupported envelope node \"" << nodename << "\"\n";
        }
        return env;
    }

//====================================================================================================
//  PresetBankXMLParser
//====================================================================================================
class PresetBankXMLParser
{
public:
    PresetBankXMLParser(const std::string& bnkxmlfile)
        :m_path(bnkxmlfile)
    {}

    PresetBank Parse()
    {
        vector< unique_ptr<ProgramInfo> > prgmbank;
        vector<KeyGroup>                  kgrp;
        vector<SampleBank::SampleBlock>   sampledata;
        DSE_MetaDataSWDL                  meta;
        try
        {
            //Load the xml file
            Poco::Path       programpath(m_path);
            xml_document     doc;
            xml_parse_result loadres;
            string           transcoded_path = Poco::Path::transcode(programpath.absolute().toString());
            
            meta.fname     = Poco::Path::transcode(programpath.getBaseName());
            meta.origfname = meta.fname;
            meta.createtime.SetTimeToNow();

            if ((loadres = doc.load_file(transcoded_path.c_str())))
            {
                ParseDSEXmlNode(doc, &meta);
                ParseSampleInfos(sampledata, doc);
                ParsePrograms(prgmbank, doc);
                ParseKeygroups(kgrp, doc);
            }
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
            sstr << "Got Exception while parsing XML \"" << m_path << "\" : " << e.what();
            throw runtime_error(sstr.str());
        }

        return move( PresetBank( move(meta),
                                 unique_ptr<ProgramBank>(new ProgramBank(move(prgmbank), 
                                                                              move(kgrp))),
                                 unique_ptr<SampleBank>(new SampleBank(move(sampledata))) 
                                 ) );
    }

private:

    void ParsePrograms( vector< unique_ptr<ProgramInfo> > & prgmbank, xml_document & doc)
    {
        using namespace PrgmXML;
        //Check version info and etc..
        xml_node rootnode = doc.child(ROOT_Programs.c_str());
        //Parse programs
        for (auto curnode : rootnode.children(NODE_Program.c_str()))
        {
            std::unique_ptr<ProgramInfo> info = ParseAProgram(curnode);
            if (info->id >= prgmbank.size())
                prgmbank.resize(info->id + 1); //Program info in the table have to be inserted at the index matching their ID!
            prgmbank[info->id] = std::move(info);
        }
    }

    //Parse a single program from the xml document
    unique_ptr<ProgramInfo> ParseAProgram(xml_node programnode)
    {
        using namespace PrgmXML;
        unique_ptr<ProgramInfo> info(new ProgramInfo);

        //Read program data
        for (xml_attribute entry : programnode.attributes())
        {
            const string nodename(entry.name());

            if (nodename == ATTR_ID)
                info->id = ClampUInt16(entry.as_uint());
            else if (nodename == ATTR_Volume)
                info->prgvol = ClampDSEVolume(entry.as_int());
            else if (nodename == ATTR_Pan)
                info->prgpan = ClampDSEPan(entry.as_int());
            else if (nodename == ATTR_PrgmUnkPoly)
                info->unkpoly = ClampUInt8(entry.as_uint());
            else if (nodename == ATTR_PrgmUnk4)
                info->unk4 = ClampUInt16(entry.as_uint());
            else
                clog << "Unsupported program data node \"" << nodename << "\"\n";
        }

        //Grab subnodes
        xml_node lfotbl = programnode.child(NODE_LFOTable.c_str());
        for (xml_node lfo : lfotbl)
        {
            info->m_lfotbl.push_back(ParseALFO(lfo));
        }

        int cntsplits = 0;
        xml_node splittbl = programnode.child(NODE_SplitTable.c_str());
        for (xml_node split : splittbl.children(NODE_Split.c_str()))
        {
            SplitEntry spl = ParseASplit(split);
            if ((spl.id - cntsplits) > 1)
            {
                clog << "<!!!!>- Found a split with an id that was out of order! " << spl.id << ", but expected " << (cntsplits + 1) << "\n";
#ifdef _DEBUG
                assert(false);
#endif
            }
            cntsplits = spl.id;
            if (spl.id >= info->m_splitstbl.size())
                info->m_splitstbl.resize(spl.id + 1);
            info->m_splitstbl[spl.id] = std::move(spl);
        }

        return info; //Should move
    }

    uint8_t parseLfoDest(xml_attribute attr)
    {
        //#TODO: Implement enum checks here to validate destination
        return  (uint8_t)std::clamp(attr.as_uint(), (unsigned int)numeric_limits<uint8_t>::min(), (unsigned int)numeric_limits<uint8_t>::max());
    }

    uint8_t parseLfoWaveShape(xml_attribute attr)
    {
        //#TODO: Implement enum checks here to validate destination
        return  (uint8_t)std::clamp(attr.as_uint((unsigned int)LFOTblEntry::eLFOWaveShape::Square), (unsigned int)numeric_limits<uint8_t>::min(), (unsigned int)numeric_limits<uint8_t>::max());
    }

    LFOTblEntry ParseALFO(xml_node lfonode)
    {
        using namespace PrgmXML;
        LFOTblEntry lfoentry;

        for (xml_attribute node : lfonode.attributes())
        {
            const string nodename(node.name());

            if (nodename == ATTR_LFOEnabled)
                lfoentry.unk52 = ClampUInt8(node.as_uint());
            else if (nodename == ATTR_LFOModDest)
                lfoentry.dest = parseLfoDest(node);
            else if (nodename == ATTR_LFOWaveShape)
                lfoentry.wshape = parseLfoWaveShape(node);
            else if (nodename == ATTR_LFORate)
                lfoentry.rate = ClampUInt16(node.as_uint());
            else if (nodename == ATTR_LFOUnk29)
                lfoentry.unk29 = ClampUInt16(node.as_uint());
            else if (nodename == ATTR_LFODepth)
                lfoentry.depth = ClampUInt16(node.as_uint());
            else if (nodename == ATTR_LFODelay)
                lfoentry.delay = ClampUInt16(node.as_uint());
            else if (nodename == ATTR_LFOUnk32)
                lfoentry.unk32 = ClampUInt16(node.as_uint());
            else if (nodename == ATTR_LFOUnk33)
                lfoentry.unk33 = ClampUInt16(node.as_uint());
            else
                clog << "Unsupported lfo node \"" << nodename << "\"\n";
        }
        return lfoentry;
    }

    SplitEntry ParseASplit(xml_node splitnode)
    {
        using namespace PrgmXML;
        SplitEntry splitentry;

        //Split's Attributes
        for (xml_attribute node : splitnode.attributes())
        {
            const string nodename(node.name());
            if (nodename == ATTR_ID)
                splitentry.id = (uint8_t)node.as_uint();
            else if (nodename == ATTR_SmplID)
                splitentry.smplid = (sampleid_t)node.as_uint();
            else
                clog << "Unsupported split attribute \"" << nodename << "\"\n";
        }

        //Split's child nodes
        for (xml_node node : splitnode.children())
        {
            const string nodename(node.name());
            const string value = node.text().get();
            if (nodename == NODE_Envelope)
            {
                splitentry.env = ParseDSEEnvelope(node, splitentry.envon);
            }
            else if (nodename == NODE_Sample)
            {
                xml_attribute smplid = node.attribute(ATTR_SmplID.c_str());
                if (!smplid)
                    throw std::runtime_error("A sample ID hasn't been defined in the xml for a split entry, from the xml file: \"" + m_path + "\"");
                utils::parseHexaValToValue(smplid.value(), splitentry.smplid);
                splitentry.smplid = (sampleid_t)smplid.as_uint();

                for (xml_node samplenode : node.children())
                {
                    const string samplenodename = samplenode.name();
                    const xml_text smplnodeval = samplenode.text();

                    if (samplenodename == PROP_Volume)
                        splitentry.smplvol = ClampDSEVolume(smplnodeval.as_int());
                    else if(samplenodename == PROP_Pan)
                        splitentry.smplpan = ClampDSEPan(smplnodeval.as_int());
                    else if (samplenodename == PROP_BendRng)
                        splitentry.bendrange = ClampUInt8(smplnodeval.as_uint());
                    else if (samplenodename == PROP_RootKey)
                        splitentry.rootkey = ClampInt8From0(smplnodeval.as_int());
                    else if (samplenodename == PROP_FTune)
                        splitentry.ftune = ClampInt8(smplnodeval.as_int());
                    else if (samplenodename == PROP_CTune)
                        splitentry.ctune = ClampInt8(smplnodeval.as_int());
                    else if (samplenodename == PROP_KeyTrans)
                        splitentry.ktps = ClampInt8(smplnodeval.as_int());
                }
            }
            else if (nodename == NODE_Keys)
            {
                for (xml_node keynode : node.children())
                {
                    const string   keynodename = keynode.name();
                    const xml_text keynodeval = keynode.text();

                    if (keynodename == PROP_LowKey)
                    {
                        splitentry.lowkey = ClampInt8From0(keynodeval.as_int());
                        splitentry.lowkey2 = splitentry.lowkey;
                    }
                    else if (keynodename == PROP_HighKey)
                    {
                        splitentry.hikey = ClampInt8From0(keynodeval.as_int());
                        splitentry.hikey2 = splitentry.hikey;
                    }
                    else if (keynodename == PROP_LowVel)
                    {
                        splitentry.lovel = ClampInt8From0(keynodeval.as_int());
                        splitentry.lovel2 = splitentry.lovel;
                    }
                    else if (keynodename == PROP_HighVel)
                    {
                        splitentry.hivel = ClampInt8From0(keynodeval.as_int());
                        splitentry.hivel2 = splitentry.hivel;
                    }
                    else if (keynodename == NODE_KGrp)
                        splitentry.kgrpid = (uint8_t)keynodeval.as_uint();
                }
            }
            else if (nodename == PROP_SplitUnk25)
                utils::parseHexaValToValue(value, splitentry.unk25);
            else
                clog << "Unsupported split node \"" << nodename << "\"\n";
        }
        return splitentry;
    }


    void ParseSampleInfos( vector<SampleBank::SampleBlock> & sampledata, xml_document& doc)
    {
        using namespace PrgmXML;
        //Check version info and etc..
        xml_node      rootnode = doc.child(ROOT_WavInfo.c_str());

        for (auto node : rootnode.children(NODE_Sample.c_str()))
        {
            SampleBank::SampleBlock blk = ParseASample(node);
            const sampleid_t sampleid = blk.pinfo_->id;
            if (sampleid >= sampledata.size())
                sampledata.resize(sampleid + 1); //Sample info in the table have to be inserted at the index matching their ID!
            sampledata[sampleid] = std::move(blk);
        }
    }

    SampleBank::SampleBlock ParseASample(xml_node samplenode)
    {
        using namespace PrgmXML;
        using std::numeric_limits;
        SampleBank::SampleBlock smpl;
        SampleBank::wavinfoptr_t wavinf(new WavInfo);
        utils::parseHexaValToValue(samplenode.attribute(ATTR_ID.c_str()).value(), wavinf->id);

        for (xml_node node : samplenode.children())
        {
            const string nodename(node.name());
            const xml_text value = node.text();

            if (nodename == PROP_FTune)
                wavinf->ftune = ClampInt8(value.as_int());
            else if (nodename == PROP_CTune)
                wavinf->ctune = ClampInt8(value.as_int());
            else if (nodename == PROP_RootKey)
                wavinf->rootkey = ClampInt8From0(value.as_uint());
            else if (nodename == PROP_KeyTrans)
                wavinf->ktps = ClampInt8(value.as_int());
            else if (nodename == PROP_Volume)
                wavinf->vol = ClampDSEVolume(value.as_int(DSE_DefaultVol));
            else if (nodename == PROP_Pan)
                wavinf->pan = ClampDSEPan(value.as_int(DSE_DefaultPan));
            else if (nodename == PROP_SmplRate)
                wavinf->smplrate = value.as_uint();
            else if (nodename == PROP_SmplFmt)
                wavinf->smplfmt = StringToDseSmplFmt(value.as_string());
            else if (nodename == NODE_Loop)
            {
                wavinf->smplloop = node.attribute(ATTR_On.c_str()).as_bool();
                for (xml_node loopnode : node.children())
                {
                    const string nodename(loopnode.name());
                    const xml_text loopvalue = loopnode.text();
                    if (nodename == PROP_LoopBeg)
                        wavinf->loopbeg = loopvalue.as_uint();
                    else if (nodename == PROP_LoopLen)
                        wavinf->looplen = loopvalue.as_uint();
                }
            }
            else if (nodename == NODE_Envelope)
                wavinf->env = ParseDSEEnvelope(node, wavinf->envon);
            else
                clog << "Unsupported wave node \"" << nodename << "\"\n";
        }

        smpl.pinfo_ = std::move(wavinf);
        return smpl;
    }

    void ParseKeygroups( vector<KeyGroup> & kgrps, xml_document& doc)
    {
        using namespace PrgmXML;
        //Check version info and etc..
        xml_node rootnode = doc.child(NODE_KeyGroups.c_str());

        //Parse keygroups
        for (xml_node curnode : rootnode.children(NODE_KGrp.c_str()))
        {
            KeyGroup kg = ParseAKeygroup(curnode);
            if(kg.id >= kgrps.size())
                kgrps.resize(kg.id + 1); //Keygroup nodes must be inserted at their id in the table
            kgrps[kg.id] = std::move(kg);
        }
    }

    KeyGroup ParseAKeygroup(xml_node keygroupnode)
    {
        using namespace PrgmXML;
        using std::numeric_limits;
        KeyGroup kgrp;

        for (xml_attribute attr : keygroupnode.attributes())
        {
            const string nodename = attr.name();
            if (nodename == ATTR_ID)
                kgrp.id = (uint16_t)attr.as_uint();
            else if (nodename == ATTR_KGrpPoly)
                kgrp.poly = ClampUInt8(attr.as_uint());
            else if (nodename == ATTR_KGrpPrio)
                kgrp.priority = ClampUInt8(attr.as_uint());
            else if (nodename == ATTR_KGrVcLow)
                kgrp.vclow = ClampUInt8(attr.as_uint());
            else if (nodename == ATTR_KGrVcHi)
                kgrp.vchigh = ClampUInt8(attr.as_uint());
            else if (nodename == ATTR_KGrUnk50)
                kgrp.unk50 = ClampUInt8(attr.as_uint());
            else if (nodename == ATTR_KGrUnk51)
                kgrp.unk51 = ClampUInt8(attr.as_uint());
            else
                clog << "Unsupported keygroup attribute \"" << nodename << "\"\n";
        }

        return kgrp;
    }
private:
    string m_path;

};

//====================================================================================================
//  PresetBankXMLWriter
//====================================================================================================
class PresetBankXMLWriter
{
public:
    PresetBankXMLWriter( const PresetBank & presbnk )
        :m_presbnk(presbnk)
    {}

    void Write(const std::string & destpath)
    {
        xml_document doc;
        WriteDSEXmlNode(doc, &m_presbnk.metadata());
        WriteWavInfo(doc);
        WritePrograms(doc);
        WriteKeyGroups(doc);

        if (!doc.save_file(destpath.c_str()))
            throw std::runtime_error("Can't write xml file " + destpath);
    }

private:

    void WritePrograms(xml_document & doc)
    {
        using namespace PrgmXML;
        xml_node prgmsnode = doc.append_child( ROOT_Programs.c_str() );
        auto     ptrprgms  = m_presbnk.prgmbank().lock(); 

        if (ptrprgms == nullptr)
        {
            clog << "PresetBankXMLWriter::WritePrograms(): no program data available!\n";
            return;
        }

        for( const ProgramBank::ptrprg_t & aprog : ptrprgms->PrgmInfo() )
        {
            if( aprog != nullptr )
                WriteAProgram( prgmsnode, *aprog );
        }
    }

    void WriteAProgram( xml_node & parent, const ProgramInfo & curprog )
    {
        using namespace PrgmXML;

        WriteCommentNode( parent, "ProgramID : " + std::to_string( curprog.id ) );
        xml_node prgnode = parent.append_child( NODE_Program.c_str() );

        //Write program header stuff
        AppendAttribute( prgnode, ATTR_ID,           curprog.id );
        AppendAttribute( prgnode, ATTR_Volume,       curprog.prgvol );
        AppendAttribute( prgnode, ATTR_Pan,          curprog.prgpan );
        AppendAttribute( prgnode, ATTR_PrgmUnkPoly,  curprog.unkpoly );
        AppendAttribute( prgnode, ATTR_PrgmUnk4,     curprog.unk4);

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
        xml_node lfoentry = AppendChildNode(parent, NODE_LFOEntry); //Always leave at least the empty node, so we can tell how many lfos there are
        AppendAttribute( lfoentry, ATTR_LFOEnabled,    curlfo.unk52  );
        AppendAttribute( lfoentry, ATTR_LFOModDest,    curlfo.dest   );
        AppendAttribute( lfoentry, ATTR_LFOWaveShape,  curlfo.wshape );
        AppendAttribute( lfoentry, ATTR_LFORate,       curlfo.rate   );
        AppendAttribute( lfoentry, ATTR_LFOUnk29,      curlfo.unk29  );
        AppendAttribute( lfoentry, ATTR_LFODepth,      curlfo.depth  );
        AppendAttribute( lfoentry, ATTR_LFODelay,      curlfo.delay  );
        AppendAttribute( lfoentry, ATTR_LFOUnk32,      curlfo.unk32  );
        AppendAttribute( lfoentry, ATTR_LFOUnk33,      curlfo.unk33  );
    }

    void WriteASplit( xml_node & parent, const SplitEntry & cursplit )
    {
        using namespace PrgmXML;
        WriteCommentNode( parent, "Split Sample " + to_string(cursplit.smplid) );
        
        xml_node splitnode = AppendChildNode(parent, NODE_Split);
        AppendAttribute(splitnode, ATTR_ID, cursplit.id);
        WriteNodeWithValue(splitnode, PROP_SplitUnk25, cursplit.unk25);

        {
            xml_node keys = AppendChildNode(splitnode, NODE_Keys);
#ifdef _DEBUG
            //Research helper to see if those ever are different from eachothers
            assert(cursplit.lowkey == cursplit.lowkey2);
            assert(cursplit.hikey  == cursplit.hikey2);
            assert(cursplit.lovel  == cursplit.lovel2);
            assert(cursplit.hivel  == cursplit.hivel2);
#endif
            WriteNodeWithValue(keys, PROP_LowKey,  cursplit.lowkey);
            WriteNodeWithValue(keys, PROP_HighKey, cursplit.hikey);
            WriteNodeWithValue(keys, PROP_LowVel,  cursplit.lovel);
            WriteNodeWithValue(keys, PROP_HighVel, cursplit.hivel);
            WriteNodeWithValue(keys, NODE_KGrp,    cursplit.kgrpid);
        }
        {
            xml_node sample = AppendChildNode(splitnode, NODE_Sample);
            AppendAttribute(sample, ATTR_SmplID, cursplit.smplid);
            WriteNodeWithValue(sample, PROP_Volume, cursplit.smplvol);
            WriteNodeWithValue(sample, PROP_Pan, cursplit.smplpan);
            WriteNodeWithValue(sample, PROP_BendRng, cursplit.bendrange);
            WriteNodeWithValue(sample, PROP_RootKey, cursplit.rootkey);
            WriteNodeWithValue(sample, PROP_FTune, cursplit.ftune);
            WriteNodeWithValue(sample, PROP_CTune, cursplit.ctune);
            WriteNodeWithValue(sample, PROP_KeyTrans, cursplit.ktps);
        }

        WriteDSEEnvelope(splitnode, cursplit.env, cursplit.envon);
    }

    void WriteKeyGroups(xml_document& doc)
    {
        using namespace PrgmXML;
        xml_node kgrpsnode = doc.append_child( NODE_KeyGroups.c_str() );
        auto     ptrprgms  = m_presbnk.prgmbank().lock(); 

        if (ptrprgms == nullptr)
        {
            clog << "PresetBankXMLWriter::WriteKeyGroups(): no program/keygroup data available!";
            return;
        }

        for( const auto & grp : ptrprgms->Keygrps() )
            WriteKeyGroup( kgrpsnode, grp );
    }

    void WriteKeyGroup( xml_node & parent, const KeyGroup & grp )
    {
        using namespace PrgmXML;
        xml_node kgrpnode = parent.append_child( NODE_KGrp.c_str() );
        AppendAttribute( kgrpnode, ATTR_ID,        grp.id );
        AppendAttribute( kgrpnode, ATTR_KGrpPoly,  grp.poly);
        AppendAttribute( kgrpnode, ATTR_KGrpPrio,  grp.priority );
        AppendAttribute( kgrpnode, ATTR_KGrVcLow,  grp.vclow );
        AppendAttribute( kgrpnode, ATTR_KGrVcHi,   grp.vchigh );
        AppendAttribute( kgrpnode, ATTR_KGrUnk50,  grp.unk50);
        AppendAttribute( kgrpnode, ATTR_KGrUnk51,  grp.unk51);
    }


    void WriteWavInfo(xml_document& doc)
    {
        using namespace PrgmXML;
        auto ptrwavs = m_presbnk.smplbank().lock();

        if (ptrwavs == nullptr)
        {
            clog << "PresetBankXMLWriter::WriteWavInfo(): No Sample Info to write.";
            return;
        }

        xml_node infonode = doc.append_child( ROOT_WavInfo.c_str() );
        for( size_t cptwav = 0; cptwav < ptrwavs->NbSlots(); ++cptwav )
        {
            auto ptrwinf = ptrwavs->sampleInfo(cptwav);
            if( ptrwinf != nullptr )
                WriteAWav( infonode, *ptrwinf );
        }
    }

    void WriteAWav( xml_node & parent, const WavInfo & winfo )
    {
        using namespace PrgmXML;
        WriteCommentNode( parent, "Sample #" + std::to_string( winfo.id ) );

        xml_node infonode = parent.append_child( NODE_Sample.c_str() );
        AppendAttribute(infonode, ATTR_ID, winfo.id);
        WriteNodeWithValue(infonode, PROP_Volume, winfo.vol);
        WriteNodeWithValue(infonode, PROP_Pan, winfo.pan);
        WriteCommentNode( infonode, "Tuning Data" );
        WriteNodeWithValue( infonode, PROP_SmplFmt,     DseSmplFmtToString(winfo.smplfmt));
        WriteNodeWithValue( infonode, PROP_FTune,       winfo.ftune );
        WriteNodeWithValue( infonode, PROP_CTune,       winfo.ctune );
        WriteNodeWithValue( infonode, PROP_RootKey,     winfo.rootkey );
        WriteNodeWithValue( infonode, PROP_KeyTrans,    winfo.ktps );
        WriteNodeWithValue( infonode, PROP_SmplRate,    winfo.smplrate);
        WriteCommentNode( infonode, "Loop Data (calculated in individual sample points)" );
        //Samples points will change depending on whether they were converted.
        {
            xml_node loopnode = AppendChildNode(infonode, NODE_Loop);
            AppendAttribute(loopnode, ATTR_On, winfo.smplloop);
            WriteNodeWithValue(loopnode, PROP_LoopBeg, winfo.loopbeg);
            WriteNodeWithValue(loopnode, PROP_LoopLen, winfo.looplen);
        }
        WriteDSEEnvelope(infonode, winfo.env, winfo.envon);
    }

private:
    const PresetBank & m_presbnk;
};

//====================================================================================================
//  Functions
//====================================================================================================

    /*
        PresetBankToXML
            Write a xml file containing the info from the given preset bank
    */
    void PresetBankToXML( const DSE::PresetBank & srcbnk, const std::string& bnkxmlfile)
    {
        try
        {
            PresetBankXMLWriter(srcbnk).Write(bnkxmlfile);
        }
        catch (...)
        {
            std::throw_with_nested(std::runtime_error("Error while writing preset bank xml file: " + bnkxmlfile));
        }
    }

    /*
        XMLToPresetBank
            Read the xml file to a preset bank.
    */
    DSE::PresetBank XMLToPresetBank(const std::string& bnkxmlfile)
    {
        try
        {
            return PresetBankXMLParser(bnkxmlfile).Parse();
        }
        catch (...)
        {
            std::throw_with_nested(std::runtime_error("Error while parsing preset bank xml file: " + bnkxmlfile));
        }
    }

    bool IsXMLPresetBank(const std::string& xmlfilepath)
    {
        using namespace PrgmXML;
        xml_document doc;
        doc.load_file(xmlfilepath.c_str());
        return (doc.child(ROOT_Programs.c_str()) || doc.child(ROOT_WavInfo.c_str()));
    }
};
