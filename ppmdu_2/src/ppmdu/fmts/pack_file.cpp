/*
*/
#include <ppmdu/fmts/pack_file.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <types/content_type_analyser.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <Poco/DirectoryIterator.h>
#include <cassert>
#include <utils/gbyteutils.hpp>
#include <utils/gfileutils.hpp>
#include <utils/utility.hpp>
using namespace std;
using namespace utils::io;
using namespace utils;
using namespace ::filetypes;
using namespace pmd2;

namespace filetypes 
{
    static const uint8_t PF_PADDING_BYTE = 0xFF; // Padding character used by the Pack file for padding files and the header

    const ContentTy CnTy_PackFile{"bin"};

//===============================================================================
//                                  Functions
//===============================================================================

    uint32_t ComputeFileNBPaddingBytes( uint32_t filelen )
    {
        return CalculateLengthPadding( filelen, 16u );// ( GetNextInt32DivisibleBy16( filelen ) - filelen );
    }


//===============================================================================
// fileindex
//===============================================================================

    std::vector<uint8_t>::iterator fileIndex::WriteToContainer( std::vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToBytes( _fileOffset, itwriteto );
        itwriteto = utils::WriteIntToBytes( _fileLength, itwriteto );
        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator fileIndex::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom, 
                                                                       std::vector<uint8_t>::const_iterator itpastend )
    {
        _fileOffset = utils::ReadIntFromBytes<decltype(_fileOffset)>(itReadfrom,itpastend);
        _fileLength = utils::ReadIntFromBytes<decltype(_fileLength)>(itReadfrom,itpastend);
        return itReadfrom;
    }

//===============================================================================
// pfheader
//===============================================================================

    std::vector<uint8_t>::iterator pfheader::WriteToContainer( std::vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToBytes( _zeros,   itwriteto );
        itwriteto = utils::WriteIntToBytes( _nbfiles, itwriteto );
        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator pfheader::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom,
                                                                      std::vector<uint8_t>::const_iterator itPastEnd)
    {
        _zeros   = utils::ReadIntFromBytes<decltype(_zeros)>  (itReadfrom,itPastEnd);
        _nbfiles = utils::ReadIntFromBytes<decltype(_nbfiles)>(itReadfrom,itPastEnd);
        return itReadfrom;
    }

    bool pfheader::isValid()const
    {
        return (_zeros == 0x0) && (_nbfiles > 0x0);
    }

//===============================================================================
//								CPack
//===============================================================================
    CPack::CPack()
        :m_ForcedFirstFileOffset(0)
    {

    }

    CPack::CPack( std::vector<std::vector<uint8_t>> && subfiles )
        :m_ForcedFirstFileOffset(0), m_SubFiles(subfiles)
    {
    }

    void CPack::ClearState()
    {
        //Clear all current data
        m_SubFiles.resize(0);
        m_OffsetTable.resize(0);
        m_ForcedFirstFileOffset = 0;
    }
    
    uint32_t CPack::PredictHeaderSize( uint32_t nbsubfiles )
    {
        // Count the begining zeros and the size of the entry for the nb of files 
        uint32_t headerlengthsofar = pfheader::HEADER_LEN;

        //calculate the number of file offset table entries by multiplying nb of files by the size of each entries
        headerlengthsofar += ( nbsubfiles * SZ_OFFSET_TBL_ENTRY );

        //Add the end delemiter 
        headerlengthsofar += SZ_OFFSET_TBL_DELIM; 

        return headerlengthsofar;
    }

    uint32_t CPack::PredictHeaderSizeWithPadding( uint32_t nbsubfiles )
    {
        const uint32_t hdrlen = PredictHeaderSize(nbsubfiles);
        return CalculatePaddedLengthTotal( hdrlen, 16u );
    }

    uint32_t CPack::getCurrentPredictedHeaderLengthWithForcedOffset()const
    {
        return ( IsForcedOffsetCurrentlyPossible() ) ? m_ForcedFirstFileOffset : PredictHeaderSizeWithPadding(m_SubFiles.size()); //It recalculates a lot !
    }

    bool CPack::IsForcedOffsetCurrentlyPossible()const
    {
        return ( m_ForcedFirstFileOffset > PredictHeaderSizeWithPadding( m_SubFiles.size() ) );
    }


    void CPack::LoadPack(std::vector<uint8_t>::const_iterator beg, std::vector<uint8_t>::const_iterator end)
    {
        //Clear all current data
        ClearState();

        //#1 - Read header
        pfheader mahead;
        mahead.ReadFromContainer(beg, end);

	    //----------- Analyze file ----------
        //Is it using a forced first file offset ?
        m_ForcedFirstFileOffset = IsPackFileUsingForcedFFOffset( beg, end, mahead._nbfiles );

        //Fill the File Offset Table
        ReadFOTFromPackFile( beg, end, mahead._nbfiles );

        //Get the file data
        ReadSubFilesFromPackFileUsingFOT( beg, end );
    }

    void CPack::LoadFolder( const std::string & pathdir )
    {
        //utils::MrChronometer chronofolderloader("Folder Loader");

        //Check folder exists
        if( !utils::isFolder(pathdir) )
            throw runtime_error("CPack::LoadFolder(): Invalid input path !");

        //Clear current data, reset forced first file offset to 0
        ClearState();

        //List files in folder, put them into a vector
        Poco::DirectoryIterator itdir(pathdir),
                                itdirend;
        //unsigned int            nbfiles = 0;
        vector<Poco::File>      validFiles;

        //A little lambda for determining valid files
        static auto lambdavalidfile = []( const Poco::File & f )
        { 
            return f.isFile() && !(f.isHidden()); 
        };

        //Count valid files inside folder
        for( Poco::DirectoryIterator itcount(pathdir); itcount != itdirend; ++itcount )
        {
            if( lambdavalidfile(*itcount) )
                validFiles.push_back(*itcount);
        }

        m_SubFiles.reserve(validFiles.size());
        //auto itbackins = std::back_inserter( m_SubFiles );

        for( const auto & afile : validFiles )
        {
            vector<uint8_t> filedata;
            ReadFileToByteVector( afile.path(), filedata );
            m_SubFiles.push_back( std::move(filedata) );
        }
        
        ////Read each files into the file data table
        //for( unsigned int cptfiles = 0; itdir != itdirend; ++cptfiles, ++itdir )
        //{
        //    if( lambdavalidfile(*itdir) )
        //        ReadFileToByteVector( itdir->path(), m_SubFiles[cptfiles] );
        //}

    }


    //void CPack::OutputToFile( const std::string & pathfile )
    vector<uint8_t> CPack::OutputPack()
    {
        //Build the FOT
        BuildFOT();

        if( m_OffsetTable.empty() )
            throw std::runtime_error( "CPack::OutputPack(): No subfiles. File offset table was empty!" );

        vector<uint8_t> result( PredictTotalFileSize() );
        auto            ittwritepos = WriteFullHeader( result.begin() );

        ittwritepos = WriteFileData( ittwritepos );

        return std::move(result); //Implicit move constructor
    }


    void CPack::OutputToFolder( const std::string & pathdir )
    {
        //MrChronometer chronooutputer( "Unpacking Files" );

        if( !utils::DoCreateDirectory( pathdir ) )
            throw runtime_error("CPack::OutputToFolder(): Invalid output path!");

        //write them out
        for( unsigned int i = 0; i < m_SubFiles.size(); ++i)
            WriteSubFileToFile( m_SubFiles[i], pathdir, i );
    }


    void CPack::BuildFOT()
    {
        //Empty FOT first
        m_OffsetTable.resize(0);

        //Computer the header's length
        uint32_t offsetsofar = getCurrentPredictedHeaderLengthWithForcedOffset();
        m_OffsetTable.reserve( m_SubFiles.size() );

        //Add files to the offset table and !! compute padding for each to get the correct offsets !!
        for( auto & entry : m_SubFiles )
        {
            m_OffsetTable.push_back( fileIndex( offsetsofar, entry.size() ) );
            offsetsofar += entry.size();
            offsetsofar =  CalculatePaddedLengthTotal( offsetsofar, 16u ); //compensate for padding
        }
    }

    uint32_t CPack::CalcAmountHeaderPaddingBytes()const
    {
        uint32_t headerlengthwithpadding = PredictHeaderSizeWithPadding( m_OffsetTable.size() );
        
        if( m_ForcedFirstFileOffset > headerlengthwithpadding ) //Calculate the extra forced padding
            headerlengthwithpadding = m_ForcedFirstFileOffset;

        return headerlengthwithpadding - PredictHeaderSize( m_OffsetTable.size() );
    }

    //vector<uint8_t>::const_iterator CPack::ReadHeader( vector<uint8_t>::const_iterator itbegin, 
    //                                                   vector<uint8_t>::const_iterator itpastend, 
    //                                                  pfheader                        & out_mahead )const
    //{
    //    return out_mahead.ReadFromContainer(itbegin, itpastend);
    //}

    // !!- OK -!!
    void CPack::ReadFOTFromPackFile( vector<uint8_t>::const_iterator itbegin, 
                                     vector<uint8_t>::const_iterator itend, 
                                     unsigned int                    nbsubfiles )
    {
        const uint32_t                  TOTAL_BYTES_FOT = nbsubfiles * SZ_OFFSET_TBL_ENTRY;
        vector<uint8_t>::const_iterator itt             = itbegin;
        m_OffsetTable.resize( nbsubfiles );

        std::advance(itt,  OFFSET_TBL_FIRST_ENTRY);                    //Move to beginning of FOT

        for( auto & entry : m_OffsetTable )
        {
            fileIndex fi;
            itt = fi.ReadFromContainer(itt, itend);
            entry = fi;
        }
    }


    void CPack::ReadSubFilesFromPackFileUsingFOT( vector<uint8_t>::const_iterator itbegin,
                                                  vector<uint8_t>::const_iterator itend )
    {
        if( m_OffsetTable.empty() )
            throw std::runtime_error( "CPack::ReadSubFilesFromPackFileUsingFOT(): The file allocation table contains no entries!" );

        //assert( !m_OffsetTable.empty() ); //If this happens, the method was probably called before the FOT was built..
        const auto NB_SUBFILES = m_OffsetTable.size(); //Avoid doing function calls all the time, and also allow compiler optimization for constants
        m_SubFiles.resize( NB_SUBFILES );

        //cout << " -Reading subfiles..\n";

        for( unsigned int i = 0; i < NB_SUBFILES; ++i )
        {
            //Resize the destination container
            m_SubFiles[i].resize( m_OffsetTable[i]._fileLength );

            //Copy the data to it
            copy_n( itbegin + m_OffsetTable[i]._fileOffset, m_OffsetTable[i]._fileLength, m_SubFiles[i].begin() );
        }
    }

    // !!- OK -!!
    uint32_t CPack::IsPackFileUsingForcedFFOffset( vector<uint8_t>::const_iterator itbeg, 
                                                   vector<uint8_t>::const_iterator itend, 
                                                   unsigned int                    nbsubfiles )const
    {
        uint32_t             expectedlength;
        uint32_t             actuallength;
        vector<uint8_t>::const_iterator ittread        = itbeg;
        
        expectedlength = PredictHeaderSizeWithPadding( nbsubfiles ); //compute expected header length

        //Get the actual first file offset from the file
        std::advance( ittread, OFFSET_TBL_FIRST_ENTRY );

        actuallength = utils::ReadIntFromBytes<uint32_t>(ittread, itend);

        //compare with first subfile's offset in file's offset table
        return ( actuallength > expectedlength ) ? actuallength : 0;
    }

    // !!- OK -!!
    uint32_t CPack::PredictTotalFileSize()const
    {
        uint32_t filesizesofar = getCurrentPredictedHeaderLengthWithForcedOffset();

        for( auto &subfile : m_SubFiles )
            filesizesofar = CalculatePaddedLengthTotal( subfile.size() + filesizesofar, static_cast<size_t>(16u) );

        return filesizesofar;
    }

    // !!- OK -!!
    std::vector<uint8_t>::iterator CPack::WriteFullHeader( std::vector<uint8_t>::iterator writeat )const
    {
        assert( !m_OffsetTable.empty() );
        pfheader mahead;
        mahead._zeros   = 0;
        mahead._nbfiles = m_SubFiles.size();

        writeat = mahead.WriteToContainer( writeat );

        //Write FOT
        for( auto &fotentry : m_OffsetTable )
            writeat = fotentry.WriteToContainer(writeat);

        //Write FOT end delimiter
        writeat = std::copy( OFFSET_TBL_DELIM.begin(), OFFSET_TBL_DELIM.end(), writeat );

        //Write padding bytes
        writeat = std::fill_n( writeat, CalcAmountHeaderPaddingBytes(), PF_PADDING_BYTE );

        return writeat;
    }

    vector<uint8_t>::iterator CPack::WriteFileData( vector<uint8_t>::iterator writeat )
    {
        //Add file data, and add padding after those that need it !
        for( auto & afile : m_SubFiles )
        {
            writeat = copy( afile.begin(), afile.end(), writeat );

            //Write padding
            writeat = fill_n( writeat, 
                              ComputeFileNBPaddingBytes( afile.size() ), 
                              PF_PADDING_BYTE );
        }

        return writeat;
    }

    string SubfileGetFExtension( vector<uint8_t>::const_iterator beg, vector<uint8_t>::const_iterator end )
    {
        string result = pmd2::filetypes::GetAppropriateFileExtension(beg,end);
        if( result.empty() )
            return std::move( result );
        else
            return "." + result;
    }

    void CPack::WriteSubFileToFile( vector<uint8_t>  & file,
                                    const std::string & path, 
                                    unsigned int        fileindex )
    {
        //static const string FILE_PREFIX = "file_";

		//----- 1. Make output filename -----
		stringstream outfilename;
        Poco::Path   outpath(path);
        outpath.makeFile();

        outfilename << utils::TryAppendSlash( path ) << (outpath.getBaseName()) <<"_"
                    <<std::setfill('0') <<std::setw(4) <<std::dec <<fileindex
                    <<"_0x" 
                    <<std::setfill('0') <<std::setw(4) <<std::hex << m_OffsetTable[fileindex]._fileOffset
                    << SubfileGetFExtension( file.begin(), file.end() );

		//------- 2. Output -------
        WriteByteVectorToFile( outfilename.str(), file ); 
    }

//========================================================================================================
//  packfile_rule
//========================================================================================================

    /*
        packfile_rule
            Rule for identifying a pack file.
    */
    class packfile_rule : public IContentHandlingRule
    {
    public:
        packfile_rule(){}
        ~packfile_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const;

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        cntRID_t getRuleID()const;
        void              setRuleID( cntRID_t id );

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        //virtual ContentBlock Analyse( types::constitbyte_t   itdatabeg, 
        //                              types::constitbyte_t   itdataend );
        virtual ContentBlock Analyse( const analysis_parameter & parameters );

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  std::vector<uint8_t>::const_iterator itdatabeg, 
                               std::vector<uint8_t>::const_iterator itdataend,
                               const std::string                  & filext);

    private:
        cntRID_t m_myID;
    };


    //Returns the value from the content type enum to represent what this container contains!
    cnt_t packfile_rule::getContentType()const
    {
        return CnTy_PackFile;
    }

    //Returns an ID number identifying the rule. Its not the index in the storage array,
    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //IDs are assigned on registration of the rule by the handler.
    cntRID_t packfile_rule::getRuleID()const
    {
        return m_myID;
    }
    void packfile_rule::setRuleID( cntRID_t id )
    {
        m_myID = id;
    }

    //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
    ContentBlock packfile_rule::Analyse( const analysis_parameter & parameters )
    {
        pfheader     headr;
        ContentBlock cb;

        //Read the header
        headr.ReadFromContainer( parameters._itdatabeg, parameters._itdataend );

        //Get the last entry in the FOT and add its len to its offset to get the end offset!
        auto itlastentry = parameters._itdatabeg;
        advance( itlastentry, OFFSET_TBL_FIRST_ENTRY + ( headr._nbfiles * SZ_OFFSET_TBL_ENTRY ) );

        fileIndex lastentry;
        lastentry.ReadFromContainer( itlastentry, parameters._itdataend );

        //build our content block info
        cb._startoffset          = 0;
        cb._endoffset            = lastentry._fileOffset + lastentry._fileLength;
        cb._rule_id_that_matched = getRuleID();
        cb._type                 = getContentType();

        return cb;
    }

    //This method is a quick boolean test to determine quickly if this content handling
    // rule matches, without in-depth analysis.
    bool packfile_rule::isMatch( std::vector<uint8_t>::const_iterator itdatabeg, std::vector<uint8_t>::const_iterator itdataend , const std::string & filext )
    {
        pfheader headr;
        itdatabeg = headr.ReadFromContainer( itdatabeg, itdataend );
        uint32_t nextint = utils::ReadIntFromBytes<uint32_t>(itdatabeg, itdataend); //We know that the next value is the first entry in the ToC, if its really a pack file!
        
        bool     filextokornotthere = filext.compare( pmd2::filetypes::PACK_FILEX ) == 0;

        return (headr._zeros == 0) && (headr._nbfiles > 0) && (nextint != 0);  //TODO: improve this, it fails and recognize files that aren't pack files !!
    }

//========================================================================================================
//  at4px_rule_registrator
//========================================================================================================
    /*
        at4px_rule_registrator
            A small singleton that has for only task to register the at4px_rule!
    */
    template<> RuleRegistrator<packfile_rule> RuleRegistrator<packfile_rule>::s_instance;

};