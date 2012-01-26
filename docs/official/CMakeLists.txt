include(${PROJECT_SOURCE_DIR}/cmake/UseLATEX.cmake)

add_latex_document(idp-manual.tex INPUTS "idp-latex.tex" DEFAULT_PDF MANGLE_TARGET_NAMES)
add_latex_document(fo-idp.tex INPUTS "idp-latex.tex" DEFAULT_PDF MANGLE_TARGET_NAMES)

install(FILES
	idp-manual.pdf
	DESTINATION share/minisatid)
