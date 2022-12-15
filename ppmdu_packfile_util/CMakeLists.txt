###########################################################
# Pack File Utility HEADER
###########################################################
list(APPEND ppmdu_packfile_util_HEADER 
    "../ppmdu_2/include/ppmdu/fmts/at4px.hpp"
    "../ppmdu_2/include/ppmdu/fmts/integer_encoding.hpp"
    "../ppmdu_2/include/ppmdu/fmts/kao.hpp"
    "../ppmdu_2/include/ppmdu/fmts/lsd.hpp"
    "../ppmdu_2/include/ppmdu/fmts/pack_file.hpp"
    "../ppmdu_2/include/ppmdu/fmts/pkdpx.hpp"
    "../ppmdu_2/include/ppmdu/fmts/px_compression.hpp"
    "../ppmdu_2/include/ppmdu/fmts/sir0.hpp"
    "../ppmdu_2/include/ppmdu/fmts/text_str.hpp"

    "../ppmdu_2/include/ppmdu/pmd2/pmd2.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_filetypes.hpp"

    "../ppmdu_2/include/types/content_type_analyser.hpp"
    "../ppmdu_2/include/types/contentid_generator.hpp"

    "../ppmdu_2/include/utils/cmdline_util.hpp"
    "../ppmdu_2/include/utils/cmdline_util_runner.hpp"
    "../ppmdu_2/include/utils/gbyteutils.hpp"
    "../ppmdu_2/include/utils/gfileio.hpp"
    "../ppmdu_2/include/utils/gfileutils.hpp"
    "../ppmdu_2/include/utils/gstringutils.hpp"
    "../ppmdu_2/include/utils/handymath.hpp"
    "../ppmdu_2/include/utils/library_wide.hpp"
    "../ppmdu_2/include/utils/multiple_task_handler.hpp"
    "../ppmdu_2/include/utils/multithread_logger.hpp"
    "../ppmdu_2/include/utils/parallel_tasks.hpp"
    "../ppmdu_2/include/utils/parse_utils.hpp"
    "../ppmdu_2/include/utils/poco_wrapper.hpp"
    "../ppmdu_2/include/utils/utility.hpp"
    "../ppmdu_2/include/utils/uuid_gen_wrapper.hpp"
    "../ppmdu_2/include/utils/whereami_wrapper.hpp"
)
###########################################################
# Pack File Utility SRC
###########################################################
list(APPEND ppmdu_packfile_util_SRC 

    "../ppmdu_2/src/ppmdu/fmts/at4px.cpp"
    "../ppmdu_2/src/ppmdu/fmts/pack_file.cpp"
    "../ppmdu_2/src/ppmdu/fmts/pkdpx.cpp"
    "../ppmdu_2/src/ppmdu/fmts/px_compression.cpp"
    "../ppmdu_2/src/ppmdu/fmts/sir0.cpp"

    "../ppmdu_2/src/ppmdu/pmd2/pmd2.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_filetypes.cpp"

    "../ppmdu_2/src/types/content_type_analyser.cpp"
    "../ppmdu_2/src/types/contentid_generator.cpp"

    "../ppmdu_2/src/utils/cmdline_util.cpp"
    "../ppmdu_2/src/utils/gbyteutils.cpp"
    "../ppmdu_2/src/utils/gfileio.cpp"
    "../ppmdu_2/src/utils/gfileutil.cpp"
    "../ppmdu_2/src/utils/library_wide.cpp"
    "../ppmdu_2/src/utils/multiple_task_handler.cpp"
    "../ppmdu_2/src/utils/multithread_logger.cpp"
    "../ppmdu_2/src/utils/parallel_tasks.cpp"
    "../ppmdu_2/src/utils/poco_wrapper.cpp"
    "../ppmdu_2/src/utils/utility.cpp"
    "../ppmdu_2/src/utils/uuid_gen_wrapper.cpp"
    "../ppmdu_2/src/utils/whereami_wrapper.cpp"
)

###########################################################
# Pack File Utility Build Stuff
###########################################################
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_executable(ppmdu_packfile_util ${ppmdu_packfile_util_SRC} "src/ppmd_packfileutil.cpp" ${ppmdu_packfile_util_HEADER} "src/ppmd_packfileutil.hpp")
set(ppmdu_packfile_util_VERSION 0.54.0)

add_compile_definitions(USE_PPMDU_CONTENT_TYPE_ANALYSER)
add_compile_definitions(PACKFILE_UTIL_VER="${ppmdu_packfile_util_VERSION}")
add_compile_definitions(_REMOVE_FPOS_SEEKPOS)

include_directories(ppmdu_packfile_util 
    "../ppmdu_2/include"
    "../${ppmdu_2_DEPS_DIRNAME}/whereami/src"
)

find_package(Poco REQUIRED Foundation)
find_package(Poco REQUIRED Util)

target_link_libraries(ppmdu_packfile_util
    whereami
    Poco::Foundation 
    Poco::Util 
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ppmdu_packfile_util PROPERTY CXX_STANDARD 17)
endif()