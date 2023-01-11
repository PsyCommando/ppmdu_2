#include <dse/dse_sequence.hpp>
#include <dse/dse_common.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

namespace DSE
{

    static const TrkEventInfo InvalidEventInfo {eTrkEventCodes::Invalid,eTrkEventCodes::Invalid, 0, "INVALID" };

    const std::map<eTrkEventCodes, std::string> EvCodeToEvNames
    { {
        { eTrkEventCodes::RepeatLastPause,      "RepeatLastPause"       },
        { eTrkEventCodes::AddToLastPause,       "AddToLastPause"        },
        { eTrkEventCodes::Pause8Bits,           "Pause8Bits"            },
        { eTrkEventCodes::Pause16Bits,          "Pause16Bits"           },
        { eTrkEventCodes::Pause24Bits,          "Pause24Bits"           },
        { eTrkEventCodes::PauseUntilRel,        "PauseUntilRelease"     },
        { eTrkEventCodes::EndOfTrack,           "EndOfTrack"            },
        { eTrkEventCodes::LoopPointSet,         "LoopPoint"             },
        { eTrkEventCodes::RepeatFrom,           "RepeatFrom"            },
        { eTrkEventCodes::RepeatSegment,        "RepeatSegment"         },
        { eTrkEventCodes::AfterRepeat,          "AfterRepeat"           },
        { eTrkEventCodes::SetOctave,            "SetOctave"             },
        { eTrkEventCodes::AddOctave,            "AddOctave"             },
        { eTrkEventCodes::SetTempo,             "SetTempo"              },
        { eTrkEventCodes::SetTempo2,            "SetTempo2"             },
        { eTrkEventCodes::SetBank,              "SetBank"               },
        { eTrkEventCodes::SetBankHighByte,      "SetBankHighByte"       },
        { eTrkEventCodes::SetBankLowByte,       "SetBankLowByte"        },
        { eTrkEventCodes::SkipNextByte,         "SkipNextByte"          },
        { eTrkEventCodes::SetPreset,            "SetPreset"             },
        { eTrkEventCodes::FadeSongVolume,       "FadeSongVolume"        },
        { eTrkEventCodes::DisableEnvelope,      "DisableEnvelope"       },
        { eTrkEventCodes::SetEnvAtkLvl,         "SetEnvAtkLvl"          },
        { eTrkEventCodes::SetEnvAtkTime,        "SetEnvAtkTime"         },
        { eTrkEventCodes::SetEnvHold,           "SetEnvHold"            },
        { eTrkEventCodes::SetEnvDecSus,         "SetEnvDecSus"          },
        { eTrkEventCodes::SetEnvFade,           "SetEnvFade"            },
        { eTrkEventCodes::SetEnvRelease,        "SetEnvRelease"         },
        { eTrkEventCodes::SetNoteVol,           "SetNoteVol"            },
        { eTrkEventCodes::SetChanPan,           "SetChanPan"            },
        { eTrkEventCodes::Unk_0xBF,             "Unk_0xBF"              },
        { eTrkEventCodes::Unk_0xC0,             "Unk_0xC0"              },
        { eTrkEventCodes::SetChanVol,           "SetChanVol"            },
        { eTrkEventCodes::SkipNext2Bytes1,      "SkipNext2Bytes"        },
        { eTrkEventCodes::SetFTune,             "SetFTune"              },
        { eTrkEventCodes::AddFTune,             "AddFTune"              },
        { eTrkEventCodes::SetCTune,             "SetCTune"              },
        { eTrkEventCodes::AddCTune,             "AddCTune"              },
        { eTrkEventCodes::SweepTune,            "SweepTune"             },
        { eTrkEventCodes::SetRndNoteRng,        "SetRndNoteRng"         },
        { eTrkEventCodes::SetDetuneRng,         "SetDetuneRng"          },
        { eTrkEventCodes::SetPitchBend,         "SetPitchBend"          },
        { eTrkEventCodes::Unk_0xD8,             "Unk_0xD8"              },
        { eTrkEventCodes::SetPitchBendRng,      "SetPitchBendRng"       },
        { eTrkEventCodes::SetLFO1,              "SetLFO1"               },
        { eTrkEventCodes::SetLFO1DelayFade,     "SetLFO1DelayFade"      },
        { eTrkEventCodes::RouteLFO1ToPitch,     "RouteLFO1ToPitch"      },
        { eTrkEventCodes::SetTrkVol,            "SetVolume"             },
        { eTrkEventCodes::AddTrkVol,            "AddTrkVol"             },
        { eTrkEventCodes::SweepTrackVol,        "SweepTrackVol"         },
        { eTrkEventCodes::SetExpress,           "SetExpression"         },
        { eTrkEventCodes::SetLFO2,              "SetLFO2"               },
        { eTrkEventCodes::SetLFO2DelFade,       "SetLFO2DelFade"        },
        { eTrkEventCodes::RouteLFO2ToVol,       "RouteLFO2ToVol"        },
        { eTrkEventCodes::SetTrkPan,            "SetPan"                },
        { eTrkEventCodes::AddTrkPan,            "AddTrkPan"             },
        { eTrkEventCodes::SweepTrkPan,          "SweepTrkPan"           },
        { eTrkEventCodes::SetLFO3,              "SetLFO3"               },
        { eTrkEventCodes::SetLFO3DelFade,       "SetLFO3DelFade"        },
        { eTrkEventCodes::RouteLFO3ToPan,       "RouteLFO3ToPan"        },
        { eTrkEventCodes::SetLFO,               "SetLFO"                },
        { eTrkEventCodes::SetLFODelFade,        "SetLFODelFade"         },
        { eTrkEventCodes::SetLFOParam,          "SetLFOParam"           },
        { eTrkEventCodes::SetLFORoute,          "SetLFORoute"           },
        { eTrkEventCodes::Unk_0xF6,             "Unk_0xF6"              },
        { eTrkEventCodes::SkipNext2Bytes2,      "SkipNext2Bytes2"       },
     } };


    /***************************************************************************************
        TrkEventsTable
            Definition of the TrkEventsTable.
            
            Contains important details on how to parse all individual events.
    ***************************************************************************************/
    const std::vector<TrkEventInfo> TrkEventsTable
    {{
        //Play Note event
        {
            eTrkEventCodes::NoteOnBeg,  //Event Codes Range Beginning
            eTrkEventCodes::NoteOnEnd,  //Event Codes Range End
            1,                          //Nb Required Parameters
            "PlayNote",                 //Event label
        },

        //TrackDelays
        {
            eTrkEventCodes::Delay_HN,
            eTrkEventCodes::Delay_64N,
            0,
            "FixedPause",
        },

        //0x90 - RepeatLastPause
        { eTrkEventCodes::RepeatLastPause,  eTrkEventCodes::Invalid, 0, EvCodeToEvNames.at(eTrkEventCodes::RepeatLastPause) },

        //0x91 - AddToLastPause
        { eTrkEventCodes::AddToLastPause,   eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::AddToLastPause) },

        //0x92 - Pause8Bits
        { eTrkEventCodes::Pause8Bits,       eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::Pause8Bits) },

        //0x93 - Pause16Bits
        { eTrkEventCodes::Pause16Bits,      eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::Pause16Bits) },

        //0x94 - Pause24Bits
        { eTrkEventCodes::Pause24Bits,      eTrkEventCodes::Invalid, 3, EvCodeToEvNames.at(eTrkEventCodes::Pause24Bits) },

        //0x95 - PauseUntilNoteOff
        { eTrkEventCodes::PauseUntilRel,    eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::PauseUntilRel) },

        //0x98 - EndOfTrack
        { eTrkEventCodes::EndOfTrack,       eTrkEventCodes::Invalid, 0, EvCodeToEvNames.at(eTrkEventCodes::EndOfTrack) },

        //0x99 - LoopPointSet
        { eTrkEventCodes::LoopPointSet,     eTrkEventCodes::Invalid, 0, EvCodeToEvNames.at(eTrkEventCodes::LoopPointSet) },

        //0x9C - RepeatFrom
        { eTrkEventCodes::RepeatFrom,       eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::RepeatFrom) },

        //0x9D - RepeatSegment
        { eTrkEventCodes::RepeatSegment,    eTrkEventCodes::Invalid, 0, EvCodeToEvNames.at(eTrkEventCodes::RepeatSegment) },

        //0x9E - AfterRepeat
        { eTrkEventCodes::AfterRepeat,      eTrkEventCodes::Invalid, 0, EvCodeToEvNames.at(eTrkEventCodes::AfterRepeat) },

        //0xA0 - SetOctave
        { eTrkEventCodes::SetOctave,        eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetOctave) },

        //0xA1 - AddOctave
        { eTrkEventCodes::AddOctave,        eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::AddOctave) },

        //0xA4 - SetTempo
        { eTrkEventCodes::SetTempo,         eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetTempo) },

        //0xA5 - SetTempo2
        { eTrkEventCodes::SetTempo2,        eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetTempo2) },

        //0xA8 - SetBank
        { eTrkEventCodes::SetBank,          eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::SetBank) },

        //0xA9 - SetBankHighByte
        { eTrkEventCodes::SetBankHighByte,  eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetBankHighByte) },

        //0xAA - SetBankLowByte
        { eTrkEventCodes::SetBankLowByte,   eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetBankLowByte) },

        //0xAB - SkipNextByte
        { eTrkEventCodes::SkipNextByte,     eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SkipNextByte) },

        //0xAC - SetPreset
        { eTrkEventCodes::SetPreset,        eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetPreset) },

        //0xAF - FadeSongVolume 
        { eTrkEventCodes::FadeSongVolume,   eTrkEventCodes::Invalid, 3, EvCodeToEvNames.at(eTrkEventCodes::FadeSongVolume) },

        //0xB0 - DisableEnvelope
        { eTrkEventCodes::DisableEnvelope,  eTrkEventCodes::Invalid, 0, EvCodeToEvNames.at(eTrkEventCodes::DisableEnvelope) },

        //0xB1 - SetEnvAtkLvl
        { eTrkEventCodes::SetEnvAtkLvl,     eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetEnvAtkLvl) },

        //0xB2 - SetEnvAtkTime
        { eTrkEventCodes::SetEnvAtkTime,    eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetEnvAtkTime) },

        //0xB3 - SetEnvHold
        { eTrkEventCodes::SetEnvHold,       eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetEnvHold) },

        //0xB4 - SetEnvDecSus
        { eTrkEventCodes::SetEnvDecSus,     eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::SetEnvDecSus) },

        //0xB5 - SetEnvFade
        { eTrkEventCodes::SetEnvFade,       eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetEnvFade) },

        //0xB6 - SetEnvRelease
        { eTrkEventCodes::SetEnvRelease,    eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetEnvRelease) },

        //0xBC - SetNoteVol
        { eTrkEventCodes::SetNoteVol,       eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetNoteVol) },

        //0xBE - SetChanPan
        { eTrkEventCodes::SetChanPan,       eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetChanPan) },

        //Unk_0xBF
        { eTrkEventCodes::Unk_0xBF,         eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::Unk_0xBF) },

        //Unk_0xC0
        { eTrkEventCodes::Unk_0xC0,         eTrkEventCodes::Invalid, 0, EvCodeToEvNames.at(eTrkEventCodes::Unk_0xC0) },

        //0xC3 - SetChanVol
        { eTrkEventCodes::SetChanVol,       eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetChanVol) },

        //0xCB - SkipNext2Bytes1
        { eTrkEventCodes::SkipNext2Bytes1,  eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::SkipNext2Bytes1) },

        //0xD0 - SetFTune
        { eTrkEventCodes::SetFTune,         eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetFTune) },

        //0xD1 - AddFTune
        { eTrkEventCodes::AddFTune,         eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::AddFTune) },

        //0xD2 - SetCTune
        { eTrkEventCodes::SetCTune,         eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetCTune) },

        //0xD3 - AddCTune
        { eTrkEventCodes::AddCTune,         eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::AddCTune) },

        //0xD4 - SweepTune
        { eTrkEventCodes::SweepTune,        eTrkEventCodes::Invalid, 3, EvCodeToEvNames.at(eTrkEventCodes::SweepTune) },

        //0xD5 - SetRndNoteRng
        { eTrkEventCodes::SetRndNoteRng,    eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::SetRndNoteRng) },

        //0xD6 - SetDetuneRng
        { eTrkEventCodes::SetDetuneRng,     eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::SetDetuneRng) },

        //0xD7 - SetPitchBend
        { eTrkEventCodes::SetPitchBend,     eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::SetPitchBend) },

        //Unk_0xD8
        { eTrkEventCodes::Unk_0xD8,         eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::Unk_0xD8) },

        //0xDB - SetPitchBendRng
        { eTrkEventCodes::SetPitchBendRng,  eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetPitchBendRng) },

        //0xDC - SetLFO1
        { eTrkEventCodes::SetLFO1,          eTrkEventCodes::Invalid, 5, EvCodeToEvNames.at(eTrkEventCodes::SetLFO1) },

        //0xDD - SetLFO1DelayFade
        { eTrkEventCodes::SetLFO1DelayFade, eTrkEventCodes::Invalid, 4, EvCodeToEvNames.at(eTrkEventCodes::SetLFO1DelayFade) },

        //0xDF - RouteLFO1ToPitch
        { eTrkEventCodes::RouteLFO1ToPitch, eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::RouteLFO1ToPitch) },

        //0xE0 - SetTrkVol
        { eTrkEventCodes::SetTrkVol,        eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetTrkVol) },

        //0xE1 - AddTrkVol
        { eTrkEventCodes::AddTrkVol,        eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::AddTrkVol) },

        //0xE2 - SweepTrackVol
        { eTrkEventCodes::SweepTrackVol,    eTrkEventCodes::Invalid, 3, EvCodeToEvNames.at(eTrkEventCodes::SweepTrackVol) },

        //0xE3 - SetExpress
        { eTrkEventCodes::SetExpress,       eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetExpress) },

        //0xE4 - SetLFO2
        { eTrkEventCodes::SetLFO2,          eTrkEventCodes::Invalid, 5, EvCodeToEvNames.at(eTrkEventCodes::SetLFO2) },

        //0xE5 - SetLFO2DelFade
        { eTrkEventCodes::SetLFO2DelFade,   eTrkEventCodes::Invalid, 4, EvCodeToEvNames.at(eTrkEventCodes::SetLFO2DelFade) },

        //0xE7 - RouteLFO2ToVol
        { eTrkEventCodes::RouteLFO2ToVol,   eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::RouteLFO2ToVol) },

        //0xE8 - SetTrkPan
        { eTrkEventCodes::SetTrkPan,        eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::SetTrkPan) },

        //0xE9 - AddTrkPan
        { eTrkEventCodes::AddTrkPan,        eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::AddTrkPan) },

        //0xEA - SweepTrkPan
        { eTrkEventCodes::SweepTrkPan,      eTrkEventCodes::Invalid, 3, EvCodeToEvNames.at(eTrkEventCodes::SweepTrkPan) },

        //0xEC - SetLFO3
        { eTrkEventCodes::SetLFO3,          eTrkEventCodes::Invalid, 5, EvCodeToEvNames.at(eTrkEventCodes::SetLFO3) },

        //0xED - SetLFO3DelFade
        { eTrkEventCodes::SetLFO3DelFade,   eTrkEventCodes::Invalid, 4, EvCodeToEvNames.at(eTrkEventCodes::SetLFO3DelFade) },

        //0xEF - RouteLFO3ToPan
        { eTrkEventCodes::RouteLFO3ToPan,   eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::RouteLFO3ToPan) },

        //0xF0 - SetLFO
        { eTrkEventCodes::SetLFO,           eTrkEventCodes::Invalid, 5, EvCodeToEvNames.at(eTrkEventCodes::SetLFO) },

        //0xF1 - SetLFODelFade
        { eTrkEventCodes::SetLFODelFade,    eTrkEventCodes::Invalid, 4, EvCodeToEvNames.at(eTrkEventCodes::SetLFODelFade) },

        //0xF2 - SetLFOParam
        { eTrkEventCodes::SetLFOParam,      eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::SetLFOParam) },

        //0xF3 - SetLFORoute
        { eTrkEventCodes::SetLFORoute,      eTrkEventCodes::Invalid, 3, EvCodeToEvNames.at(eTrkEventCodes::SetLFORoute) },

        //Unk_0xF6
        { eTrkEventCodes::Unk_0xF6,         eTrkEventCodes::Invalid, 1, EvCodeToEvNames.at(eTrkEventCodes::Unk_0xF6) },

        //SkipNext2Bytes2
        { eTrkEventCodes::SkipNext2Bytes2,  eTrkEventCodes::Invalid, 2, EvCodeToEvNames.at(eTrkEventCodes::SkipNext2Bytes2) },

    }};

    const std::unordered_map<std::string, eTrkEventCodes> EvNameToEvCodes
    { {
        { EvCodeToEvNames.at(eTrkEventCodes::RepeatLastPause  ), eTrkEventCodes::RepeatLastPause     },
        { EvCodeToEvNames.at(eTrkEventCodes::AddToLastPause   ), eTrkEventCodes::AddToLastPause      },
        { EvCodeToEvNames.at(eTrkEventCodes::Pause8Bits       ), eTrkEventCodes::Pause8Bits          },
        { EvCodeToEvNames.at(eTrkEventCodes::Pause16Bits      ), eTrkEventCodes::Pause16Bits         },
        { EvCodeToEvNames.at(eTrkEventCodes::Pause24Bits      ), eTrkEventCodes::Pause24Bits         },
        { EvCodeToEvNames.at(eTrkEventCodes::PauseUntilRel    ), eTrkEventCodes::PauseUntilRel       },
        { EvCodeToEvNames.at(eTrkEventCodes::EndOfTrack       ), eTrkEventCodes::EndOfTrack          },
        { EvCodeToEvNames.at(eTrkEventCodes::LoopPointSet     ), eTrkEventCodes::LoopPointSet        },
        { EvCodeToEvNames.at(eTrkEventCodes::RepeatFrom       ), eTrkEventCodes::RepeatFrom          },
        { EvCodeToEvNames.at(eTrkEventCodes::RepeatSegment    ), eTrkEventCodes::RepeatSegment       },
        { EvCodeToEvNames.at(eTrkEventCodes::AfterRepeat      ), eTrkEventCodes::AfterRepeat         },
        { EvCodeToEvNames.at(eTrkEventCodes::SetOctave        ), eTrkEventCodes::SetOctave           },
        { EvCodeToEvNames.at(eTrkEventCodes::AddOctave        ), eTrkEventCodes::AddOctave           },
        { EvCodeToEvNames.at(eTrkEventCodes::SetTempo         ), eTrkEventCodes::SetTempo            },
        { EvCodeToEvNames.at(eTrkEventCodes::SetTempo2        ), eTrkEventCodes::SetTempo2           },
        { EvCodeToEvNames.at(eTrkEventCodes::SetBank          ), eTrkEventCodes::SetBank             },
        { EvCodeToEvNames.at(eTrkEventCodes::SetBankHighByte  ), eTrkEventCodes::SetBankHighByte     },
        { EvCodeToEvNames.at(eTrkEventCodes::SetBankLowByte   ), eTrkEventCodes::SetBankLowByte      },
        { EvCodeToEvNames.at(eTrkEventCodes::SkipNextByte     ), eTrkEventCodes::SkipNextByte        },
        { EvCodeToEvNames.at(eTrkEventCodes::SetPreset        ), eTrkEventCodes::SetPreset           },
        { EvCodeToEvNames.at(eTrkEventCodes::FadeSongVolume   ), eTrkEventCodes::FadeSongVolume      },
        { EvCodeToEvNames.at(eTrkEventCodes::DisableEnvelope  ), eTrkEventCodes::DisableEnvelope     },
        { EvCodeToEvNames.at(eTrkEventCodes::SetEnvAtkLvl     ), eTrkEventCodes::SetEnvAtkLvl        },
        { EvCodeToEvNames.at(eTrkEventCodes::SetEnvAtkTime    ), eTrkEventCodes::SetEnvAtkTime       },
        { EvCodeToEvNames.at(eTrkEventCodes::SetEnvHold       ), eTrkEventCodes::SetEnvHold          },
        { EvCodeToEvNames.at(eTrkEventCodes::SetEnvDecSus     ), eTrkEventCodes::SetEnvDecSus        },
        { EvCodeToEvNames.at(eTrkEventCodes::SetEnvFade       ), eTrkEventCodes::SetEnvFade          },
        { EvCodeToEvNames.at(eTrkEventCodes::SetEnvRelease    ), eTrkEventCodes::SetEnvRelease       },
        { EvCodeToEvNames.at(eTrkEventCodes::SetNoteVol       ), eTrkEventCodes::SetNoteVol          },
        { EvCodeToEvNames.at(eTrkEventCodes::SetChanPan       ), eTrkEventCodes::SetChanPan          },
        { EvCodeToEvNames.at(eTrkEventCodes::Unk_0xBF         ), eTrkEventCodes::Unk_0xBF            },
        { EvCodeToEvNames.at(eTrkEventCodes::Unk_0xC0         ), eTrkEventCodes::Unk_0xC0            },
        { EvCodeToEvNames.at(eTrkEventCodes::SetChanVol       ), eTrkEventCodes::SetChanVol          },
        { EvCodeToEvNames.at(eTrkEventCodes::SkipNext2Bytes1  ), eTrkEventCodes::SkipNext2Bytes1     },
        { EvCodeToEvNames.at(eTrkEventCodes::SetFTune         ), eTrkEventCodes::SetFTune            },
        { EvCodeToEvNames.at(eTrkEventCodes::AddFTune         ), eTrkEventCodes::AddFTune            },
        { EvCodeToEvNames.at(eTrkEventCodes::SetCTune         ), eTrkEventCodes::SetCTune            },
        { EvCodeToEvNames.at(eTrkEventCodes::AddCTune         ), eTrkEventCodes::AddCTune            },
        { EvCodeToEvNames.at(eTrkEventCodes::SweepTune        ), eTrkEventCodes::SweepTune           },
        { EvCodeToEvNames.at(eTrkEventCodes::SetRndNoteRng    ), eTrkEventCodes::SetRndNoteRng       },
        { EvCodeToEvNames.at(eTrkEventCodes::SetDetuneRng     ), eTrkEventCodes::SetDetuneRng        },
        { EvCodeToEvNames.at(eTrkEventCodes::SetPitchBend     ), eTrkEventCodes::SetPitchBend        },
        { EvCodeToEvNames.at(eTrkEventCodes::Unk_0xD8         ), eTrkEventCodes::Unk_0xD8            },
        { EvCodeToEvNames.at(eTrkEventCodes::SetPitchBendRng  ), eTrkEventCodes::SetPitchBendRng     },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFO1          ), eTrkEventCodes::SetLFO1             },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFO1DelayFade ), eTrkEventCodes::SetLFO1DelayFade    },
        { EvCodeToEvNames.at(eTrkEventCodes::RouteLFO1ToPitch ), eTrkEventCodes::RouteLFO1ToPitch    },
        { EvCodeToEvNames.at(eTrkEventCodes::SetTrkVol        ), eTrkEventCodes::SetTrkVol           },
        { EvCodeToEvNames.at(eTrkEventCodes::AddTrkVol        ), eTrkEventCodes::AddTrkVol           },
        { EvCodeToEvNames.at(eTrkEventCodes::SweepTrackVol    ), eTrkEventCodes::SweepTrackVol       },
        { EvCodeToEvNames.at(eTrkEventCodes::SetExpress       ), eTrkEventCodes::SetExpress          },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFO2          ), eTrkEventCodes::SetLFO2             },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFO2DelFade   ), eTrkEventCodes::SetLFO2DelFade      },
        { EvCodeToEvNames.at(eTrkEventCodes::RouteLFO2ToVol   ), eTrkEventCodes::RouteLFO2ToVol      },
        { EvCodeToEvNames.at(eTrkEventCodes::SetTrkPan        ), eTrkEventCodes::SetTrkPan           },
        { EvCodeToEvNames.at(eTrkEventCodes::AddTrkPan        ), eTrkEventCodes::AddTrkPan           },
        { EvCodeToEvNames.at(eTrkEventCodes::SweepTrkPan      ), eTrkEventCodes::SweepTrkPan         },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFO3          ), eTrkEventCodes::SetLFO3             },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFO3DelFade   ), eTrkEventCodes::SetLFO3DelFade      },
        { EvCodeToEvNames.at(eTrkEventCodes::RouteLFO3ToPan   ), eTrkEventCodes::RouteLFO3ToPan      },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFO           ), eTrkEventCodes::SetLFO              },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFODelFade    ), eTrkEventCodes::SetLFODelFade       },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFOParam      ), eTrkEventCodes::SetLFOParam         },
        { EvCodeToEvNames.at(eTrkEventCodes::SetLFORoute      ), eTrkEventCodes::SetLFORoute         },
        { EvCodeToEvNames.at(eTrkEventCodes::Unk_0xF6         ), eTrkEventCodes::Unk_0xF6            },
        { EvCodeToEvNames.at(eTrkEventCodes::SkipNext2Bytes2  ), eTrkEventCodes::SkipNext2Bytes2     },
    } };

    const std::array<std::string, static_cast<uint8_t>(eNote::nbNotes)> NoteNames
    {{
        "C",
        "C#",
        "D",
        "D#",
        "E",
        "F",
        "F#",
        "G",
        "G#",
        "A",
        "A#",
        "B",
     }};

    const std::array<eTrkDelays, NbTrkDelayValues> TrkDelayCodesTbl
    { {
        eTrkDelays::_half,
        eTrkDelays::_dotqtr,
        eTrkDelays::_two3rdsofahalf,
        eTrkDelays::_qtr,
        eTrkDelays::_dot8th,
        eTrkDelays::_two3rdsofqtr,
        eTrkDelays::_8th,
        eTrkDelays::_dot16th,
        eTrkDelays::_two3rdsof8th,
        eTrkDelays::_16th,
        eTrkDelays::_dot32nd,
        eTrkDelays::_two3rdsof16th,
        eTrkDelays::_32nd,
        eTrkDelays::_dot64th,
        eTrkDelays::_two3rdsof32th,
        eTrkDelays::_64th,
    } };

    const std::map<uint8_t, eTrkDelays> TicksToTrkDelayID
    { {
        { static_cast<uint8_t>(eTrkDelays::_half),            eTrkDelays::_half             },
        { static_cast<uint8_t>(eTrkDelays::_dotqtr),          eTrkDelays::_dotqtr           },
        { static_cast<uint8_t>(eTrkDelays::_two3rdsofahalf),  eTrkDelays::_two3rdsofahalf   },
        { static_cast<uint8_t>(eTrkDelays::_qtr),             eTrkDelays::_qtr              },
        { static_cast<uint8_t>(eTrkDelays::_dot8th),          eTrkDelays::_dot8th           },
        { static_cast<uint8_t>(eTrkDelays::_two3rdsofqtr),    eTrkDelays::_two3rdsofqtr     },
        { static_cast<uint8_t>(eTrkDelays::_8th),             eTrkDelays::_8th              },
        { static_cast<uint8_t>(eTrkDelays::_dot16th),         eTrkDelays::_dot16th          },
        { static_cast<uint8_t>(eTrkDelays::_two3rdsof8th),    eTrkDelays::_two3rdsof8th     },
        { static_cast<uint8_t>(eTrkDelays::_16th),            eTrkDelays::_16th             },
        { static_cast<uint8_t>(eTrkDelays::_dot32nd),         eTrkDelays::_dot32nd          },
        { static_cast<uint8_t>(eTrkDelays::_two3rdsof16th),   eTrkDelays::_two3rdsof16th    },
        { static_cast<uint8_t>(eTrkDelays::_32nd),            eTrkDelays::_32nd             },
        { static_cast<uint8_t>(eTrkDelays::_dot64th),         eTrkDelays::_dot64th          },
        { static_cast<uint8_t>(eTrkDelays::_two3rdsof32th),   eTrkDelays::_two3rdsof32th    },
        { static_cast<uint8_t>(eTrkDelays::_64th),            eTrkDelays::_64th             },
    } };

    const std::map<eTrkDelays, uint8_t> TrkDelayToEvID
    { {
        { eTrkDelays::_half,            0x80 },
        { eTrkDelays::_dotqtr,          0x81 },
        { eTrkDelays::_two3rdsofahalf,  0x82 },
        { eTrkDelays::_qtr,             0x83 },
        { eTrkDelays::_dot8th,          0x84 },
        { eTrkDelays::_two3rdsofqtr,    0x85 },
        { eTrkDelays::_8th,             0x86 },
        { eTrkDelays::_dot16th,         0x87 },
        { eTrkDelays::_two3rdsof8th,    0x88 },
        { eTrkDelays::_16th,            0x89 },
        { eTrkDelays::_dot32nd,         0x8A },
        { eTrkDelays::_two3rdsof16th,   0x8B },
        { eTrkDelays::_32nd,            0x8C },
        { eTrkDelays::_dot64th,         0x8D },
        { eTrkDelays::_two3rdsof32th,   0x8E },
        { eTrkDelays::_64th,            0x8F },
    } };

    const std::map<uint8_t, eTrkDelays> TrkDelayCodeVals
    { {
        { 0x80, eTrkDelays::_half           },
        { 0x81, eTrkDelays::_dotqtr         },
        { 0x82, eTrkDelays::_two3rdsofahalf },
        { 0x83, eTrkDelays::_qtr            },
        { 0x84, eTrkDelays::_dot8th         },
        { 0x85, eTrkDelays::_two3rdsofqtr   },
        { 0x86, eTrkDelays::_8th            },
        { 0x87, eTrkDelays::_dot16th        },
        { 0x88, eTrkDelays::_two3rdsof8th   },
        { 0x89, eTrkDelays::_16th           },
        { 0x8A, eTrkDelays::_dot32nd        },
        { 0x8B, eTrkDelays::_two3rdsof16th  },
        { 0x8C, eTrkDelays::_32nd           },
        { 0x8D, eTrkDelays::_dot64th        },
        { 0x8E, eTrkDelays::_two3rdsof32th  },
        { 0x8F, eTrkDelays::_64th           },
    } };

//====================================================================================================
// Utility
//====================================================================================================
    std::optional<TrkEventInfo> GetEventInfo( eTrkEventCodes ev )
    {
        for( const auto & entry : TrkEventsTable )
        {
            if( ( entry.evcodeend != eTrkEventCodes::Invalid     ) && 
                ( ev >= entry.evcodebeg && ev <= entry.evcodeend ) )
            {
                return make_optional(entry );
            }
            else if( entry.evcodebeg == ev )
            {
                return make_optional(entry);
            }
        }
        return nullopt;
    }

    eTrkEventCodes StringToEventCode(const std::string& str)
    {
        auto itfound = EvNameToEvCodes.find(str);
        if (itfound == EvNameToEvCodes.end())
            return eTrkEventCodes::Invalid;
        return itfound->second;
    }

    static const std::unordered_map<std::string, eNote> NoteNamesToNote
    { {
        { NoteNames[(size_t) eNote::C  ], eNote::C  },
        { NoteNames[(size_t) eNote::Cs ], eNote::Cs },
        { NoteNames[(size_t) eNote::D  ], eNote::D  },
        { NoteNames[(size_t) eNote::Ds ], eNote::Ds },
        { NoteNames[(size_t) eNote::E  ], eNote::E  },
        { NoteNames[(size_t) eNote::F  ], eNote::F  },
        { NoteNames[(size_t) eNote::Fs ], eNote::Fs },
        { NoteNames[(size_t) eNote::G  ], eNote::G  },
        { NoteNames[(size_t) eNote::Gs ], eNote::Gs },
        { NoteNames[(size_t) eNote::A  ], eNote::A  },
        { NoteNames[(size_t) eNote::As ], eNote::As },
        { NoteNames[(size_t) eNote::B  ], eNote::B  },
    } };

    eNote StringToNote(const std::string& str)
    {
        auto itfound = NoteNamesToNote.find(str);
        if (itfound == NoteNamesToNote.end())
            return eNote::Invalid;
        return itfound->second;
    }

    void ParsePlayNoteParam1( uint8_t noteparam1, int8_t& out_octdiff, uint8_t& out_param2len, uint8_t& out_key )
    {
        //1. Get param2's len
        out_param2len = ( ( noteparam1 & NoteEvParam1NbParamsMask ) >> 6 ) & 0x3; //(0011) just to be sure no sign bits slip through somehow

        //2. Get and apply the octave modifiere
        out_octdiff = ( ( (noteparam1 & NoteEvParam1PitchMask) >> 4 ) & 0x3 ) - NoteEvOctaveShiftRange;

        //3. Get the key parameter 0x0 to 0xB, sometimes 0xF for special purpose notes!
        out_key = (noteparam1 & 0xF);
    }

    ev_play_note ParsePlayNote(const TrkEvent& ev, uint8_t curoctave)
    {
        ev_play_note plev;
        ParsePlayNoteParam1(ev.params.front(), plev.octmod, plev.param2len, plev.parsedkey);

        //Special case for when the play note event is 0xF
        if (plev.parsedkey > static_cast<uint8_t>(eNote::nbNotes))
        {
            throw std::runtime_error("DSE Key ID 0x0F is unsupported!\n");
        }
        else
        {
            curoctave = static_cast<int8_t>(curoctave) + plev.octmod;                      //Apply octave modification
            plev.mnoteid = (curoctave * static_cast<uint8_t>(eNote::nbNotes)) + plev.parsedkey; //Calculate MIDI key!
        }

        //Parse the note hold duration bytes
        for (size_t cntby = 0; cntby < plev.param2len; ++cntby)
            plev.holdtime = (plev.holdtime << 8) | ev.params[cntby + 1];

        return plev;
    }

    std::string MidiNoteIdToText(midinote_t midinote )
    {
        stringstream sstr;
        uint16_t     key    = midinote % static_cast<uint8_t>(eNote::nbNotes);
        uint16_t     octave = midinote / static_cast<uint8_t>(eNote::nbNotes);
        sstr << NoteNames.at(key) << octave;
        return std::move(sstr.str());
    }

    std::optional<eTrkDelays>  FindClosestTrkDelayID(uint8_t delayticks)
    {
        for (size_t i = 0; i < TrkDelayCodesTbl.size(); ++i)
        {
            if ((uint8_t)TrkDelayCodesTbl[i] == delayticks)
                return std::make_optional(TrkDelayCodesTbl[i]);
            else if ((i + 1) < (TrkDelayCodesTbl.size() - 1))
            {
                //Check if the next value is smaller than the delay. If it is, we can't get a value any closer to "delayticks".
                if (delayticks > static_cast<uint8_t>(TrkDelayCodesTbl[i + 1]))
                {
                    //Compare this value and the next and see which one we're closest to
                    uint8_t diff = static_cast<uint8_t>(TrkDelayCodesTbl[i]) - static_cast<uint8_t>(TrkDelayCodesTbl[i + 1]);

                    if (delayticks < (diff / 2))
                        return std::make_optional(TrkDelayCodesTbl[i + 1]); //The closest value in this case is the next one
                    else
                        return std::make_optional(TrkDelayCodesTbl[i]); //The closest value in this case is the current one
                }
            }
        }

        //If everything fails, return a null opt
        std::clog << "FindClosestTrkDelayID(): No closer delay found for " << static_cast<uint16_t>(delayticks) << " ticks !!\n";
        return std::nullopt; //Couldn't find something below the longest pause!
    }

//====================================================================================================
//====================================================================================================

};