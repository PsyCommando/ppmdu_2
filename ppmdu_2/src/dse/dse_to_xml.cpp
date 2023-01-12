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
        const std::string ATTR_DSE_Unk17         = "unk17"s;
        const std::string ATTR_DSE_Vol           = "vol"s;
        const std::string ATTR_DSE_Pan           = "pan"s;
        const std::string ATTR_DSE_Unk5          = "unk5"s;
        const std::string ATTR_DSE_Unk6 = "unk6"s;
        const std::string ATTR_DSE_Unk7 = "unk7"s;
        const std::string ATTR_DSE_Unk8 = "unk8"s;
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
    }
    void DSE::DSE_MetaDataSWDL::ParseXml(pugi::xml_node dsei)
    {
        using namespace XmlConstants;
        DSE_MetaData::ParseXml(dsei);
        utils::parseHexaValToValue(dsei.attribute(ATTR_DSE_Unk17.c_str()).value(), unk17);
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
    };

    void ParseSequenceInfo(pugi::xml_node parent, std::vector<DSE::seqinfo_table>& sequences)
    {
        using namespace SeqInfoXml;
        xml_node sequencesnode = parent.child(NODE_Sequences.c_str());
        for (const xml_node& node : sequencesnode.children(NODE_Sequence.c_str()))
        {
            seqinfo_table tbl;
            tbl.ParseXml(node);
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