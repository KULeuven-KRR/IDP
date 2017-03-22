/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

class TheoryVisitor;
class DefaultTraversingTheoryVisitor;
class TheoryMutatingVisitor;

class ApproxCheckTwoValued;
class CheckContainment;
class CheckContainsAggTerms;
class CheckContainsRecDefAggTerms;
class CheckContainsFuncTerms;
class CheckPartialTerm;
class CheckSorts;
class CollectQuantifiedVariables;
class CollectOpensOfDefinitions;
class GetDefinedSymbolDirectDependencies;
class CountNbOfSubFormulas;
class DefaultFormulaVisitor;
class FOBDDFactory;
class FOPropagator;
template<class InterpretationFactory, class PropDomain> class TypedFOPropagator;
template<class InterpretationFactory, class PropDomain> class FOPropagatorFactory;
class GenerateBDDAccordingToBounds;
class GrounderFactory;
class Printer;
template<typename Stream> class StreamPrinter;
class TheorySupportedChecker;
class TheorySymmetryAnalyzer;
template<typename Stream> class TPTPPrinter;
template<typename Stream> class EcnfPrinter;
template<typename Stream> class IDPPrinter;
class CountQuantVars;
class SplitIntoMonotoneAgg;
class ReplaceNestedWithTseitinTerm;
class ConstructNewReducedForm;
class Skolemize;
class TopDownApproximatingDefinition;
class BottomUpApproximatingDefinition;
class AddIfCompletion;
class FormulaClauseBuilder;
class CollectSymbols;
class CollectSymbolOccurences;
class ReplaceLTCSymbols;
class RemoveQuantificationsOverSort;
class ContainedVariables;
class SubstituteVarWithDom;
class CombineAggregates;
class AddMarkers;
class ReplacePredByPred;
class ReplacePredByFunctions;
class ReplaceVariableUsingEqualities;

#define VISITORS() \
		friend class DefaultTraversingTheoryVisitor; \
		friend class TheoryVisitor; \
		friend class ApproxCheckTwoValued; \
		friend class CheckContainment; \
		friend class CheckContainsAggTerms; \
		friend class CheckApproxContainsRecDefAggTerms; \
		friend class CheckContainsFuncTerms; \
		friend class CheckContainsDomainTerms; \
		friend class CheckContainsFuncTermsOutsideOfSets; \
		friend class CheckPartialTerm; \
		friend class CheckSorts; \
		friend class CollectOpensOfDefinitions; \
		friend class GetDefinedSymbolDirectDependencies; \
		friend class CountNbOfSubFormulas; \
		friend class DefaultFormulaVisitor; \
		friend class DeriveTermBounds; \
		friend class FOBDDFactory; \
		friend class FOPropagator; \
		template<class InterpretationFactory, class PropDomain> friend class TypedFOPropagator; \
		template<class InterpretationFactory, class PropDomain> friend class FOPropagatorFactory; \
		friend class GenerateBDDAccordingToBounds; \
		friend class GrounderFactory; \
		friend class Printer; \
		template<typename Stream> friend class StreamPrinter; \
		template<typename Stream> friend class TPTPPrinter; \
		template<typename Stream> friend class IDPPrinter; \
		template<typename Stream> friend class EcnfPrinter; \
		friend class CountQuantVars; \
		friend class TheorySupportedChecker; \
		friend class TheorySymmetryAnalyzer; \
		friend class InterchangeabilityAnalyzer; \
		friend class TheoryMutatingVisitor; \
		friend class ReplaceDefinitionsWithCompletion; \
		friend class AddFuncConstraints; \
		friend class DeriveSorts; \
		friend class Flatten; \
		friend class GraphAggregates; \
		friend class GraphFunctions; \
		friend class GraphFuncsAndAggs; \
		friend class GraphFuncsAndAggsForXSB; \
		friend class PushNegations; \
		friend class PushQuantifications; \
		friend class PushQuantificationsCompletely; \
		friend class PullQuantifications; \
		friend class EliminateUniversalQuantifications; \
		friend class RemoveEquivalences; \
		friend class SplitComparisonChains; \
		friend class SubstituteTerm; \
		friend class SubstituteVarWithDom; \
		friend class UnnestTerms; \
		friend class DefinitionsToNormalForm; \
		friend class UnnestForXSB; \
		friend class UnnestFuncsAndAggs; \
		friend class UnnestPartialTerms; \
		friend class UnnestThreeValuedTerms;\
		friend class UnnestHeadTermsNotVarsOrDomElems;\
		friend class FindDelayPredForms;\
		friend class FindDoubleDelayLiteral;\
		friend class SplitIntoMonotoneAgg;\
		friend class AddIfCompletion;\
		friend class FormulaClauseBuilder; \
		friend class ReplaceNestedWithTseitinTerm;\
		friend class ConstructNewReducedForm;\
		friend class TopDownApproximatingDefinition;\
		friend class ContainedVariables;\
		friend class CombineAggregates;\
		friend class TopDownApproximatingDefinitionForallRule;\
		friend class BottomUpApproximatingDefinition;\
		friend class Skolemize;\
		friend class ReplacePredByPred;\
		friend class CollectQuantifiedVariables;\
		friend class CollectSymbols;\
		friend class CollectSymbolOccurences;\
		friend class AddMarkers;\
		friend class ReplaceLTCSymbols;\
		friend class ReplaceVariableUsingEqualities; \
		friend class ReplacePredByFunctions;\
		friend class Simplify;\
		friend class RemoveQuantificationsOverSort;\
		friend class CardConstrToFO;\
		friend class Metafier;\
		friend class SubstituteVarWithVar;

class AbstractTheory;
class Theory;
class AbstractGroundTheory;
class GroundPolicy;
template<typename T> class GroundTheory;
class Formula;
class PredForm;
class EqChainForm;
class EquivForm;
class BoolForm;
class QuantForm;
class AggForm;
class GroundDefinition;
class GroundRule;
class PCGroundRule;
class AggGroundRule;
class GroundSet;
class GroundAggregate;
class Rule;
class Definition;
class FixpDef;
class Term;
class VarTerm;
class FuncTerm;
class DomainTerm;
class AggTerm;
class CPTerm;
class CPVarTerm;
class CPSetTerm;
class CPReification;
class SetExpr;
class EnumSetExpr;
class QuantSetExpr;
class UserProcedure;

#define VISITORFRIENDS() \
		friend class AbstractTheory; \
		friend class Theory; \
		friend class AbstractGroundTheory; \
		template<typename T> friend class GroundTheory; \
		friend class Formula; \
		friend class PredForm; \
		friend class EqChainForm; \
		friend class EquivForm; \
		friend class BoolForm; \
		friend class QuantForm; \
		friend class AggForm; \
		friend class GroundDefinition; \
		friend class GroundRule; \
		friend class PCGroundRule; \
		friend class AggGroundRule; \
		friend class GroundSet; \
		friend class GroundAggregate; \
		friend class UserProcedure; \
		friend class Rule; \
		friend class Definition; \
		friend class FixpDef; \
		friend class Term; \
		friend class VarTerm; \
		friend class FuncTerm; \
		friend class DomainTerm; \
		friend class AggTerm; \
		friend class CPTerm; \
		friend class CPVarTerm; \
		friend class CPSetTerm; \
		friend class CPReification; \
		friend class SetExpr; \
		friend class EnumSetExpr; \
		friend class QuantSetExpr;

#define ACCEPTDECLAREBOTH(Type)\
protected:\
	VISITORS()\
	virtual void accept(TheoryVisitor* v) const = 0;\
	virtual Type* accept(TheoryMutatingVisitor* v) = 0;

#define ACCEPTNONMUTATING()\
protected:\
	VISITORS()\
	virtual void accept(TheoryVisitor* v) const;

#define ACCEPTBOTH(Type)\
protected:\
	VISITORS()\
	virtual void accept(TheoryVisitor* v) const;\
	virtual Type* accept(TheoryMutatingVisitor* v);

#define IMPLACCEPTNONMUTATING(VisitingType)\
	void VisitingType::accept(TheoryVisitor* v) const {\
		v->visit(this);\
	}

#define IMPLACCEPTBOTH(VisitingType, Type)\
	void VisitingType::accept(TheoryVisitor* v) const {\
		v->visit(this);\
	}\
	Type* VisitingType::accept(TheoryMutatingVisitor* v) {\
		return v->visit(this);\
	}
