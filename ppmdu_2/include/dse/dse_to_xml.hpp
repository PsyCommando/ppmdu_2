#ifndef DSE_TO_XML_HPP
#define DSE_TO_XML_HPP
/*
dse_to_xml.hpp
2015/07/07
psycommando@gmail.com
Descritpton: Contains utilities for converting DSE audio parameters to XML data.
*/
#include <dse/dse_common.hpp>

namespace pugi { class xml_document; class xml_node; }

namespace DSE
{
///////////////////////////////////////////////////////////////////////
// Common XML Constants
///////////////////////////////////////////////////////////////////////
    namespace XmlConstants
    {
        extern const std::string ROOT_DSEInfo;
        extern const std::string ATTR_DSE_Version;
        extern const std::string ATTR_DSE_BankLow;
        extern const std::string ATTR_DSE_BankHigh;
        extern const std::string ATTR_DSE_OrigFname;
        extern const std::string ATTR_DSE_OrigLoadOrder;
    }

///////////////////////////////////////////////////////////////////////
// DSE Information XML Node Helper
///////////////////////////////////////////////////////////////////////
    
    /// <summary>
    /// Parses an xml node from a document containing dse meta data of the given type.
    /// </summary>
    void ParseDSEXmlNode(pugi::xml_node doc, DSE::DSE_MetaData* meta);

    /// <summary>
    /// Writes an xml node to a document containing dse meta data of the given type.
    /// </summary>
    void WriteDSEXmlNode(pugi::xml_node doc, const DSE::DSE_MetaData* meta);

///////////////////////////////////////////////////////////////////////
// DSE Information XML Node Helper
///////////////////////////////////////////////////////////////////////
    void ParseSequenceInfo(pugi::xml_node parent, std::vector<DSE::seqinfo_table>& sequences);
    void WriteSequenceInfo(pugi::xml_node parent, const std::vector<DSE::seqinfo_table>& sequences);
    void WriteSequenceInfo(pugi::xml_node parent, const DSE::seqinfo_table& seq);
};

#endif 