cmake_minimum_required(VERSION 2.8)
		
OPTION(USEINTERACTIVE "allow user-interaction" ON)
		
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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D GIDLVERSION=\\\"${idp_VERSION_MAJOR}-${idp_VERSION_MINOR}\\\" ")
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
	commontypes.hpp
	parseinfo.hpp parseinfo.cpp
	vocabulary.hpp vocabulary.cpp
	structure.hpp structure.cpp
	term.hpp term.cpp
	theory.hpp theory.cpp
	options.cpp options.hpp
	luaconnection.hpp luaconnection.cpp
	internalargument.hpp internalargument.cpp
	namespace.hpp namespace.cpp
	parser/parser.yy
	parser/yyltype.hpp
	parser/lex.ll
	parser/clconst.hpp
	GeneralUtils.hpp
	insert.hpp insert.cpp
	error.hpp error.cpp
	common.hpp common.cpp
	interactive.hpp interactive.cpp
	printers/print.hpp printers/print.cpp
	printers/idpprinter.hpp printers/ecnfprinter.hpp printers/tptpprinter.hpp
	ecnf.hpp ecnf.cpp
	symmetry.hpp symmetry.cpp
	monitors/interactiveprintmonitor.hpp
	inferences/modelexpansion/tracemonitor.hpp
	inferences/modelexpansion/propagatemonitor.hpp
	inferences/modelexpansion/LuaTraceMonitor.hpp inferences/modelexpansion/LuaTraceMonitor.cpp
	monitors/luainteractiveprintmonitor.hpp
	groundtheories/PrintGroundPolicy.hpp
	groundtheories/GroundPolicy.hpp groundtheories/GroundPolicy.cpp
	groundtheories/SolverPolicy.hpp groundtheories/SolverPolicy.cpp
	groundtheories/AbstractGroundTheory.hpp groundtheories/AbstractGroundTheory.cpp
	groundtheories/GroundTheory.hpp
	inferences/grounding/grounders/LazyQuantGrounder.hpp inferences/grounding/grounders/LazyQuantGrounder.cpp
	inferences/grounding/grounders/DefinitionGrounders.hpp inferences/grounding/grounders/DefinitionGrounders.cpp
	inferences/grounding/grounders/FormulaGrounders.hpp inferences/grounding/grounders/FormulaGrounders.cpp
	inferences/grounding/grounders/SetGrounders.hpp inferences/grounding/grounders/SetGrounders.cpp
	inferences/grounding/grounders/TermGrounders.hpp inferences/grounding/grounders/TermGrounders.cpp
	inferences/grounding/grounders/GroundUtils.hpp
	inferences/grounding/GroundingContext.hpp
	inferences/grounding/GroundTermTranslator.hpp inferences/grounding/GroundTermTranslator.cpp
	inferences/grounding/GroundTranslator.hpp inferences/grounding/GroundTranslator.cpp
	inferences/grounding/Utils.hpp
	inferences/grounding/GrounderFactory.hpp inferences/grounding/GrounderFactory.cpp
	inferences/grounding/grounders/Grounder.hpp inferences/grounding/grounders/Grounder.cpp
	inferences/modelexpansion/ModelExpansion.hpp inferences/modelexpansion/ModelExpansion.cpp
	inferences/CalculateDefinitions.hpp inferences/CalculateDefinitions.cpp
	inferences/InferenceSolverConnection.hpp inferences/InferenceSolverConnection.cpp
	inferences/propagation/SymbolicPropagation.hpp
	inferences/propagation/OptimalPropagation.hpp
	inferences/propagation/GroundingPropagation.hpp
	inferences/propagation/propagate.hpp inferences/propagation/propagate.cpp
	inferences/propagation/PropagatorFactory.hpp inferences/propagation/PropagatorFactory.cpp
	inferences/propagation/GenerateBDDAccordingToBounds.hpp inferences/propagation/GenerateBDDAccordingToBounds.cpp
	inferences/Entails.hpp inferences/Entails.cpp
	inferences/Query.hpp inferences/Query.cpp
	rungidl.hpp rungidl.cpp
	commands/allcommands.hpp commands/allcommands.cpp
	commands/commandinterface.hpp
	commands/printblocks.hpp
	commands/printdomainatom.hpp
	commands/printoptions.hpp
	commands/newstructure.hpp
	commands/modelexpand.hpp
	commands/newoptions.hpp
	commands/clone.hpp
	commands/transformations.hpp
	commands/idptype.hpp
	commands/setatomvalue.hpp
	commands/settablevalue.hpp
	commands/iterators.hpp
	commands/propagate.hpp
	commands/query.hpp
	commands/structureproperties.hpp
	commands/createdummytuple.hpp
	commands/ground.hpp
	commands/clean.hpp
	commands/help.hpp
	commands/setvocabulary.hpp
	commands/entails.hpp
	commands/simplify.hpp
	commands/createrange.hpp
	commands/tablesize.hpp
	commands/twovaluedextensions.hpp
	generators/ArithmeticOperatorsGenerator.hpp
	generators/ComparisonGenerator.hpp
	generators/BasicGenerators.hpp
	generators/EnumLookupGenerator.hpp
	generators/FalseQuantKernelGenerator.hpp
	generators/InstGenerator.hpp generators/InstGenerator.cpp
	generators/InverseAbsValueGenerator.hpp
	generators/InverseUnaFunctionGenerator.hpp
	generators/InvertNumericGenerator.hpp
	generators/LookupGenerator.hpp
	generators/SimpleFuncGenerator.hpp
	generators/SortInstGenerator.hpp
	generators/SortLookupGenerator.hpp
	generators/LookupGenerator.hpp
	generators/InverseInstGenerator.hpp
	generators/TableGenerator.hpp
	generators/TreeInstGenerator.hpp
	generators/BasicCheckers.hpp
	generators/GeneratorNodes.hpp
	generators/TrueQuantKernelGenerator.hpp
	generators/GeneratorFactory.hpp generators/GeneratorFactory.cpp
	generators/BDDBasedGeneratorFactory.hpp generators/BDDBasedGeneratorFactory.cpp
	visitors/StructureVisitor.hpp visitors/StructureVisitor.cpp
	visitors/TheoryVisitor.hpp visitors/TheoryVisitor.cpp
	visitors/TheoryMutatingVisitor.hpp visitors/TheoryMutatingVisitor.cpp
	structureanalysis/EnumerateSymbolicTable.hpp
	theoryinformation/CollectOpensOfDefinitions.hpp theoryinformation/CollectOpensOfDefinitions.cpp
	theoryinformation/CheckContainment.hpp theoryinformation/CheckContainment.cpp
	theoryinformation/CheckContainsFuncTerms.hpp
	theoryinformation/CheckSorts.hpp theoryinformation/CheckSorts.cpp
	theoryinformation/CountNbOfSubFormulas.hpp
	theoryinformation/CheckFuncTerms.hpp
	theoryinformation/CheckPartialTerm.hpp
	theoryinformation/ApproxCheckTwoValued.hpp theoryinformation/ApproxCheckTwoValued.cpp
	theoryinformation/DeriveTermBounds.hpp theoryinformation/DeriveTermBounds.cpp
	theorytransformations/PushNegations.hpp theorytransformations/PushNegations.cpp
	theorytransformations/Flatten.hpp theorytransformations/Flatten.cpp
	theorytransformations/DeriveSorts.hpp theorytransformations/DeriveSorts.cpp
	theorytransformations/AddCompletion.hpp theorytransformations/AddCompletion.cpp
	theorytransformations/GraphFuncsAndAggs.hpp theorytransformations/GraphFuncsAndAggs.cpp
	theorytransformations/PushQuantifications.hpp theorytransformations/PushQuantifications.cpp
	theorytransformations/RemoveEquivalences.hpp theorytransformations/RemoveEquivalences.cpp
	theorytransformations/SplitComparisonChains.hpp theorytransformations/SplitComparisonChains.cpp
	theorytransformations/SubstituteTerm.hpp
	theorytransformations/SplitIntoMonotoneAgg.hpp
	theorytransformations/UnnestFuncsAndAggs.hpp theorytransformations/UnnestFuncsAndAggs.cpp
	theorytransformations/UnnestPartialTerms.hpp
	theorytransformations/UnnestTerms.hpp theorytransformations/UnnestTerms.cpp
	theorytransformations/UnnestThreeValuedTerms.hpp theorytransformations/UnnestThreeValuedTerms.cpp
	GlobalData.hpp GlobalData.cpp
	fobdds/FoBdd.hpp fobdds/FoBdd.cpp
	fobdds/FoBddKernel.hpp
	fobdds/FoBddKernels.cpp
	fobdds/FoBddAtomKernel.hpp
	fobdds/FoBddQuantKernel.hpp
	fobdds/FoBddTerm.hpp
	fobdds/FoBddTerms.cpp
	fobdds/FoBddDomainTerm.hpp
	fobdds/FoBddVariable.hpp fobdds/FoBddVariable.cpp
	fobdds/FoBddIndex.hpp
	fobdds/FoBddFuncTerm.hpp
	fobdds/FoBddFactory.hpp fobdds/FoBddFactory.cpp
	fobdds/FoBddManager.hpp fobdds/FoBddManager.cpp
	fobdds/FoBddUtils.hpp fobdds/FoBddUtils.cpp
	fobdds/FoBddVisitor.hpp fobdds/FoBddVisitor.cpp
	fobdds/bddvisitors/OrderTerms.hpp
	fobdds/bddvisitors/TermOccursNested.hpp
	fobdds/bddvisitors/ContainsPartialFunctions.hpp
	fobdds/bddvisitors/FirstNonConstMultTerm.hpp
	fobdds/bddvisitors/TermCollector.hpp
	fobdds/bddvisitors/IndexCollector.hpp
	fobdds/bddvisitors/VariableCollector.hpp
	fobdds/bddvisitors/AddMultSimplifier.hpp
	fobdds/bddvisitors/SubstituteTerms.hpp
	fobdds/bddvisitors/ContainsFuncTerms.hpp
	fobdds/bddvisitors/BumpIndices.hpp
	fobdds/bddvisitors/TermsToLeft.hpp
	fobdds/bddvisitors/CheckIsArithmeticFormula.hpp
	fobdds/bddvisitors/Copy.hpp
	fobdds/bddvisitors/BddToFormula.hpp
	fobdds/bddvisitors/RemoveMinus.hpp
	fobdds/bddvisitors/UngraphFunctions.hpp
	fobdds/bddvisitors/ContainsTerm.hpp
	fobdds/bddvisitors/CombineConstsOfMults.hpp
	fobdds/EstimateBDDInferenceCost.hpp
	fobdds/bddvisitors/ApplyDistributivity.hpp
	fobdds/bddvisitors/FirstConstTermInFunc.hpp
	fobdds/bddvisitors/CollectSameOperationTerms.hpp
	IdpException.hpp
	utils/FileManagement.hpp utils/FileManagement.cpp
	utils/TheoryUtils.hpp utils/TheoryUtils.cpp
	utils/StringUtils.hpp utils/StringUtils.cpp
	${BISON_myparser_OUTPUTS} ${FLEX_myscanner_OUTPUTS}
)

target_link_libraries(idp minisatid lua linenoise tinythread)

add_executable(idp-bin main.cpp)
if(BUILD_IDP_STATIC)
	set_target_properties(idp-bin PROPERTIES LINK_FLAGS "-static ")
	set_target_properties(idp PROPERTIES LINK_FLAGS "-static ")
endif()
target_link_libraries(idp-bin idp)
set_target_properties(idp-bin PROPERTIES OUTPUT_NAME idp)

# add the install targets
install (TARGETS idp-bin DESTINATION bin)
install (TARGETS idp DESTINATION lib)
install (FILES ${PROJECT_SOURCE_DIR}/scripts/runidp.sh DESTINATION bin PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install (FILES ${PROJECT_SOURCE_DIR}/data/var/.idprc DESTINATION var)
install (FILES ${PROJECT_SOURCE_DIR}/data/share/std/mx.idp 
				${PROJECT_SOURCE_DIR}/data/share/std/idp_intern.idp 
				${PROJECT_SOURCE_DIR}/data/share/std/idp_intern.lua 
		DESTINATION share/std)
