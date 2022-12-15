#include <utils/multithread_logger.hpp>
#include <utils/library_wide.hpp>

namespace logging
{
    const std::string & GetLibWideLogDirectory()
    {
        return utils::LibWide().StringValue(utils::lwData::eBasicValues::ProgramLogDir);
    }
};