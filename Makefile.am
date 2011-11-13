SUBDIRS = lib pcsolver
AM_CXXFLAGS = @AC_CXXFLAGS@ -Wall -Wextra\
		-std=c++0x\
		-I$(srcdir)/lib/loki/include\
		-I$(srcdir)/lib/lua/etc -I$(srcdir)/lib/lua/src\
		-I$(builddir)/lib/lua/src  -I $(srcdir)/lib/\
		-DDATADIR=\"$(datadir)\" -DRCDIR=\"$(localstatedir)\"\
		-DINTERNALLIBARYLUA=\"/std/idp_intern.lua\"\
		-DINTERNALLIBARYIDP=\"/std/idp_intern.idp\" -DCONFIGFILENAME=\"/idp/.idprc\"
		
if USEINTERACTIVE
AM_CXXFLAGS += -DUSEINTERACTIVE
endif
		
AM_LDFLAGS = @AC_LDFLAGS@

AM_YFLAGS = -d -v

bin_PROGRAMS = gidl

gidl_LDADD = $(builddir)/lib/lua/src/.libs/liblua.la
gidl_LDADD += $(builddir)/pcsolver/src/libminisatid-2.0.la


BUILT_SOURCES = parse.h

grounder_SOURCES = \
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
		parse.yy yyltype.hpp\
		lex.ll\
		clconst.hpp\
		GeneralUtils.hpp\
		insert.hpp insert.cpp\
		error.hpp error.cpp\
		common.hpp common.cpp\
		interactive.hpp interactive.cpp\
		printers/print.hpp printers/print.cpp\
		printers/idpprinter.hpp printers/ecnfprinter.hpp printers/tptpprinter.hpp\
		checker.hpp checker.cpp\
		propagate.hpp propagate.cpp\
		ecnf.hpp ecnf.cpp\
		ground.hpp ground.cpp\
		symbolicstructure.hpp symbolicstructure.cpp\
        symmetry.hpp symmetry.cpp\
		monitors/interactiveprintmonitor.hpp monitors/tracemonitor.hpp\
		monitors/luatracemonitor.hpp monitors/luainteractiveprintmonitor.hpp\
		groundtheories/PrintGroundPolicy.hpp\
		groundtheories/GroundPolicy.hpp\
		groundtheories/SolverPolicy.hpp groundtheories/SolverPolicy.cpp\
		groundtheories/AbstractGroundTheory.hpp\
		grounders/LazyQuantGrounder.hpp grounders/LazyQuantGrounder.cpp\
		grounders/DefinitionGrounders.hpp grounders/DefinitionGrounders.cpp\
		grounders/FormulaGrounders.hpp grounders/FormulaGrounders.cpp\
		grounders/SetGrounders.hpp grounders/SetGrounders.cpp\
		grounders/TermGrounders.hpp grounders/TermGrounders.cpp\
		grounders/GroundUtils.hpp\
		grounders/Grounder.hpp grounders/Grounder.cpp\
		rungidl.hpp rungidl.cpp\
		commands/allcommands.hpp\
		commands/optimization.hpp\
		commands/commandinterface.hpp\
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
		commands/createtuple.hpp\
		commands/ground.hpp\
		commands/completion.hpp\
		commands/tobdd.hpp\
		commands/clean.hpp\
		commands/help.hpp\
		commands/changevocabulary.hpp\
		commands/estimatecosts.hpp\
		commands/detectsymmetry.hpp\
		commands/derefandincrement.hpp\
		commands/entails.hpp\
		commands/removenesting.hpp\
		commands/simplify.hpp\
		commands/createrange.hpp\
		commands/tablesize.hpp\
		PropagatorFactory.hpp PropagatorFactory.cpp\
		generators/ArithmeticOperatorsGenerator.hpp\
		generators/ComparisonGenerator.hpp\
		generators/BasicGenerators.hpp\
		generators/EnumLookupGenerator.hpp\
		generators/FalseQuantKernelGenerator.hpp\
		generators/InstGenerator.hpp\
		generators/InverseAbsValueGenerator.hpp\
		generators/InverseUnaFunctionGenerator.hpp\
		generators/InvertNumericGenerator.hpp\
		generators/LookupGenerator.hpp\
		generators/SimpleFuncGenerator.hpp\
		generators/SortInstGenerator.hpp\
		generators/SortLookupGenerator.hpp\
		generators/TableGenerator.hpp\
		generators/TreeInstGenerator.hpp\
		generators/BasicCheckers.hpp\
		generators/GeneratorNodes.hpp\
		generators/TrueQuantKernelGenerator.hpp\
		generators/GeneratorFactory.cpp\
		generators/BDDBasedGeneratorFactory.cpp\
		OptionsStack.hpp OptionsStack.cpp\
		theoryinformation/CollectOpensOfDefinitions.hpp theoryinformation/CollectOpensOfDefinitions.cpp\
		theoryinformation/CheckContainment.hpp theoryinformation/CheckContainment.cpp\
		theoryinformation/CheckContainsFuncTerms.hpp\
		theoryinformation/CheckSorts.hpp theoryinformation/CheckSorts.cpp\
		theorytransformations/PushNegations.hpp theorytransformations/PushNegations.cpp\
		theorytransformations/Flatten.hpp theorytransformations/Flatten.cpp\
		theorytransformations/CountSubformulas.hpp\
		theorytransformations/CheckFuncTerms.hpp\
		theorytransformations/DeriveSorts.hpp theorytransformations/DeriveSorts.cpp\
		theorytransformations/AddCompletion.hpp theorytransformations/AddCompletion.cpp\
		theorytransformations/GraphFunctions.hpp theorytransformations/GraphFunctions.cpp\
		theorytransformations/GraphAggregates.hpp theorytransformations/GraphAggregates.cpp\
		theorytransformations/PushQuantifications.hpp theorytransformations/PushQuantifications.cpp\
		theorytransformations/RemoveEquivalences.hpp theorytransformations/RemoveEquivalences.cpp\
		theorytransformations/SplitComparisonChains.hpp theorytransformations/SplitComparisonChains.cpp\
		theorytransformations/SubstituteTerm.hpp\
		theorytransformations/UnnestPartialTerms.hpp\
		theorytransformations/UnnestTerms.hpp theorytransformations/UnnestTerms.cpp\
		theorytransformations/UnnestThreeValuedTerms.hpp theorytransformations/UnnestThreeValuedTerms.cpp\
		TheoryVisitor.hpp TheoryVisitor.cpp\
		TheoryMutatingVisitor.hpp TheoryMutatingVisitor.cpp\
		theorytransformations/Utils.hpp theorytransformations/Utils.cpp\
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
		fobdds/bddvisitors/CollectSameOperationTerms.hpp
		
gidl_dist = $(srcdir)/lib/loki-0.1.7
		
gidl_SOURCES = main.cpp $(grounder_SOURCES)

AM_CXXFLAGS+=-I $(top_srcdir)/pcsolver/src/ 

LIBSRCDIR = $(srcdir)/lib
LIBBLDDIR = $(builddir)/lib

# THREADING SUPPORT
AM_LDFLAGS += -lpthread

# CALLBACK FRAMEWORK
AM_CXXFLAGS += -I $(LIBSRCDIR)/pluscallback1.7/

data_stddir = $(datadir)/std

data_std_DATA = data/idp_intern.idp data/idp_intern.lua data/mx.idp data/theory.idp data/domain.idp

data_localdir = $(localstatedir)/idp
data_local_DATA = data/.idprc

dist_doc_DATA = README $(data_std_DATA) $(data_local_DATA)

if BUILDTESTS
bin_PROGRAMS += alltests
endif 

alltests_SOURCES = tests/testsuite.cpp tests/mxtests.cpp tests/generatortests.cpp tests/bddtests.cpp $(grounder_SOURCES)
alltests_LDADD = -lgtest -lpthread $(gidl_LDADD)

AM_CXXFLAGS += -DTESTDIR=\"$(abs_srcdir)/tests/simplemxtests/\"

TESTS = alltests
