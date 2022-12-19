###########################################################
# PX Utilities HEADER
###########################################################
list(APPEND ppmdu_pxutil_HEADER 
    "../ppmdu_2/include/ppmdu/fmts/at4px.hpp"
    "../ppmdu_2/include/ppmdu/fmts/integer_encoding.hpp"
    "../ppmdu_2/include/ppmdu/fmts/pack_file.hpp"
    "../ppmdu_2/include/ppmdu/fmts/pkdpx.hpp"
    "../ppmdu_2/include/ppmdu/fmts/px_compression.hpp"
    "../ppmdu_2/include/ppmdu/fmts/sir0.hpp"
    "../ppmdu_2/include/ppmdu/fmts/ssa.hpp"
    "../ppmdu_2/include/ppmdu/fmts/ssb.hpp"
    "../ppmdu_2/include/ppmdu/fmts/text_str.hpp"
    "../ppmdu_2/include/ppmdu/fmts/wan.hpp"
    "../ppmdu_2/include/ppmdu/fmts/waza_p.hpp"
    "../ppmdu_2/include/ppmdu/fmts/wte.hpp"
    "../ppmdu_2/include/ppmdu/fmts/wtu.hpp"

    "../ppmdu_2/include/ppmdu/pmd2/pmd2.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_filetypes.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_image_formats.hpp"

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
# PX Utilities SRC
###########################################################
list(APPEND ppmdu_pxutil_SRC 
    "../ppmdu_2/src/ppmdu/fmts/at4px.cpp"
    "../ppmdu_2/src/ppmdu/fmts/pkdpx.cpp"
    "../ppmdu_2/src/ppmdu/fmts/px_compression.cpp"
    "../ppmdu_2/src/ppmdu/fmts/sir0.cpp"
    "../ppmdu_2/src/ppmdu/fmts/text_str.cpp"

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
# PX Utilities Build Stuff
###########################################################
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

file(
    COPY 
        "../ppmdu_2/readmes/ppmd_dopx.txt"
        "../ppmdu_2/readmes/ppmd_unpx.txt"
    DESTINATION 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

include_directories(
    "../ppmdu_2/include"
    "../${ppmdu_2_DEPS_DIRNAME}/whereami/src"
)

add_compile_definitions(USE_PPMDU_CONTENT_TYPE_ANALYSER)
add_compile_definitions(_REMOVE_FPOS_SEEKPOS)

find_package(Poco REQUIRED Foundation)
find_package(Poco REQUIRED Util)

###########################################################
# DoPX
###########################################################
add_executable(ppmdu_dopx ${ppmdu_pxutil_SRC} "src/ppx_compressor.cpp" ${ppmdu_pxutil_HEADER} "src/ppx_compressor.hpp" )
set(ppmdu_dopx_VERSION 0.13.0)
add_compile_definitions(DOPX_VER="${ppmdu_dopx_VERSION}")

target_link_libraries(ppmdu_dopx
    whereami
    Poco::Foundation 
    Poco::Util 
)

###########################################################
# UnPX
###########################################################
add_executable(ppmdu_unpx ${ppmdu_pxutil_SRC} "src/ppx_extractor.cpp" ${ppmdu_pxutil_HEADER} "src/ppx_extractor.hpp" )
set(ppmdu_unpx_VERSION 0.41.0)
add_compile_definitions(UNPX_VER="${ppmdu_unpx_VERSION}")

target_link_libraries(ppmdu_unpx
    whereami
    Poco::Foundation 
    Poco::Util 
)

