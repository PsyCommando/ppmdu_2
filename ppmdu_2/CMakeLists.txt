# CMakeList.txt : CMake project for ppmdu_2, include source and define
# project specific logic here.
#

# Add source files
#include(FetchContent)
#include(FindZLIB)
#include(FindPNG)

# Output dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_compile_definitions(USE_PPMDU_CONTENT_TYPE_ANALYSER)


#find_package(pugixml REQUIRED)
#find_package(Poco REQUIRED Foundation)
#find_package(Poco REQUIRED Util)
#find_package(ZLIB REQUIRED)
#find_package(PNG REQUIRED)
# include_directories(
#    "include"
#    "../${ppmdu_2_DEPS_DIRNAME}/whereami/src" 
#    "../${ppmdu_2_DEPS_DIRNAME}/EasyBMP" 
#    "../${ppmdu_2_DEPS_DIRNAME}/png++"
#    "../${ppmdu_2_DEPS_DIRNAME}/jdksmidi/include"
#)



#################################################
# PPMDU Lib
#################################################
set(ppmdu_SRC
    "src/dse/bgm_container.cpp"
    "src/dse/dse_common.cpp"
    "src/dse/dse_containers.cpp"
    "src/dse/dse_conversion.cpp"
    "src/dse/dse_conversion_info.cpp"
    "src/dse/dse_interpreter.cpp"
    "src/dse/dse_interpreter_events.cpp"
    "src/dse/dse_prgmbank_xml_io.cpp"
    "src/dse/dse_sequence.cpp"
    "src/dse/sample_processor.cpp"

    "src/ext_fmts/adpcm.cpp"
    "src/ext_fmts/bmp_io.cpp"
    "src/ext_fmts/midi_fmtrule.cpp"
    "src/ext_fmts/png_io.cpp"
    "src/ext_fmts/rawimg_io.cpp"
    "src/ext_fmts/riff_palette.cpp"
    "src/ext_fmts/sf2.cpp"
    "src/ext_fmts/supported_io.cpp"
    "src/ext_fmts/supported_io_info.cpp"
    "src/ext_fmts/txt_palette_io.cpp"

    "src/ppmdu/containers/color.cpp"
    "src/ppmdu/containers/item_data.cpp"
    "src/ppmdu/containers/item_data_xml_io.cpp"
#    "src/ppmdu/containers/level_tileset.cpp"
#    "src/ppmdu/containers/level_tileset_list.cpp"
    "src/ppmdu/containers/level_xml_io.cpp"
    "src/ppmdu/containers/move_data_xml_io.cpp"
    "src/ppmdu/containers/pokemon_stats.cpp"
    "src/ppmdu/containers/pokemon_stats_xml_io.cpp"
    "src/ppmdu/containers/script_content.cpp"
    "src/ppmdu/containers/sprite_data.cpp"
    "src/ppmdu/containers/sprite_io.cpp"
    "src/ppmdu/containers/sprite_xml_io.cpp"

    "src/ppmdu/fmts/at4px.cpp"
    "src/ppmdu/fmts/bg_list_data.cpp"
    "src/ppmdu/fmts/bgp.cpp"
    "src/ppmdu/fmts/bpl.cpp"
    "src/ppmdu/fmts/item_p.cpp"
    "src/ppmdu/fmts/kao.cpp"
    "src/ppmdu/fmts/lsd.cpp"
    "src/ppmdu/fmts/m_level.cpp"
    "src/ppmdu/fmts/mappa.cpp"
    "src/ppmdu/fmts/monster_data.cpp"
    "src/ppmdu/fmts/pack_file.cpp"
    "src/ppmdu/fmts/pkdpx.cpp"
    "src/ppmdu/fmts/pmd2_fontdata.cpp"
    "src/ppmdu/fmts/px_compression.cpp"
    "src/ppmdu/fmts/raw_rgbx32_palette_rule.cpp"
    "src/ppmdu/fmts/sir0.cpp"
    "src/ppmdu/fmts/ssa.cpp"
    "src/ppmdu/fmts/ssb.cpp"
    "src/ppmdu/fmts/text_str.cpp"
    "src/ppmdu/fmts/wan.cpp"
    "src/ppmdu/fmts/wan_writer.cpp"
    "src/ppmdu/fmts/waza_p.cpp"
    "src/ppmdu/fmts/wte.cpp"
    "src/ppmdu/fmts/wtu.cpp"
    "src/ppmdu/fmts/sedl.cpp"
    "src/ppmdu/fmts/smdl.cpp"
    "src/ppmdu/fmts/swdl.cpp"

    "src/ppmdu/pmd2/game_stats.cpp"
    "src/ppmdu/pmd2/pmd2.cpp"
    "src/ppmdu/pmd2/pmd2_asm.cpp"
    "src/ppmdu/pmd2/pmd2_asm_data.cpp"
    "src/ppmdu/pmd2/pmd2_configloader.cpp"
    "src/ppmdu/pmd2/pmd2_filetypes.cpp"
    "src/ppmdu/pmd2/pmd2_gameloader.cpp"
    "src/ppmdu/pmd2/pmd2_graphics.cpp"
    "src/ppmdu/pmd2/pmd2_image_formats.cpp"
#    "src/ppmdu/pmd2/pmd2_levels.cpp"
    "src/ppmdu/pmd2/pmd2_palettes.cpp"
    "src/ppmdu/pmd2/pmd2_scripts.cpp"
    "src/ppmdu/pmd2/pmd2_scripts_opcodes.cpp"
    "src/ppmdu/pmd2/pmd2_scripts_xml_io.cpp"
    "src/ppmdu/pmd2/pmd2_sprites.cpp"
    "src/ppmdu/pmd2/pmd2_text.cpp"
    "src/ppmdu/pmd2/pmd2_text_io.cpp"
    "src/ppmdu/pmd2/pmd2_xml_sniffer.cpp"
    "src/ppmdu/pmd2/script_processing.cpp"
    "src/ppmdu/pmd2/sprite_rle.cpp"

    "src/types/content_type_analyser.cpp"
    "src/types/contentid_generator.cpp"

    "src/utils/cmdline_util.cpp"
    "src/utils/gbyteutils.cpp"
    "src/utils/gfileio.cpp"
    "src/utils/gfileutil.cpp"
    "src/utils/library_wide.cpp"
    "src/utils/multiple_task_handler.cpp"
    "src/utils/multithread_logger.cpp"
    "src/utils/parallel_tasks.cpp"
    "src/utils/poco_wrapper.cpp"
    "src/utils/pugixml_utils.cpp"
    "src/utils/utility.cpp"
    "src/utils/uuid_gen_wrapper.cpp"
    "src/utils/whereami_wrapper.cpp"
)
set(ppmdu_HEADER
    "include/dse/bgm_blob.hpp"
    "include/dse/bgm_container.hpp"
    "include/dse/dse_common.hpp"
    "include/dse/dse_containers.hpp"
    "include/dse/dse_conversion.hpp"
    "include/dse/dse_conversion_info.hpp"
    "include/dse/dse_interpreter.hpp"
    "include/dse/dse_sequence.hpp"
    "include/dse/dse_to_xml.hpp"
    "include/dse/sadl.hpp"

    "include/ext_fmts/adpcm.hpp"
    "include/ext_fmts/bmp_io.hpp"
    "include/ext_fmts/midi_fmtrule.hpp"
    "include/ext_fmts/png_io.hpp"
    "include/ext_fmts/rawimg_io.hpp"
    "include/ext_fmts/riff.hpp"
    "include/ext_fmts/riff_palette.hpp"
    "include/ext_fmts/sf2.hpp"
    "include/ext_fmts/supported_io.hpp"
    "include/ext_fmts/supported_io_info.hpp"
    "include/ext_fmts/txt_palette_io.hpp"
    "include/ext_fmts/wav_io.hpp"

    "include/ppmdu/containers/base_image.hpp"
    "include/ppmdu/containers/color.hpp"
    "include/ppmdu/containers/img_pixel.hpp"
    "include/ppmdu/containers/index_iterator.hpp"
    "include/ppmdu/containers/item_data.hpp"
#    "include/ppmdu/containers/level_tileset.hpp"
#    "include/ppmdu/containers/level_tileset_list.hpp"
    "include/ppmdu/containers/linear_image.hpp"
    "include/ppmdu/containers/move_data.hpp"
    "include/ppmdu/containers/pokemon_stats.hpp"
    "include/ppmdu/containers/script_content.hpp"
    "include/ppmdu/containers/sprite_data.hpp"
    "include/ppmdu/containers/sprite_io.hpp"
    "include/ppmdu/containers/tiled_image.hpp"

    "include/ppmdu/fmts/at4px.hpp"
    "include/ppmdu/fmts/bg_list_data.hpp"
    "include/ppmdu/fmts/bgp.hpp"
    "include/ppmdu/fmts/bma.hpp"
    "include/ppmdu/fmts/bpa.hpp"
    "include/ppmdu/fmts/bpc.hpp"
    "include/ppmdu/fmts/bpc_compression.hpp"
    "include/ppmdu/fmts/bpl.hpp"
    "include/ppmdu/fmts/integer_encoding.hpp"
    "include/ppmdu/fmts/item_p.hpp"
    "include/ppmdu/fmts/kao.hpp"
    "include/ppmdu/fmts/lsd.hpp"
    "include/ppmdu/fmts/m_level.hpp"
    "include/ppmdu/fmts/mappa.hpp"
    "include/ppmdu/fmts/monster_data.hpp"
    "include/ppmdu/fmts/pack_file.hpp"
    "include/ppmdu/fmts/pkdpx.hpp"
    "include/ppmdu/fmts/pmd2_fontdata.hpp"
    "include/ppmdu/fmts/px_compression.hpp"
    "include/ppmdu/fmts/sir0.hpp"
    "include/ppmdu/fmts/ssa.hpp"
    "include/ppmdu/fmts/ssb.hpp"
    "include/ppmdu/fmts/text_str.hpp"
    "include/ppmdu/fmts/wan.hpp"
    "include/ppmdu/fmts/waza_p.hpp"
    "include/ppmdu/fmts/wte.hpp"
    "include/ppmdu/fmts/wtu.hpp"
    "include/ppmdu/fmts/sedl.hpp"
    "include/ppmdu/fmts/smdl.hpp"
    "include/ppmdu/fmts/swdl.hpp"

    "include/ppmdu/pmd2/dungeon_rng_data.hpp"
    "include/ppmdu/pmd2/game_stats.hpp"
    "include/ppmdu/pmd2/level_data.hpp"
    "include/ppmdu/pmd2/pmd2.hpp"
    "include/ppmdu/pmd2/pmd2_asm.hpp"
    "include/ppmdu/pmd2/pmd2_asm_data.hpp"
    "include/ppmdu/pmd2/pmd2_audio.hpp"
    "include/ppmdu/pmd2/pmd2_configloader.hpp"
    "include/ppmdu/pmd2/pmd2_filetypes.hpp"
    "include/ppmdu/pmd2/pmd2_gamedataio.hpp"
    "include/ppmdu/pmd2/pmd2_gameloader.hpp"
    "include/ppmdu/pmd2/pmd2_graphics.hpp"
    "include/ppmdu/pmd2/pmd2_hcdata.hpp"
    "include/ppmdu/pmd2/pmd2_image_formats.hpp"
    "include/ppmdu/pmd2/pmd2_levels.hpp"
    "include/ppmdu/pmd2/pmd2_palettes.hpp"
    "include/ppmdu/pmd2/pmd2_scripts.hpp"
    "include/ppmdu/pmd2/pmd2_scripts_opcodes.hpp"
    "include/ppmdu/pmd2/pmd2_sprites.hpp"
    "include/ppmdu/pmd2/pmd2_text.hpp"
    "include/ppmdu/pmd2/pmd2_xml_sniffer.hpp"
    "include/ppmdu/pmd2/script_processing.hpp"
    "include/ppmdu/pmd2/sprite_rle.hpp"

    "include/types/content_type_analyser.hpp"
    "include/types/contentid_generator.hpp"

    "include/utils/cmdline_util.hpp"
    "include/utils/cmdline_util_runner.hpp"
    "include/utils/gbyteutils.hpp"
    "include/utils/gfileio.hpp"
    "include/utils/gfileutils.hpp"
    "include/utils/gstringutils.hpp"
    "include/utils/handymath.hpp"
    "include/utils/library_wide.hpp"
    "include/utils/multiple_task_handler.hpp"
    "include/utils/multithread_logger.hpp"
    "include/utils/parallel_tasks.hpp"
    "include/utils/parse_utils.hpp"
    "include/utils/poco_wrapper.hpp"
    "include/utils/pugixml_utils.hpp"
    "include/utils/utility.hpp"
    "include/utils/uuid_gen_wrapper.hpp"
    "include/utils/whereami_wrapper.hpp"
)

#add_library(ppmdu SHARED ${ppmdu_SRC} ${ppmdu_HEADER})

#set(ppmdu_VERSION 1.0.0)
#if (CMAKE_VERSION VERSION_GREATER 3.12)
#  set_property(TARGET ppmdu PROPERTY CXX_STANDARD 17)
#endif()

#execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
#                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
#                OUTPUT_VARIABLE PACKAGE_GIT_VERSION
#                ERROR_QUIET
#                OUTPUT_STRIP_TRAILING_WHITESPACE)

#target_link_libraries(ppmdu PRIVATE whereami pugixml jdksmidi easybmp pugixml::shared pugixml::pugixml Poco::Foundation Poco::Util PNG::PNG ZLIB::ZLIB)
