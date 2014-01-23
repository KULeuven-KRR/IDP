#-------------------------------------------------------------------------------
#  Copyright 2010-2012 Katholieke Universiteit Leuven
# 
#  Use of this software is governed by the GNU LGPLv3.0 license
# 
#  Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
#  Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
#  K.U.Leuven, Departement Computerwetenschappen,
#  Celestijnenlaan 200A, B-3001 Leuven, Belgium
#-------------------------------------------------------------------------------
include(${PROJECT_SOURCE_DIR}/cmake/UseLATEX/UseLATEX.cmake)

set(TEXFILES "")
set(IMAGEFILES "")
set(DOCSURL "${PROJECT_SOURCE_DIR}/docs/official/")
file(GLOB_RECURSE TEXFILES ${DOCSURL} "*.tex" "*.sty" "*.cls")
file(GLOB IMAGEFILES ${DOCSURL} "*.png")
file(GLOB_RECURSE IDPFILES ${DOCSURL} "*.idp")
 
set(TEXFILESNAMES "")
foreach(file ${TEXFILES})
	STRING(REGEX REPLACE ${DOCSURL} "" filepart ${file})
	SET(TEXFILESNAMES ${TEXFILESNAMES} ${filepart})
endforeach(file)
set(IMAGEFILENAMES "")
foreach(file ${IMAGEFILES})
	STRING(REGEX REPLACE ${DOCSURL} "" filepart ${file})
	SET(IMAGEFILENAMES ${IMAGEFILENAMES} ${filepart})
endforeach(file)
set(IDPFILENAMES "")
foreach(file ${IDPFILES})
	STRING(REGEX REPLACE ${DOCSURL} "" filepart ${file})
	SET(IDPFILENAMES ${IDPFILENAMES} ${filepart})
endforeach(file)

add_latex_document(idp-manual.tex INPUTS ${TEXFILESNAMES} IMAGES ${IMAGEFILENAMES} DEFAULT_PDF MANGLE_TARGET_NAMES)
add_latex_document(fo-idp.tex INPUTS ${TEXFILESNAMES} IMAGES ${IMAGEFILENAMES} DEFAULT_PDF MANGLE_TARGET_NAMES)
add_latex_document(tutorial.tex INPUTS ${TEXFILESNAMES} ${IDPFILENAMES} IMAGES ${IMAGEFILENAMES} DEFAULT_PDF MANGLE_TARGET_NAMES)
 
install(FILES
 	${PROJECT_BINARY_DIR}/docs/official/idp-manual.pdf
	DESTINATION share/idp)
