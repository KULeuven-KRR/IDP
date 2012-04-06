cmake_minimum_required(VERSION 2.8)
		
OPTION(USEINTERACTIVE "allow user-interaction" ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")		
if(USEINTERACTIVE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D USEINTERACTIVE")
endif()

MESSAGE(STATUS "${PROJECT_SOURCE_DIR}")

include_directories (
	"${PROJECT_SOURCE_DIR}/lib/loki/include"
	"${PROJECT_SOURCE_DIR}/lib/tinythread/source"
	"${PROJECT_SOURCE_DIR}/lib/minisatid/src/"
	"${PROJECT_SOURCE_DIR}/lib/linenoise/include"
	"${PROJECT_SOURCE_DIR}/lib/lua/src"
	"${PROJECT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${PROJECT_SOURCE_DIR}/lib/pluscallback1.7/"
	"${PROJECT_SOURCE_DIR}/lib/linenoise/"
	"${PROJECT_SOURCE_DIR}/lib/tinythread/"
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D GIDLVERSION=\\\"${idp_VERSION}\\\" ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D IDPINSTALLDIR=\\\"\"${CMAKE_INSTALL_PREFIX}/\\\"\" ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D INTERNALLIBARYLUA=\\\"share/std/idp_intern.lua\\\" ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D INTERNALLIBARYIDP=\\\"share/std/idp_intern.idp\\\" ") 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D CONFIGFILENAME=\\\"var/.idprc\\\" ")
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()


find_package(BISON)
find_package(FLEX)

BISON_TARGET(myparser parser/parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
FLEX_TARGET(myscanner parser/lex.ll  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
ADD_FLEX_BISON_DEPENDENCY(myscanner myparser)

add_library(idp
	vocabulary/vocabulary.cpp
	structure/StructureComponents.cpp 
	structure/NumericOperations.cpp
	structure/Structure.cpp
	structure/AbstractStructure.cpp
	structure/Universe.cpp
	structure/TableSize.cpp
	structure/DomainElementFactory.cpp
	theory/term.cpp
	theory/theory.cpp
	options.cpp options.hpp
	lua/luaconnection.cpp
	internalargument.cpp
	namespace/namespace.cpp
	parser/parser.yy
	parser/lex.ll
	parser/clconst.hpp
	insert.hpp insert.cpp
	errorhandling/error.cpp
	common.cpp
	external/interactive.cpp
	printers/print.cpp
	theory/ecnf.cpp
	inferences/symmetrybreaking/symmetry.cpp
	inferences/modelexpansion/LuaTraceMonitor.cpp
	monitors/luainteractiveprintmonitor.hpp
	groundtheories/GroundTheory.cpp
	groundtheories/GroundPolicy.cpp
	groundtheories/SolverPolicy.cpp
	groundtheories/AbstractGroundTheory.cpp
	inferences/grounding/grounders/LazyFormulaGrounders.cpp
	inferences/grounding/grounders/LazyRuleGrounder.cpp
	inferences/grounding/grounders/DefinitionGrounders.cpp
	inferences/grounding/grounders/FormulaGrounders.cpp
	inferences/grounding/grounders/SetGrounders.cpp
	inferences/grounding/grounders/TermGrounders.cpp
	inferences/grounding/grounders/OptimizationTermGrounders.cpp
	inferences/grounding/GroundTermTranslator.cpp
	inferences/grounding/GroundTranslator.cpp
	inferences/grounding/GrounderFactory.cpp
	inferences/grounding/grounders/Grounder.cpp
	inferences/modelexpansion/ModelExpansion.cpp
	inferences/definitionevaluation/CalculateDefinitions.cpp
	inferences/SolverConnection.cpp
	inferences/propagation/propagate.cpp
	inferences/propagation/PropagatorFactory.cpp
	inferences/propagation/GenerateBDDAccordingToBounds.cpp
	inferences/entailment/Entails.cpp
	inferences/querying/Query.cpp
	external/rungidl.cpp
	external/commands/allcommands.cpp
	generators/InstGenerator.cpp
	generators/TreeInstGenerator.cpp
	generators/GeneratorFactory.cpp
	generators/BDDBasedGeneratorFactory.cpp
	visitors/StructureVisitor.cpp
	visitors/TheoryVisitor.cpp
	visitors/TheoryMutatingVisitor.cpp
	theory/information/CollectOpensOfDefinitions.cpp
	theory/information/CheckContainment.cpp
	theory/information/CheckSorts.cpp
	theory/information/ApproxCheckTwoValued.cpp
	theory/information/DeriveTermBounds.cpp
	theory/transformations/PushNegations.cpp
	theory/transformations/Flatten.cpp
	theory/transformations/DeriveSorts.cpp
	theory/transformations/AddCompletion.cpp
	theory/transformations/AddFuncConstraints.cpp
	theory/transformations/GraphFuncsAndAggs.cpp
	theory/transformations/PushQuantifications.cpp
	theory/transformations/RemoveEquivalences.cpp
	theory/transformations/SplitComparisonChains.cpp
	theory/transformations/UnnestDomainTerms.cpp
	theory/transformations/UnnestTerms.cpp
	theory/transformations/UnnestThreeValuedTerms.cpp
	GlobalData.cpp
	fobdds/FoBdd.cpp
	fobdds/FoBddKernels.cpp
	fobdds/FoBddTerms.cpp
	fobdds/FoBddVariable.cpp
	fobdds/FoBddFactory.cpp
	fobdds/FoBddManager.cpp
	fobdds/FoBddUtils.cpp
	fobdds/FoBddVisitor.cpp
	fobdds/FoBddSetExpr.cpp
	utils/FileManagement.cpp
	utils/NumericLimits.cpp
	theory/TheoryUtils.cpp
	utils/StringUtils.cpp
	utils/CPUtils.cpp
	${BISON_myparser_OUTPUTS} ${FLEX_myscanner_OUTPUTS}
)

target_link_libraries(idp minisatid lua linenoise tinythread)

add_executable(idp-bin external/main.cpp)
if(BUILD_IDP_STATIC)
	set_target_properties(idp-bin PROPERTIES LINK_FLAGS "-static ")
	set_target_properties(idp PROPERTIES LINK_FLAGS "-static ")
endif()
target_link_libraries(idp-bin idp)
set_target_properties(idp-bin PROPERTIES OUTPUT_NAME idp)

set_target_properties(
	idp idp-bin PROPERTIES
	VERSION ${idp_VERSION}
)

# add the install targets
install (TARGETS idp-bin DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install (TARGETS idp DESTINATION lib)
install (FILES ${PROJECT_SOURCE_DIR}/scripts/runidp.sh ${PROJECT_SOURCE_DIR}/scripts/runidp.bat DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install (FILES ${PROJECT_SOURCE_DIR}/data/var/.idprc DESTINATION var)
install (FILES ${PROJECT_SOURCE_DIR}/data/share/std/mx.idp 
				${PROJECT_SOURCE_DIR}/data/share/std/idp_intern.idp 
				${PROJECT_SOURCE_DIR}/data/share/std/idp_intern.lua 
				${PROJECT_SOURCE_DIR}/data/share/std/table_utils.idp 
		DESTINATION share/std)

#?SET(CPACK_PACKAGE_EXECUTABLES "${CPACK_PACKAGE_EXECUTABLES} idp runidp.bat runidp.sh")
