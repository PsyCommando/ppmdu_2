#include <ppmdu/fmts/text_str.hpp>
#include <ppmdu/pmd2/pmd2.hpp>
#include <utils/gfileio.hpp>
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>
#include <utils/poco_wrapper.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <map>
using namespace std;

#define PMD2_STRINGS_NO_LOCALE 

namespace pmd2 { namespace filetypes
{
//
//  Constants 
//
    //static const string        SpecialCharSequ_MusicNote = "\129\244"; //~'music note'
    //static const unsigned char SpecialChar_CtrlChar      = 129u;
    //static const unsigned char SpecialChar_MusicNoteEnd  = 244u;

//============================================================================================
//  Loader
//============================================================================================
    /*
        TextStrLoader
            Loads the text_*.str files used in pmd2!
    */
    class TextStrLoader
    {
    public:
        TextStrLoader( const std::string & filepath, const std::locale & txtloc, bool escapejis )
            :m_strFilePath(filepath), m_locale(txtloc), m_escapejis(escapejis)
        {}

        operator std::vector<std::string>() //#REMOVEME: Just out of curiosity I wanted to try this!
        {
            return Read();
        }

        std::vector<std::string> operator()()
        {
            return Read();
        }

        std::vector<std::string> Read()
        {
            try
            {
                m_txtstr = vector<string>(); //Ensure the vector has a valid state
                m_filedata = utils::io::ReadFileToByteVector( m_strFilePath );
                //Read pointer table
                ReadPointerTable();
                clog <<"Found " <<dec <<m_ptrTable.size() <<" strings to parse!\nParsing..";
                //Read all the strings
                ReadStrings();
            }
            catch( exception & e )
            {
                stringstream sstr;
                sstr << "Error while parsing file \"" << m_strFilePath <<"\". Cannot load the game's text strings file! : "
                     << e.what();
                throw_with_nested(runtime_error(sstr.str()));
            }

            return std::move(m_txtstr);
        }

    private:


        void ReadPointerTable()
        {
            m_ptrTable.resize(0);
            auto itptrs = m_filedata.begin();

            //First get the first pointer to get the end of the ptr table!
            uint32_t endptrtbl = utils::ReadIntFromBytes<uint32_t>( itptrs, m_filedata.end() );  //iterator is incremented
            m_ptrTable.reserve( endptrtbl / sizeof(uint32_t) );                     //reserve memory for all pointers
            m_ptrTable.push_back( endptrtbl );

            //Read all pointers
            auto itendptrs = m_filedata.begin() + endptrtbl;
            for( ; itptrs != itendptrs; )
                m_ptrTable.push_back( utils::ReadIntFromBytes<uint32_t>( itptrs, m_filedata.end() ) );
        }

        void ReadStrings()
        {
            const unsigned int PtrTableSize = static_cast<unsigned int>(m_ptrTable.size())-1; // The last pointer is a pointer to the end of the file!
            const unsigned int LastPtrIndex = PtrTableSize - 1;    // Index of the last element before the end
            
            //Allocate
            m_txtstr.resize(m_ptrTable.size());
            
            //Read them all
            for( unsigned int i = 0; i < PtrTableSize;  )
            {
                unsigned int len = 0;

                if( i == LastPtrIndex )
                    len = PtrTableSize - i;
                else
                    len = (m_ptrTable[i+1] - m_ptrTable[i]);

                //char* ptrstr = reinterpret_cast<char*>(m_filedata.data() + m_ptrTable[i]); //#TODO: think of something faster...
                auto itcurstr = m_filedata.begin() + m_ptrTable[i];
                string curstr( itcurstr, itcurstr + len);
                //curstr.push_back('\0');

                m_txtstr[i] = std::move( EscapeUnprintableCharacters( curstr, m_escapejis, false, m_locale ) );
                ++i;
            }
            clog<<" Done!\n";
        }

        std::string              m_strFilePath;
        std::vector<uint8_t>     m_filedata;
        std::vector<uint32_t>    m_ptrTable;
        std::vector<std::string> m_txtstr;
        const std::locale      & m_locale;
        bool                     m_escapejis;
    };

//============================================================================================
//  Writer
//============================================================================================

    /*
        TextStrWriter
            Write a text_*.str file from the text strings specified!
    */
    class TextStrWriter
    {
    public:
        TextStrWriter( const std::vector<std::string> & textstr, const std::locale & txtloc )
            :m_txtstr(textstr), m_locale(txtloc)
        {}

        /*
            Returns the nb of characters that were treated as part of the escaped char
        */
        //template<class _itout>
        //    unsigned int HandleEscapedChar( _itout & itwrite, const std::string & textafterbs, const std::locale & txtloc )
        //{
        //    unsigned int escLen = 1; //in case of error use as a single char

        //    //If the character is a backslash, and there is at least another value in the string
        //    char nextchar = textafterbs.front();

        //    //Expedite string endings handling
        //    if( nextchar == '0' )
        //    {
        //        (*itwrite) = static_cast<unsigned char>(0);
        //        ++itwrite;
        //        return 2;
        //    }

        //    if( isprint(nextchar,txtloc) ) 
        //    {
        //        //If the next character is printable
        //        if( isalpha( nextchar, txtloc )  || ispunct(nextchar, txtloc) )
        //        {

        //            escLen = 2;

        //            //If its a letter or punctuation char
        //            switch(nextchar)
        //            {
        //                case'n':
        //                {
        //                    (*itwrite) = '\n';
        //                    break;
        //                }
        //                case 't':
        //                {
        //                    (*itwrite) = '\t';
        //                    break;
        //                }
        //                case '\\':
        //                {
        //                    (*itwrite) = '\\';
        //                    break;
        //                }
        //                case '\b':
        //                {
        //                    (*itwrite) = '\b';
        //                    break;
        //                }
        //                default:
        //                {
        //                    clog <<"Ecountered unexpected escaped character \"\\" <<nextchar <<"\". Handling both characters as-is!\n";
        //                    (*itwrite) = '\\'; //Not an escaped character
        //                    escLen = 1;
        //                }
        //            };

        //            ++itwrite;
        //        }
        //        else if( isdigit(nextchar, txtloc) )
        //        {
        //            //If its a number
        //            stringstream parse;
        //            unsigned short valread = 0; 
        //            parse.imbue( txtloc );
        //        
        //            //If is decimal
        //            parse << textafterbs << dec;
        //            parse >> valread;

        //            (*itwrite) = static_cast<unsigned char>(valread);
        //            ++itwrite;

        //            escLen = static_cast<unsigned int>(parse.tellg()) + 1; //Account for the lenght of the number + the backslash
        //        }
        //        else
        //        {
        //            //If its neither we have no support for that
        //            clog <<"Ecountered unexpected escaped character \"\\" <<nextchar <<"\". Handling both characters as-is!\n";
        //            (*itwrite) = '\\';
        //            ++itwrite;
        //        }
        //    }
        //    else
        //    {
        //        (*itwrite) = '\\'; //Not an escaped character if character is unprintable
        //        ++itwrite;
        //    }

        //    return escLen;
        //}

        void Write( const std::string & filepath )
        {
            //Re-init ptr write positon, just in case someone would re-use the object..
            m_ptrTblWriteAt = 0;

            //Reserve ptr table space!
            m_fileData.resize( m_txtstr.size() * PTR_LEN );

            clog << "Writing " <<dec << m_txtstr.size() <<" strings to file \"" <<filepath <<"\"";

            auto itbackins = back_inserter( m_fileData );

            for( unsigned int cntstr = 0; cntstr < m_txtstr.size();  )
            {
                const auto & str = m_txtstr[cntstr];
                utils::WriteIntToBytes<uint32_t>( m_fileData.size(), m_fileData.begin() + m_ptrTblWriteAt );  //Write string offset
                m_ptrTblWriteAt += PTR_LEN;

                //
                //string processed;
                //auto         itprocessedbackins = back_inserter( processed );
                //processed.reserve(str.size());
                //for( unsigned int i = 0; i < str.size();  )
                //{
                //    char c = str[i];
                //    if( c == '\\' && i < (str.size()-1) )
                //    {   
                //        //HandleEscapedChar returns the amount of characters that were handled as part of the escaped char.
                //        i+= HandleEscapedChar( itprocessedbackins, str.substr( i + 1 ), m_locale ) ;
                //    }
                //    else
                //    {
                //        (*itprocessedbackins) = c; //If the character is not a backslash, or if there are no other characters after the backslash
                //        ++i;
                //    }
                //}
                //processed = std::move( ReplaceEscapedCharacters( str, m_locale ) );
                string processed = str; 
                ReplaceEscapedSequenceTest(processed);

                std::copy( processed.begin(), processed.end(), itbackins );
                ++cntstr;
            }
            cout<<" Done!\n";
            //Write file
            utils::io::WriteByteVectorToFile( filepath, m_fileData );
        }

    private:
        static const unsigned int        PTR_LEN = sizeof(uint32_t);
        std::vector<uint8_t>             m_fileData;
        uint32_t                         m_ptrTblWriteAt;    //Position to write current str offset at
        const std::vector<std::string> & m_txtstr;
        const std::locale              & m_locale;
    };

//=========================================================================================
//  Functions
//=========================================================================================
    std::vector<std::string> ParseTextStrFile( const std::string & filepath, eGameRegion gver, const std::locale & txtloc )
    {
        bool escapejis = gver != eGameRegion::Japan;
        //Read the pointer table first
        return TextStrLoader(filepath,txtloc,escapejis); //lol, implicit cast operator
    }
    
    void WriteTextStrFile( const std::string & filepath, const std::vector<std::string> & text, eGameRegion gver, const std::locale & txtloc )
    {
        TextStrWriter(text,txtloc).Write(filepath);
    }

};};