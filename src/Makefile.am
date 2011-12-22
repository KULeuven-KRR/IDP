####################################################################
#Copyright 2010-2012 Katholieke Universiteit Leuven
# 
#Use of this software is governed by the GNU LGPLv3.0 license
#
#Written by Broes De Cat, Stef De Pooter, Johan Wittocx
#and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
#Celestijnenlaan 200A, B-3001 Leuven, Belgium
####################################################################

SUBDIRS = parser

libsrcdir = $(top_srcdir)/lib
libbuilddir = $(top_builddir)/lib

AM_CXXFLAGS = @AC_CXXFLAGS@ -Wall -Wextra\
		-std=c++0x\
		-I$(libsrcdir)/loki/include\
		-I$(libsrcdir)/tinythread/source\
		-I $(libsrcdir)/minisatid/src/\
		-I$(libsrcdir)/linenoise/include\
		-I$(libsrcdir)/lua/etc -I$(libsrcdir)/lua/src\
		-I$(libbuilddir)/lua/src\
		-I $(libsrcdir)/pluscallback1.7/\
		-I $(libsrcdir)/linenoise/\
		-DIDPDATADIR=\"$(datadir)\" -DRCDIR=\"$(localstatedir)\"\
		-DINTERNALLIBARYLUA=\"/std/idp_intern.lua\"\
		-DINTERNALLIBARYIDP=\"/std/idp_intern.idp\" -DCONFIGFILENAME=\"/idp/.idprc\"\
		-DGIDLVERSION=\"$(GIDL_VERSION)\"
		
if USEINTERACTIVE
AM_CXXFLAGS += -DUSEINTERACTIVE
endif

AM_LDFLAGS = @AC_LDFLAGS@

AM_YFLAGS = -d -v

lib_LTLIBRARIES = libgidl-2.0.la
libgidl_2_0_la_LDFLAGS = -version-info 1:0:0
gidl_includedir = $(includedir)/gidl-2.0/

#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = gidl-2.0.pc
#EXTRA_DIST = gidl-2.0.pc.in

# do not forget ldconfig after installing ;)
libgidl_2_0_la_LIBADD = \
		$(libbuilddir)/lua/src/.libs/liblua.la\
		$(libbuilddir)/minisatid/src/libminisatid-2.0.la\
		$(libbuilddir)/linenoise/liblinenoise-1.0.la\
		$(libbuilddir)/tinythread/source/libtinythread-1.0.la

BUILT_SOURCES = parser/parser.h

libgidl_2_0_la_SOURCES = \
		commontypes.hpp\
		parseinfo.hpp parseinfo.cpp\
		vocabulary.hpp vocabulary.cpp\
		structure.hpp structure.cpp\
		term.hpp term.cpp\
		theory.hpp theory.cpp\
		options.cpp options.hpp\
		luaconnection.hpp luaconnection.cpp\
		internalargument.hpp internalargument.cpp\
		namespace.hpp namespace.cpp\
		parser/parser.yy\
		parser/yyltype.hpp\
		parser/lex.ll\
		parser/clconst.hpp\
		GeneralUtils.hpp\
		insert.hpp insert.cpp\
		error.hpp error.cpp\
		common.hpp common.cpp\
		interactive.hpp interactive.cpp\
		printers/print.hpp printers/print.cpp\
		printers/idpprinter.hpp printers/ecnfprinter.hpp printers/tptpprinter.hpp\
		ecnf.hpp ecnf.cpp\
        symmetry.hpp symmetry.cpp\
		monitors/interactiveprintmonitor.hpp\
		inferences/modelexpansion/tracemonitor.hpp\
		inferences/modelexpansion/propagatemonitor.hpp\
		inferences/modelexpansion/LuaTraceMonitor.hpp inferences/modelexpansion/LuaTraceMonitor.cpp\
		monitors/luainteractiveprintmonitor.hpp\
		groundtheories/PrintGroundPolicy.hpp\
		groundtheories/GroundPolicy.hpp groundtheories/GroundPolicy.cpp\
		groundtheories/SolverPolicy.hpp groundtheories/SolverPolicy.cpp\
		groundtheories/AbstractGroundTheory.hpp groundtheories/AbstractGroundTheory.cpp\
		groundtheories/GroundTheory.hpp\
		inferences/grounding/grounders/LazyQuantGrounder.hpp inferences/grounding/grounders/LazyQuantGrounder.cpp\
		inferences/grounding/grounders/DefinitionGrounders.hpp inferences/grounding/grounders/DefinitionGrounders.cpp\
		inferences/grounding/grounders/FormulaGrounders.hpp inferences/grounding/grounders/FormulaGrounders.cpp\
		inferences/grounding/grounders/SetGrounders.hpp inferences/grounding/grounders/SetGrounders.cpp\
		inferences/grounding/grounders/TermGrounders.hpp inferences/grounding/grounders/TermGrounders.cpp\
		inferences/grounding/GroundUtils.hpp\
		inferences/grounding/GroundingContext.hpp\
		inferences/grounding/GroundTermTranslator.hpp inferences/grounding/GroundTermTranslator.cpp\
		inferences/grounding/GroundTranslator.hpp inferences/grounding/GroundTranslator.cpp\
		inferences/grounding/Utils.hpp\
		inferences/grounding/GrounderFactory.hpp inferences/grounding/GrounderFactory.cpp\
		inferences/grounding/grounders/Grounder.hpp inferences/grounding/grounders/Grounder.cpp\
		inferences/modelexpansion/ModelExpansion.hpp inferences/modelexpansion/ModelExpansion.cpp\
		inferences/CalculateDefinitions.hpp inferences/CalculateDefinitions.cpp\
		inferences/InferenceSolverConnection.hpp inferences/InferenceSolverConnection.cpp\
		inferences/propagation/SymbolicPropagation.hpp\
		inferences/propagation/OptimalPropagation.hpp\
		inferences/propagation/GroundingPropagation.hpp\
		inferences/propagation/propagate.hpp inferences/propagation/propagate.cpp\
		inferences/propagation/PropagatorFactory.hpp inferences/propagation/PropagatorFactory.cpp\
		inferences/propagation/GenerateBDDAccordingToBounds.hpp inferences/propagation/GenerateBDDAccordingToBounds.cpp\
		inferences/Entails.hpp inferences/Entails.cpp\
		inferences/Query.hpp inferences/Query.cpp\
		rungidl.hpp rungidl.cpp\
		commands/allcommands.hpp commands/allcommands.cpp\
		commands/commandinterface.hpp\
		commands/optimization.hpp\
		commands/printblocks.hpp\
		commands/printdomainatom.hpp\
		commands/printoptions.hpp\
		commands/newstructure.hpp\
		commands/newtheory.hpp\
		commands/modelexpand.hpp\
		commands/newoptions.hpp\
		commands/clonestructure.hpp\
		commands/clonetheory.hpp\
		commands/pushnegations.hpp\
		commands/mergetheories.hpp\
		commands/flatten.hpp\
		commands/idptype.hpp\
		commands/setatomvalue.hpp\
		commands/settablevalue.hpp\
		commands/domainiterator.hpp\
		commands/tableiterator.hpp\
		commands/propagate.hpp\
		commands/query.hpp\
		commands/structureproperties.hpp\
		commands/createtuple.hpp\
		commands/ground.hpp\
		commands/completion.hpp\
		commands/tobdd.hpp\
		commands/clean.hpp\
		commands/help.hpp\
		commands/setvocabulary.hpp\
		commands/estimatecosts.hpp\
		commands/detectsymmetry.hpp\
		commands/derefandincrement.hpp\
		commands/entails.hpp\
		commands/removenesting.hpp\
		commands/simplify.hpp\
		commands/createrange.hpp\
		commands/tablesize.hpp\
		commands/twovaluedextensions.hpp\
		commands/materialize.hpp\
		generators/ArithmeticOperatorsGenerator.hpp\
		generators/ComparisonGenerator.hpp\
		generators/BasicGenerators.hpp\
		generators/EnumLookupGenerator.hpp\
		generators/FalseQuantKernelGenerator.hpp\
		generators/InstGenerator.hpp generators/InstGenerator.cpp\
		generators/InverseAbsValueGenerator.hpp\
		generators/InverseUnaFunctionGenerator.hpp\
		generators/InvertNumericGenerator.hpp\
		generators/LookupGenerator.hpp\
		generators/SimpleFuncGenerator.hpp\
		generators/SortInstGenerator.hpp\
		generators/SortLookupGenerator.hpp\
		generators/LookupGenerator.hpp\
		generators/InverseInstGenerator.hpp\
		generators/TableGenerator.hpp\
		generators/TreeInstGenerator.hpp\
		generators/BasicCheckers.hpp\
		generators/GeneratorNodes.hpp\
		generators/TrueQuantKernelGenerator.hpp\
		generators/GeneratorFactory.hpp generators/GeneratorFactory.cpp\
		generators/BDDBasedGeneratorFactory.hpp generators/BDDBasedGeneratorFactory.cpp\
		visitors/StructureVisitor.hpp visitors/StructureVisitor.cpp\
		visitors/TheoryVisitor.hpp visitors/TheoryVisitor.cpp\
		visitors/TheoryMutatingVisitor.hpp visitors/TheoryMutatingVisitor.cpp\
        structureanalysis/EnumerateSymbolicTable.hpp\
		theoryinformation/CollectOpensOfDefinitions.hpp theoryinformation/CollectOpensOfDefinitions.cpp\
		theoryinformation/CheckContainment.hpp theoryinformation/CheckContainment.cpp\
		theoryinformation/CheckContainsFuncTerms.hpp\
		theoryinformation/CheckSorts.hpp theoryinformation/CheckSorts.cpp\
		theoryinformation/CountNbOfSubformulas.hpp\
		theoryinformation/CheckFuncTerms.hpp\
		theoryinformation/CheckPartialTerm.hpp\
		theoryinformation/ApproxCheckTwoValued.hpp theoryinformation/ApproxCheckTwoValued.cpp\
		theoryinformation/DeriveTermBounds.hpp theoryinformation/DeriveTermBounds.cpp\
		theorytransformations/PushNegations.hpp theorytransformations/PushNegations.cpp\
		theorytransformations/Flatten.hpp theorytransformations/Flatten.cpp\
		theorytransformations/DeriveSorts.hpp theorytransformations/DeriveSorts.cpp\
		theorytransformations/AddCompletion.hpp theorytransformations/AddCompletion.cpp\
		theorytransformations/GraphFuncsAndAggs.hpp theorytransformations/GraphFuncsAndAggs.cpp\
		theorytransformations/SplitProducts.hpp theorytransformations/SplitProducts.cpp\
		theorytransformations/PushQuantifications.hpp theorytransformations/PushQuantifications.cpp\
		theorytransformations/RemoveEquivalences.hpp theorytransformations/RemoveEquivalences.cpp\
		theorytransformations/SplitComparisonChains.hpp theorytransformations/SplitComparisonChains.cpp\
		theorytransformations/SubstituteTerm.hpp\
		theorytransformations/SplitIntoMonotoneAgg.hpp\
		theorytransformations/UnnestFuncsAndAggs.hpp theorytransformations/UnnestFuncsAndAggs.cpp\
		theorytransformations/UnnestPartialTerms.hpp\
		theorytransformations/UnnestTerms.hpp theorytransformations/UnnestTerms.cpp\
		theorytransformations/UnnestThreeValuedTerms.hpp theorytransformations/UnnestThreeValuedTerms.cpp\
		GlobalData.hpp GlobalData.cpp\
		fobdds/FoBdd.hpp fobdds/FoBdd.cpp\
		fobdds/FoBddKernel.hpp\
		fobdds/FoBddKernels.cpp\
		fobdds/FoBddAtomKernel.hpp\
		fobdds/FoBddQuantKernel.hpp\
		fobdds/FoBddTerm.hpp\
		fobdds/FoBddTerms.cpp\
		fobdds/FoBddDomainTerm.hpp\
		fobdds/FoBddVariable.hpp fobdds/FoBddVariable.cpp\
		fobdds/FoBddIndex.hpp\
		fobdds/FoBddFuncTerm.hpp\
		fobdds/FoBddFactory.hpp fobdds/FoBddFactory.cpp\
		fobdds/FoBddManager.hpp fobdds/FoBddManager.cpp\
		fobdds/FoBddUtils.hpp fobdds/FoBddUtils.cpp\
		fobdds/FoBddVisitor.hpp fobdds/FoBddVisitor.cpp\
		fobdds/bddvisitors/OrderTerms.hpp\
		fobdds/bddvisitors/TermOccursNested.hpp\
		fobdds/bddvisitors/ContainsPartialFunctions.hpp\
		fobdds/bddvisitors/FirstNonConstMultTerm.hpp\
		fobdds/bddvisitors/TermCollector.hpp\
		fobdds/bddvisitors/IndexCollector.hpp\
		fobdds/bddvisitors/VariableCollector.hpp\
		fobdds/bddvisitors/AddMultSimplifier.hpp\
		fobdds/bddvisitors/SubstituteTerms.hpp\
		fobdds/bddvisitors/ContainsFuncTerms.hpp\
		fobdds/bddvisitors/BumpIndices.hpp\
		fobdds/bddvisitors/TermsToLeft.hpp\
		fobdds/bddvisitors/CheckIsArithmeticFormula.hpp\
		fobdds/bddvisitors/Copy.hpp\
		fobdds/bddvisitors/BddToFormula.hpp\
		fobdds/bddvisitors/RemoveMinus.hpp\
		fobdds/bddvisitors/UngraphFunctions.hpp\
		fobdds/bddvisitors/ContainsTerm.hpp\
		fobdds/bddvisitors/CombineConstsOfMults.hpp\
		fobdds/EstimateBDDInferenceCost.hpp\
		fobdds/bddvisitors/ApplyDistributivity.hpp\
		fobdds/bddvisitors/FirstConstTermInFunc.hpp\
		fobdds/bddvisitors/CollectSameOperationTerms.hpp\
		IdpException.hpp\
		utils/FileManagement.hpp utils/FileManagement.cpp\
		utils/TheoryUtils.hpp utils/TheoryUtils.cpp\
		utils/StringUtils.hpp utils/StringUtils.cpp

bin_PROGRAMS = idp
idp_LDADD = $(builddir)/libgidl-2.0.la
idp_SOURCES = main.cpp
