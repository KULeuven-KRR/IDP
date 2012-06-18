include(${PROJECT_SOURCE_DIR}/cmake/UseLATEX.cmake)

add_latex_document(idp-manual.tex INPUTS "idp-latex.tex" "installing_and_running.tex" "comments.tex" "includes.tex" "namespaces.tex" "vocabularies.tex" "theories.tex" "terms.tex" "queries.tex" "structures.tex" "procedures.tex" "options.tex" "idp2.tex" "errors.tex"  DEFAULT_PDF MANGLE_TARGET_NAMES)
add_latex_document(fo-idp.tex INPUTS "idp-latex.tex" DEFAULT_PDF MANGLE_TARGET_NAMES)


install(FILES
	${PROJECT_BINARY_DIR}/docs/official/idp-manual.pdf
	DESTINATION share/minisatid)
