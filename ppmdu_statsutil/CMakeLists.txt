###########################################################
# Stats Utility SRC
###########################################################
set(ppmdu_stats_util_SRC
    "../ppmdu_2/src/ppmdu/containers/item_data.cpp"
    "../ppmdu_2/src/ppmdu/containers/item_data_xml_io.cpp"
    "../ppmdu_2/src/ppmdu/containers/level_xml_io.cpp"
    "../ppmdu_2/src/ppmdu/containers/move_data_xml_io.cpp"
    "../ppmdu_2/src/ppmdu/containers/pokemon_stats.cpp"
    "../ppmdu_2/src/ppmdu/containers/pokemon_stats_xml_io.cpp"
    "../ppmdu_2/src/ppmdu/containers/script_content.cpp"

    "../ppmdu_2/src/ppmdu/fmts/at4px.cpp"
    "../ppmdu_2/src/ppmdu/fmts/item_p.cpp"
    "../ppmdu_2/src/ppmdu/fmts/lsd.cpp"
    "../ppmdu_2/src/ppmdu/fmts/m_level.cpp"
    "../ppmdu_2/src/ppmdu/fmts/mappa.cpp"
    "../ppmdu_2/src/ppmdu/fmts/monster_data.cpp"
    "../ppmdu_2/src/ppmdu/fmts/pack_file.cpp"
    "../ppmdu_2/src/ppmdu/fmts/pkdpx.cpp"
    "../ppmdu_2/src/ppmdu/fmts/px_compression.cpp"
    "../ppmdu_2/src/ppmdu/fmts/sir0.cpp"
    "../ppmdu_2/src/ppmdu/fmts/ssa.cpp"
    "../ppmdu_2/src/ppmdu/fmts/ssb.cpp"
    "../ppmdu_2/src/ppmdu/fmts/text_str.cpp"
    "../ppmdu_2/src/ppmdu/fmts/waza_p.cpp"

    "../ppmdu_2/src/ppmdu/pmd2/game_stats.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_asm.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_asm_data.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_configloader.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_filetypes.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_gameloader.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_scripts.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_scripts_opcodes.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_scripts_xml_io.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_text.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_text_io.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_xml_sniffer.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/script_processing.cpp"

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
    "../ppmdu_2/src/utils/pugixml_utils.cpp"
    "../ppmdu_2/src/utils/utility.cpp"
    "../ppmdu_2/src/utils/uuid_gen_wrapper.cpp"
    "../ppmdu_2/src/utils/whereami_wrapper.cpp"
)

###########################################################
# Stats Utility HEADER
###########################################################
set(ppmdu_stats_util_HEADER
    "../ppmdu_2/include/ppmdu/containers/item_data.hpp"
    "../ppmdu_2/include/ppmdu/containers/move_data.hpp"
    "../ppmdu_2/include/ppmdu/containers/pokemon_stats.hpp"
    "../ppmdu_2/include/ppmdu/containers/script_content.hpp"

    "../ppmdu_2/include/ppmdu/fmts/at4px.hpp"
    "../ppmdu_2/include/ppmdu/fmts/integer_encoding.hpp"
    "../ppmdu_2/include/ppmdu/fmts/item_p.hpp"
    "../ppmdu_2/include/ppmdu/fmts/lsd.hpp"
    "../ppmdu_2/include/ppmdu/fmts/m_level.hpp"
    "../ppmdu_2/include/ppmdu/fmts/mappa.hpp"
    "../ppmdu_2/include/ppmdu/fmts/monster_data.hpp"
    "../ppmdu_2/include/ppmdu/fmts/pack_file.hpp"
    "../ppmdu_2/include/ppmdu/fmts/pkdpx.hpp"
    "../ppmdu_2/include/ppmdu/fmts/px_compression.hpp"
    "../ppmdu_2/include/ppmdu/fmts/sir0.hpp"
    "../ppmdu_2/include/ppmdu/fmts/ssa.hpp"
    "../ppmdu_2/include/ppmdu/fmts/ssb.hpp"
    "../ppmdu_2/include/ppmdu/fmts/text_str.hpp"
    "../ppmdu_2/include/ppmdu/fmts/waza_p.hpp"

    "../ppmdu_2/include/ppmdu/pmd2/dungeon_rng_data.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/game_stats.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/level_data.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_asm.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_asm_data.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_audio.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_configloader.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_filetypes.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_gamedataio.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_gameloader.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_graphics.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_hcdata.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_image_formats.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_levels.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_palettes.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_scripts.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_scripts_opcodes.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_sprites.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_text.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_xml_sniffer.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/script_processing.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/sprite_rle.hpp"

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
    "../ppmdu_2/include/utils/pugixml_utils.hpp"
    "../ppmdu_2/include/utils/utility.hpp"
    "../ppmdu_2/include/utils/uuid_gen_wrapper.hpp"
    "../ppmdu_2/include/utils/whereami_wrapper.hpp"
)

###########################################################
# Stats Utility Build Stuff
###########################################################
# Output dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_executable(ppmdu_stats_util ${ppmdu_stats_util_SRC} "src/statsutil.cpp" ${ppmdu_stats_util_HEADER} "src/statsutil.hpp")
set(ppmdu_stats_util_VERSION 0.24.0)

add_compile_definitions(PPMDU_STATSUTIL) #Workaround for some compiling issues. Probably should be fixed eventually.
add_compile_definitions(USE_PPMDU_CONTENT_TYPE_ANALYSER)
add_compile_definitions(STATSUTIL_VER="${ppmdu_stats_util_VERSION}")
add_compile_definitions(_REMOVE_FPOS_SEEKPOS)

include_directories(ppmdu_stats_util
    "../ppmdu_2/include"
    "../${ppmdu_2_DEPS_DIRNAME}/whereami/src"
)

find_package(pugixml CONFIG REQUIRED)
find_package(Poco REQUIRED Foundation)
find_package(Poco REQUIRED Util)
target_link_libraries(ppmdu_stats_util 
    whereami 
    pugixml 
    Poco::Foundation 
    Poco::Util
)

file(
    COPY 
        "../ppmdu_2/resources/pmd2data.xml"
        "../ppmdu_2/resources/pmd2scriptdata.xml"
        "../ppmdu_2/readmes/ppmd_statsutil.txt"
    DESTINATION 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)