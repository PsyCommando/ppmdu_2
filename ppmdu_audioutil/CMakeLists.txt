###########################################################
# Header
###########################################################
list(APPEND audioutil_HEADER 
    "../ppmdu_2/include/ppmdu/pmd2/game_stats.hpp"
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

    "../ppmdu_2/include/utils/audio_utilities.hpp"
    "../ppmdu_2/include/utils/cmdline_util.hpp"
    "../ppmdu_2/include/utils/cmdline_util_runner.hpp"
    "../ppmdu_2/include/utils/gbyteutils.hpp"
    "../ppmdu_2/include/utils/gfileio.hpp"
    "../ppmdu_2/include/utils/gfileutils.hpp"
    "../ppmdu_2/include/utils/gstringutils.hpp"
    "../ppmdu_2/include/utils/handymath.hpp"
    "../ppmdu_2/include/utils/library_wide.hpp"
    "../ppmdu_2/include/utils/midi_helpers.hpp"
    "../ppmdu_2/include/utils/multiple_task_handler.hpp"
    "../ppmdu_2/include/utils/multithread_logger.hpp"
    "../ppmdu_2/include/utils/parallel_tasks.hpp"
    "../ppmdu_2/include/utils/parse_utils.hpp"
    "../ppmdu_2/include/utils/poco_wrapper.hpp"
    "../ppmdu_2/include/utils/pugixml_utils.hpp"
    "../ppmdu_2/include/utils/utility.hpp"
    "../ppmdu_2/include/utils/uuid_gen_wrapper.hpp"
    "../ppmdu_2/include/utils/whereami_wrapper.hpp"

    #DSE Stuff
    "../ppmdu_2/include/dse/containers/dse_bnkl.hpp"
    "../ppmdu_2/include/dse/containers/dse_keygroup_list.hpp"
    "../ppmdu_2/include/dse/containers/dse_mcrl.hpp"
    "../ppmdu_2/include/dse/containers/dse_music_sequence.hpp"
    "../ppmdu_2/include/dse/containers/dse_music_track.hpp"
    "../ppmdu_2/include/dse/containers/dse_preset_bank.hpp"
    "../ppmdu_2/include/dse/containers/dse_preset_db.hpp"
    "../ppmdu_2/include/dse/containers/dse_program_bank.hpp"
    "../ppmdu_2/include/dse/containers/dse_sample_bank.hpp"
    "../ppmdu_2/include/dse/containers/dse_se_sequence.hpp"

    "../ppmdu_2/include/dse/fmts/sadl.hpp"
    "../ppmdu_2/include/dse/fmts/sedl.hpp"
    "../ppmdu_2/include/dse/fmts/smdl.hpp"
    "../ppmdu_2/include/dse/fmts/swdl.hpp"

    "../ppmdu_2/include/dse/bgm_blob.hpp"
    "../ppmdu_2/include/dse/dse_common.hpp"
    "../ppmdu_2/include/dse/dse_conversion.hpp"
    "../ppmdu_2/include/dse/dse_conversion_info.hpp"
    "../ppmdu_2/include/dse/dse_interpreter.hpp"
    "../ppmdu_2/include/dse/dse_sequence.hpp"
    "../ppmdu_2/include/dse/dse_to_sf2.hpp"
    "../ppmdu_2/include/dse/dse_to_xml.hpp"

    "../ppmdu_2/include/ext_fmts/adpcm.hpp"
    "../ppmdu_2/include/ext_fmts/midi_fmtrule.hpp"
    "../ppmdu_2/include/ext_fmts/riff.hpp"
    "../ppmdu_2/include/ext_fmts/sf2.hpp"
    "../ppmdu_2/include/ext_fmts/wav_io.hpp"
    
)

###########################################################
# SRC
###########################################################
list(APPEND audioutil_SRC 
    "../ppmdu_2/src/ppmdu/pmd2/pmd2.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_asm.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_configloader.cpp"
    "../ppmdu_2/src/ppmdu/pmd2/pmd2_filetypes.cpp"
    #"../ppmdu_2/src/ppmdu/pmd2/pmd2_gameloader.cpp"
    #"../ppmdu_2/src/ppmdu/pmd2/pmd2_scripts.cpp"
    #"../ppmdu_2/src/ppmdu/pmd2/pmd2_text.cpp"

    "../ppmdu_2/src/types/content_type_analyser.cpp"
    "../ppmdu_2/src/types/contentid_generator.cpp"

    "../ppmdu_2/src/utils/audio_utilities.cpp"
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

    #DSE Stuff
    "../ppmdu_2/src/dse/containers/dse_preset_db.cpp"
    "../ppmdu_2/src/dse/containers/dse_music_sequence.cpp"

    "../ppmdu_2/src/dse/fmts/sedl.cpp"
    "../ppmdu_2/src/dse/fmts/smdl.cpp"
    "../ppmdu_2/src/dse/fmts/swdl.cpp"

	"../ppmdu_2/src/dse/bgm_container.cpp"
    "../ppmdu_2/src/dse/dse_common.cpp"
    "../ppmdu_2/src/dse/dse_conversion.cpp"
    "../ppmdu_2/src/dse/dse_conversion_info.cpp"
    "../ppmdu_2/src/dse/dse_interpreter.cpp"
    "../ppmdu_2/src/dse/dse_interpreter_events.cpp"
    "../ppmdu_2/src/dse/dse_loader.cpp"
    "../ppmdu_2/src/dse/dse_midi_track_parser.cpp"
    "../ppmdu_2/src/dse/dse_midi_track_writer.cpp"
    "../ppmdu_2/src/dse/dse_preset_bank_xml_io.cpp"
    "../ppmdu_2/src/dse/dse_se_sequence_xml_io.cpp"
    "../ppmdu_2/src/dse/dse_sequence.cpp"
    "../ppmdu_2/src/dse/dse_sequence_xml_io.cpp"
    "../ppmdu_2/src/dse/sample_processor.cpp"
    "../ppmdu_2/src/dse/dse_to_sf2.cpp"
    "../ppmdu_2/src/dse/dse_to_xml.cpp"

    "../ppmdu_2/src/ext_fmts/adpcm.cpp"
    "../ppmdu_2/src/ext_fmts/midi_fmtrule.cpp"
    "../ppmdu_2/src/ext_fmts/sf2.cpp"
)

###########################################################
# Build Stuff
###########################################################
# Output dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_executable(
    audioutil 
    ${audioutil_SRC} 
    "src/audioutil.cpp" 
    "src/audioutil_export.cpp" 
    "src/audioutil_import.cpp" 
    ${audioutil_HEADER} 
    "src/audioutil.hpp" 
)
set(audioutil_VERSION 0.39.0)

add_compile_definitions(USE_PPMDU_CONTENT_TYPE_ANALYSER)
add_compile_definitions(AUDIOUTIL_VER="${audioutil_VERSION}")
add_compile_definitions(_REMOVE_FPOS_SEEKPOS)

include_directories(audioutil 
    "../ppmdu_2/include"
    "../${ppmdu_2_DEPS_DIRNAME}/whereami/src"
    "../${ppmdu_2_DEPS_DIRNAME}/jdksmidi/include"
)

find_package(pugixml CONFIG REQUIRED)
find_package(Poco REQUIRED Foundation)
find_package(Poco REQUIRED JSON)
find_package(Poco REQUIRED Util)

target_link_libraries(audioutil 
    whereami 
    jdksmidi 
    pugixml 
    Poco::Foundation 
    Poco::Util
    Poco::JSON
)


file(
    COPY 
        "../ppmdu_2/resources/pmd2_to_gm.xml"
        "../ppmdu_2/resources/pmd2data.xml"
        "../ppmdu_2/resources/pmd2eos_cvinfo.xml"
        "../ppmdu_2/readmes/ppmd_audioutil.txt"
    DESTINATION 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)