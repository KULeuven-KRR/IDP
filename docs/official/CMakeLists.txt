include(${PROJECT_SOURCE_DIR}/cmake/UseLATEX.cmake)

set(TEXFILES "")
file(GLOB_RECURSE TEXFILES "${PROJECT_SOURCE_DIR}/docs/official" "*.tex")

set(TEXFILENAMES "")
foreach(file ${TEXFILES})
	GET_FILENAME_COMPONENT(filename ${file} NAME)
	SET(TEXFILENAMES ${TEXFILENAMES} ${filename})
endforeach(file)

message(STATUS "Texfiles: ${TEXFILENAMES}")

add_latex_document(idp-manual.tex INPUTS ${TEXFILENAMES}  DEFAULT_PDF MANGLE_TARGET_NAMES)
add_latex_document(fo-idp.tex INPUTS ${TEXFILENAMES} DEFAULT_PDF MANGLE_TARGET_NAMES)

install(FILES
	${PROJECT_BINARY_DIR}/docs/official/idp-manual.pdf
	DESTINATION share/minisatid)
