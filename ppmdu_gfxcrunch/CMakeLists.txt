###########################################################
# GFX Crunch HEADER
###########################################################
list(APPEND ppmdu_gfxcrunch_HEADER 
    "../ppmdu_2/include/ext_fmts/bmp_io.hpp"
    "../ppmdu_2/include/ext_fmts/png_io.hpp"
    "../ppmdu_2/include/ext_fmts/rawimg_io.hpp"
    "../ppmdu_2/include/ext_fmts/riff.hpp"
    "../ppmdu_2/include/ext_fmts/riff_palette.hpp"
    "../ppmdu_2/include/ext_fmts/supported_io.hpp"
    "../ppmdu_2/include/ext_fmts/supported_io_info.hpp"
    "../ppmdu_2/include/ext_fmts/txt_palette_io.hpp"

    "../ppmdu_2/include/ppmdu/containers/base_image.hpp"
    "../ppmdu_2/include/ppmdu/containers/color.hpp"
    "../ppmdu_2/include/ppmdu/containers/img_pixel.hpp"
    "../ppmdu_2/include/ppmdu/containers/index_iterator.hpp"
    "../ppmdu_2/include/ppmdu/containers/linear_image.hpp"
    "../ppmdu_2/include/ppmdu/containers/sprite_data.hpp"
    "../ppmdu_2/include/ppmdu/containers/sprite_io.hpp"
    "../ppmdu_2/include/ppmdu/containers/tiled_image.hpp"

    "../ppmdu_2/include/ppmdu/fmts/at4px.hpp"
    "../ppmdu_2/include/ppmdu/fmts/bg_list_data.hpp"
    "../ppmdu_2/include/ppmdu/fmts/bgp.hpp"
    "../ppmdu_2/include/ppmdu/fmts/bma.hpp"
    "../ppmdu_2/include/ppmdu/fmts/bpa.hpp"
    "../ppmdu_2/include/ppmdu/fmts/bpc.hpp"
    "../ppmdu_2/include/ppmdu/fmts/bpc_compression.hpp"
    "../ppmdu_2/include/ppmdu/fmts/bpl.hpp"
    "../ppmdu_2/include/ppmdu/fmts/integer_encoding.hpp"
    "../ppmdu_2/include/ppmdu/fmts/kao.hpp"
    "../ppmdu_2/include/ppmdu/fmts/pack_file.hpp"
    "../ppmdu_2/include/ppmdu/fmts/pkdpx.hpp"
    "../ppmdu_2/include/ppmdu/fmts/pmd2_fontdata.hpp"
    "../ppmdu_2/include/ppmdu/fmts/px_compression.hpp"
    "../ppmdu_2/include/ppmdu/fmts/sir0.hpp"
    "../ppmdu_2/include/ppmdu/fmts/text_str.hpp"
    "../ppmdu_2/include/ppmdu/fmts/wan.hpp"
    "../ppmdu_2/include/ppmdu/fmts/wte.hpp"
    "../ppmdu_2/include/ppmdu/fmts/wtu.hpp"

    "../ppmdu_2/include/ppmdu/pmd2/game_stats.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/level_data.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_asm.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_configloader.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_filetypes.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_gamedataio.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_gameloader.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_graphics.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_hcdata.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_image_formats.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_levels.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_palettes.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_sprites.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_text.hpp"
    "../ppmdu_2/include/ppmdu/pmd2/pmd2_xml_sniffer.hpp"
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
# GFX Crunch SRC
###########################################################
list(APPEND ppmdu_gfxcrunch_SRC 
    "../ppmdu_2/src/ext_fmts/bmp_io.cpp"
    "../ppmdu_2/src/ext_fmts/png_io.cpp"
    "../ppmdu_2/src/ext_fmts/rawimg_io.cpp"
    "../ppmdu_2/src/ext_fmts/riff_palette.cpp"
    "../ppmdu_2/src/ext_fmts/supported_io.cpp"
    "../ppmdu_2/src/ext_fmts/supported_io_info.cpp"
    "../ppmdu_2/src/ext_fmts/txt_palette_io.cpp"

    "../ppmdu_2/src/ppmdu/containers/color.cpp"
    "../ppmdu_2/src/ppmdu/containers/sprite_data.cpp"
    "../ppmdu_2/src/ppmdu/containers/sprite_io.cpp"
    "../ppmdu_2/src/ppmdu/containers/sprite_xml_io.cpp"

    "../ppmdu_2/src/ppmdu/fmts/at4px.cpp"
    "../ppmdu_2/src/ppmdu/fmts/bg_list_data.cpp"
    "../ppmdu_2/src/ppmdu/fmts/bgp.cpp"
    "../ppmdu_2/src/ppmdu/fmts/bpl.cpp"
    "../ppmdu_2/src/ppmdu/fmts/bpc.cpp"
    "../ppmdu_2/src/ppmdu/fmts/kao.cpp"
    "../ppmdu_2/src/ppmdu/fmts/pack_file.cpp"
    "../ppmdu_2/src/ppmdu/fmts/pkdpx.cpp"
    "../ppmdu_2/src/ppmdu/fmts/pmd2_fontdata.cpp"
    "../ppmdu_2/src/ppmdu/fmts/px_compression.cpp"
    "../ppmdu_2/src/ppmdu/fmts/raw_rgbx32_palette_rule.cpp"
    "../ppmdu_2/src/ppmdu/fmts/sir0.cpp"
    "../ppmdu_2/src/ppmdu/fmts/text_str.cpp"
    "../ppmdu_2/src/ppmdu/fmts/wan.cpp"
    "../ppmdu_2/src/ppmdu/fmts/wan_writer.cpp"
    "../ppmdu_2/src/ppmdu/fmts/wte.cpp"
    "../ppmdu_2/src/ppmdu/fmts/wtu.cpp"

    "../ppmdu_2/src/ppmdu/pmd2/pmd2.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_asm.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_asm_data.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_configloader.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_filetypes.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_graphics.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_image_formats.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_palettes.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_sprites.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_text.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_text_io.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_xml_sniffer.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/sprite_rle.cpp"

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
# GFX Crunch Build Stuff 
###########################################################
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_executable(ppmdu_gfxcrunch ${ppmdu_gfxcrunch_SRC} "src/gfxcrunch.cpp" ${ppmdu_gfxcrunch_HEADER} "src/gfxcrunch.hpp")
set(ppmdu_gfxcrunch_VERSION 0.13.0)

add_compile_definitions(USE_PPMDU_CONTENT_TYPE_ANALYSER)
add_compile_definitions(GFXCRUNCH_VER="${ppmdu_gfxcrunch_VERSION}")
add_compile_definitions(_REMOVE_FPOS_SEEKPOS)

include_directories(ppmdu_gfxcrunch 
    "../ppmdu_2/include"
    "../${ppmdu_2_DEPS_DIRNAME}/whereami/src"
    "../${ppmdu_2_DEPS_DIRNAME}/png++"
    "../${ppmdu_2_DEPS_DIRNAME}/EasyBMP"
)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(Poco REQUIRED Foundation)
find_package(Poco REQUIRED Util)

target_link_libraries(ppmdu_gfxcrunch
    whereami
    easybmp 
    pugixml 
    Poco::Foundation 
    Poco::Util 
    ZLIB::ZLIB 
    PNG::PNG
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ppmdu_gfxcrunch PROPERTY CXX_STANDARD 17)
endif()

file(
    COPY 
        "../ppmdu_2/resources/pmd2data.xml"
        "../ppmdu_2/resources/animres.xml"
        "../ppmdu_2/resources/pokesprites_names.txt" #TODO: When we get the sprite names directly remove this
        "../ppmdu_2/readmes/ppmd_gfxcrunch.txt"
    DESTINATION 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)