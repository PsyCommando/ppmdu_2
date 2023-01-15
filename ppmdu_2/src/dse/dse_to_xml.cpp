#include <dse/dse_to_xml.hpp>
#include <dse/containers/dse_music_sequence.hpp>
#include <dse/containers/dse_se_sequence.hpp>

#include <utils/parse_utils.hpp>
#include <utils/pugixml_utils.hpp>

#include <pugixml.hpp>

using namespace std;
using namespace pugi;
using namespace utils;
using namespace pugixmlutils;

namespace DSE
{
    namespace XmlConstants 
    {
        const std::string ROOT_DSEInfo           = "DSE"s;
        const std::string ATTR_DSE_Version       = "version"s;
        const std::string ATTR_DSE_BankLow       = "bankLow"s;
        const std::string ATTR_DSE_BankHigh      = "bankHigh"s;
        const std::string ATTR_DSE_InternalFname = "internal_filename"s;
        const std::string ATTR_DSE_OrigFname     = "original_filename"s;
        const std::string ATTR_DSE_OrigLoadOrder = "original_load_order"s;
        const std::string ATTR_DSE_OrigTime      = "original_timestamp"s;
        const std::string ATTR_DSE_OrigCentSec   = "original_centsec"s;
        const std::string ATTR_DSE_Unk17         = "unk17"s;
        const std::string ATTR_DSE_Vol           = "vol"s;
        const std::string ATTR_DSE_Pan           = "pan"s;
        const std::string ATTR_DSE_Unk5          = "unk5"s;
        const std::string ATTR_DSE_Unk6          = "unk6"s;
        const std::string ATTR_DSE_Unk7          = "unk7"s;
        const std::string ATTR_DSE_Unk8          = "unk8"s;
        const std::string ATTR_DSE_NbPrgmSlots   = "prgi_slots"s;
    };

    void ParseDSEXmlNode(pugi::xml_node parent, DSE::DSE_MetaData* meta)
    {
        using namespace XmlConstants;
        meta->ParseXml(parent.child(ROOT_DSEInfo.c_str()));
    }

    void WriteDSEXmlNode(pugi::xml_node parent, const DSE::DSE_MetaData* meta)
    {
        using namespace XmlConstants;
        meta->WriteXml(parent.append_child(ROOT_DSEInfo.c_str()));
    }

    //
    // DSE_MetaData
    //
    void DSE::DSE_MetaData::WriteXml(xml_node dsei)const
    {
        using namespace XmlConstants;
        AppendAttribute(dsei, ATTR_DSE_Version,       DseVerToInt(origversion));
        AppendAttribute(dsei, ATTR_DSE_BankLow,       bankid_coarse);
        AppendAttribute(dsei, ATTR_DSE_BankHigh,      bankid_fine);
        AppendAttribute(dsei, ATTR_DSE_InternalFname, fname);
        AppendAttribute(dsei, ATTR_DSE_OrigFname,     origfname);
        AppendAttribute(dsei, ATTR_DSE_OrigLoadOrder, origloadorder);
        AppendAttribute(dsei, ATTR_DSE_OrigTime,      static_cast<std::string>(createtime));
        AppendAttribute(dsei, ATTR_DSE_OrigCentSec,   createtime.centsec);
    }

    void DSE::DSE_MetaData::ParseXml(xml_node dsei)
    {
        using namespace XmlConstants;
        for (xml_attribute att : dsei.attributes())
        {
            const string att_name = att.name();
            if(att_name == ATTR_DSE_Version)
                origversion = DSE::intToDseVer(utils::parseHexaValToValue<uint16_t>(att.value()));
            else if(att_name == ATTR_DSE_BankLow)
                utils::parseHexaValToValue(att.value(), bankid_coarse);
            else if (att_name == ATTR_DSE_BankHigh)
                utils::parseHexaValToValue(att.value(), bankid_fine);
            else if(att_name == ATTR_DSE_OrigLoadOrder)
                utils::parseHexaValToValue(att.value(), origloadorder);
            else if(att_name == ATTR_DSE_OrigFname)
                origfname = std::string(att.as_string());
            else if(att_name == ATTR_DSE_InternalFname)
                fname = std::string(att.as_string());
            else if (att_name == ATTR_DSE_OrigTime)
            {
                const string timestamp = att.as_string();
                stringstream ssin(timestamp);
                ssin >> createtime;
            }
            else if(att_name == ATTR_DSE_OrigCentSec)
                utils::parseHexaValToValue(att.value(), createtime.centsec);
        }
    }

    //
    // DSE_MetaDataSWDL
    //
    void DSE::DSE_MetaDataSWDL::WriteXml(pugi::xml_node dsei)const
    {
        using namespace XmlConstants;
        DSE_MetaData::WriteXml(dsei);
        AppendAttribute(dsei, ATTR_DSE_Unk17, unk17);
        AppendAttribute(dsei, ATTR_DSE_NbPrgmSlots, nbprgislots);
    }
    void DSE::DSE_MetaDataSWDL::ParseXml(pugi::xml_node dsei)
    {
        using namespace XmlConstants;
        DSE_MetaData::ParseXml(dsei);
        utils::parseHexaValToValue(dsei.attribute(ATTR_DSE_Unk17.c_str()).value(), unk17);
        nbprgislots = (uint16_t)dsei.attribute(ATTR_DSE_NbPrgmSlots.c_str()).as_uint();
    }

    //
    // DSE_MetaDataSMDL
    //
    void DSE::DSE_MetaDataSMDL::WriteXml(pugi::xml_node dsei)const
    {
        using namespace XmlConstants;
        DSE_MetaData::WriteXml(dsei);
        AppendAttribute(dsei, ATTR_DSE_Vol, mainvol);
        AppendAttribute(dsei, ATTR_DSE_Pan, mainpan);
    }
    void DSE::DSE_MetaDataSMDL::ParseXml(pugi::xml_node dsei)
    {
        using namespace XmlConstants;
        DSE_MetaData::ParseXml(dsei);
        utils::parseHexaValToValue(dsei.attribute(ATTR_DSE_Vol.c_str()).value(), mainvol);
        utils::parseHexaValToValue(dsei.attribute(ATTR_DSE_Pan.c_str()).value(), mainpan);
    }

    //
    // DSE_MetaDataSEDL
    //
    void DSE::DSE_MetaDataSEDL::WriteXml(pugi::xml_node dsei)const
    {
        using namespace XmlConstants;
        DSE_MetaData::WriteXml(dsei);
        AppendAttribute(dsei, ATTR_DSE_Unk5, unk5);
        AppendAttribute(dsei, ATTR_DSE_Unk6, unk6);
        AppendAttribute(dsei, ATTR_DSE_Unk7, unk7);
        AppendAttribute(dsei, ATTR_DSE_Unk8, unk8);
    }
    void DSE::DSE_MetaDataSEDL::ParseXml(pugi::xml_node dsei)
    {
        using namespace XmlConstants;
        DSE_MetaData::ParseXml(dsei);
        utils::parseHexaValToValue(dsei.attribute(ATTR_DSE_Unk5.c_str()).value(), unk5);
        utils::parseHexaValToValue(dsei.attribute(ATTR_DSE_Unk6.c_str()).value(), unk6);
        utils::parseHexaValToValue(dsei.attribute(ATTR_DSE_Unk7.c_str()).value(), unk7);
        utils::parseHexaValToValue(dsei.attribute(ATTR_DSE_Unk8.c_str()).value(), unk8);
    }



//
// Sequence Info
//
    namespace SeqInfoXml
    {
        const std::string NODE_Sequences = "Sequences"s;
        const std::string NODE_Sequence  = "Sequence"s;
        const std::string NODE_SeqInfo = "Info"s;
    };

    void seqinfo_table::WriteXml(pugi::xml_node seqnode)const
    {
        using namespace SeqInfoXml;
        xml_node infonode = AppendChildNode(seqnode, NODE_SeqInfo);
        WriteHexNumberVarToXml(unk30, infonode);
        WriteHexNumberVarToXml(unk16, infonode);
        //WriteNumberVarToXml(nbtrks,  infonode);
        //WriteNumberVarToXml(nbchans, infonode);
        WriteHexNumberVarToXml(unk19, infonode);
        WriteHexNumberVarToXml(unk20, infonode);
        WriteHexNumberVarToXml(unk21, infonode);
        WriteHexNumberVarToXml(unk22, infonode);
        WriteHexNumberVarToXml(unk23, infonode);
        WriteHexNumberVarToXml(unk24, infonode);
        WriteHexNumberVarToXml(unk25, infonode);
        WriteHexNumberVarToXml(unk26, infonode);
        WriteHexNumberVarToXml(unk27, infonode);
        WriteHexNumberVarToXml(unk28, infonode);
        WriteHexNumberVarToXml(unk29, infonode);
        WriteHexNumberVarToXml(unk31, infonode);
        WriteHexNumberVarToXml(unk12, infonode);

        WriteHexNumberVarToXml(unk5, infonode);
        WriteHexNumberVarToXml(unk6, infonode);
        WriteHexNumberVarToXml(unk7, infonode);
        WriteHexNumberVarToXml(unk8, infonode);
    }

    void seqinfo_table::ParseXml(pugi::xml_node seqnode)
    {
        using namespace SeqInfoXml;
        xml_node infonode = seqnode.child(NODE_SeqInfo.c_str());
        if (!infonode)
            return;
        ParseNumberVarFromXml(unk30, infonode);
        ParseNumberVarFromXml(unk16, infonode);
        //ParseNumberVarFromXml(nbtrks,  infonode);
        //ParseNumberVarFromXml(nbchans, infonode);
        ParseNumberVarFromXml(unk19, infonode);
        ParseNumberVarFromXml(unk20, infonode);
        ParseNumberVarFromXml(unk21, infonode);
        ParseNumberVarFromXml(unk22, infonode);
        ParseNumberVarFromXml(unk23, infonode);
        ParseNumberVarFromXml(unk24, infonode);
        ParseNumberVarFromXml(unk25, infonode);
        ParseNumberVarFromXml(unk26, infonode);
        ParseNumberVarFromXml(unk27, infonode);
        ParseNumberVarFromXml(unk28, infonode);
        ParseNumberVarFromXml(unk29, infonode);
        ParseNumberVarFromXml(unk31, infonode);
        ParseNumberVarFromXml(unk12, infonode);

        ParseNumberVarFromXml(unk5, infonode);
        ParseNumberVarFromXml(unk6, infonode);
        ParseNumberVarFromXml(unk7, infonode);
        ParseNumberVarFromXml(unk8, infonode);
    }

    void ParseSequenceInfo(pugi::xml_node parent, std::vector<DSE::seqinfo_table>& sequences)
    {
        using namespace SeqInfoXml;
        if (parent.name() == NODE_Sequences)
        {
            for (const xml_node& node : parent.children(NODE_Sequence.c_str()))
            {
                seqinfo_table tbl;
                tbl.ParseXml(node);
                sequences.push_back(std::move(tbl));
            }
        }
        else
        {
            xml_node seq = (parent.name() == NODE_Sequence) ? parent : parent.child(NODE_Sequence.c_str());
            if (!seq)
                throw std::runtime_error("No sequence node found!");
            seqinfo_table tbl;
            tbl.ParseXml(seq);
            sequences.push_back(std::move(tbl));
        }

    }
    void WriteSequenceInfo(pugi::xml_node parent, const std::vector<DSE::seqinfo_table>& sequences)
    {
        using namespace SeqInfoXml;
        //Append or create a sequences node
        xml_node sequencesnode = (parent.name() == NODE_Sequences)? parent : parent.child(NODE_Sequences.c_str());
        if (!sequencesnode)
            sequencesnode = AppendChildNode(parent, NODE_Sequences);

        //Then append or get a sequence node for each of the contained sequences
        auto rangeseqs = sequencesnode.children(NODE_Sequence.c_str());
        auto itbegsubseq = rangeseqs.begin();
        auto itendsubseq = rangeseqs.end();
        for (const seqinfo_table& inf : sequences)
        {
            //Grab sequence nodes if we have them, or create new ones if we're missing any
            xml_node subseqnode;
            if (itbegsubseq != itendsubseq)
            {
                subseqnode = *itbegsubseq;
                ++itbegsubseq;
            }
            else
                subseqnode = AppendChildNode(sequencesnode, NODE_Sequences);
            WriteSequenceInfo(sequencesnode, inf);
        }
    }
    void WriteSequenceInfo(pugi::xml_node parent, const DSE::seqinfo_table& seq)
    {
        using namespace SeqInfoXml;
        //Grab or create a sequence parent node
        xml_node seqnode = (parent.name() == NODE_Sequence)? parent : parent.child(NODE_Sequence.c_str());
        if (!seqnode)
            seqnode = AppendChildNode(parent, NODE_Sequence);

        //Write the sequence info table
        seq.WriteXml(seqnode);
    }

};