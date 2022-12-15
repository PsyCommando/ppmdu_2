add_subdirectory("jdksmidi")
include_directories("whereami/src")

add_library(whereami STATIC 
	"whereami/src/whereami.h" 
	"whereami/src/whereami.c"
)

add_library(easybmp STATIC 
	"EasyBMP/EasyBMP/EasyBMP.cpp"
	"EasyBMP/EasyBMP/EasyBMP.h"
	"EasyBMP/EasyBMP/EasyBMP_BMP.h"
	"EasyBMP/EasyBMP/EasyBMP_DataStructures.h"
	"EasyBMP/EasyBMP/EasyBMP_VariousBMPutilities.h"
)