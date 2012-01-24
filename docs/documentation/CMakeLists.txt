include(${PROJECT_SOURCE_DIR}/cmake/UseLATEX.cmake)

add_latex_document(idp-manual.tex DEFAULT_PDF)

install(FILES
	"${CMAKE_BINARY_DIR}/docs/documentation/idp-manual.pdf"
	DESTINATION share/minisatid)
