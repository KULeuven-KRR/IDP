include(${PROJECT_SOURCE_DIR}/cmake/UseLATEX.cmake)

set(TEXFILES "")
set(IMAGEFILES "")
file(GLOB_RECURSE TEXFILES "${PROJECT_SOURCE_DIR}/docs/official" "*.tex")
file(GLOB_RECURSE IMAGEFILES "${PROJECT_SOURCE_DIR}/docs/official" "*.png")

set(TEXFILENAMES "")
foreach(file ${TEXFILES})
	GET_FILENAME_COMPONENT(filename ${file} NAME)
	SET(TEXFILENAMES ${TEXFILENAMES} ${filename})
endforeach(file)
set(IMAGEFILENAMES "")
foreach(file ${IMAGEFILES})
	GET_FILENAME_COMPONENT(filename ${file} NAME)
	SET(IMAGEFILENAMES ${IMAGEFILENAMES} ${filename})
endforeach(file)

add_latex_document(idp-manual.tex INPUTS ${TEXFILENAMES} IMAGES ${IMAGEFILENAMES} DEFAULT_PDF MANGLE_TARGET_NAMES)
add_latex_document(fo-idp.tex INPUTS ${TEXFILENAMES} IMAGES ${IMAGEFILENAMES} DEFAULT_PDF MANGLE_TARGET_NAMES)

install(FILES
	${PROJECT_BINARY_DIR}/docs/official/idp-manual.pdf
	DESTINATION share/idp)
