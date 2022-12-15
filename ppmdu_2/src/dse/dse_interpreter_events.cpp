#include <dse/dse_interpreter.hpp>
#include <utils/poco_wrapper.hpp>
#include <dse/dse_conversion.hpp>

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>



#ifndef AUDIOUTIL_VER
#define AUDIOUTIL_VER "Poochyena"
#endif

using namespace std;

namespace DSE
{
};