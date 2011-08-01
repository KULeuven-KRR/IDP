/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ALLCOMMANDS_HPP_
#define ALLCOMMANDS_HPP_

#include "internalargument.hpp"
#include "commands/printblocks.hpp"
#include "commands/printdomainatom.hpp"
#include "commands/printoptions.hpp"
#include "commands/newstructure.hpp"
#include "commands/newtheory.hpp"
#include "commands/modelexpand.hpp"
#include "commands/newoptions.hpp"
#include "commands/clonestructure.hpp"
#include "commands/clonetheory.hpp"
#include "commands/pushnegations.hpp"
#include "commands/mergetheories.hpp"
#include "commands/flatten.hpp"
#include "commands/idptype.hpp"
#include "commands/setatomvalue.hpp"
#include "commands/settablevalue.hpp"
#include "commands/domainiterator.hpp"
#include "commands/tableiterator.hpp"
#include "commands/propagate.hpp"
#include "commands/query.hpp"
#include "commands/createtuple.hpp"
#include "commands/createrange.hpp"
#include "commands/ground.hpp"
#include "commands/completion.hpp"
#include "commands/tobdd.hpp"
#include "commands/clean.hpp"
#include "commands/changevocabulary.hpp"
#include "commands/estimatecosts.hpp"
#include "commands/derefandincrement.hpp"
#include "commands/help.hpp"
#include "commands/optimization.hpp"
#include "commands/simplify.hpp"

//TODO add support for easily using these inferences directly in lua, by also providing a help/usage text and replacing idp_intern. with something easier

#include <vector>

// Important: pointer owner is transferred to receiver!
std::vector<Inference*> getAllInferences(){
	std::vector<Inference*> inferences;
	inferences.push_back(SetAtomValueInference::getMakeAtomTrueInference());
	inferences.push_back(SetAtomValueInference::getMakeAtomFalseInference());
	inferences.push_back(SetAtomValueInference::getMakeAtomUnknownInference());
	inferences.push_back(SetTableValueInference::getMakeTableTrueInference());
	inferences.push_back(SetTableValueInference::getMakeTableFalseInference());
	inferences.push_back(SetTableValueInference::getMakeTableUnknownInference());
	inferences.push_back(new DomainIteratorInference());
	inferences.push_back(new TableIteratorInference());
	inferences.push_back(new PropagateInference());
	inferences.push_back(new PrintVocabularyInference());
	inferences.push_back(new PrintTheoryInference());
	inferences.push_back(new PrintDomainAtomInference());
	inferences.push_back(new PrintFormulaInference());
	inferences.push_back(new PrintNamespaceInference());
	inferences.push_back(new PrintOptionInference());
	inferences.push_back(new PrintStructureInference());
	inferences.push_back(new NewTheoryInference());
	inferences.push_back(new CloneStructureInference());
	inferences.push_back(new ModelExpandInference());
	inferences.push_back(new NewOptionsInference());
	inferences.push_back(new NewStructureInference());
	inferences.push_back(new CloneTheoryInference());
	inferences.push_back(new IdpTypeInference());
	inferences.push_back(new FlattenInference());
	inferences.push_back(new MergeTheoriesInference());
	inferences.push_back(new PushNegationsInference());
	inferences.push_back(new QueryInference());
	inferences.push_back(new CreateTupleInference());
	inferences.push_back(new CreateRangeInference());
	inferences.push_back(new GroundInference());
	inferences.push_back(new CompletionInference());
	inferences.push_back(new CleanInference());
	inferences.push_back(new ToBDDInference(AT_FORMULA));
	inferences.push_back(new ToBDDInference(AT_QUERY));
	inferences.push_back(new ChangeVocabularyInference());
	inferences.push_back(new EstimateBDDCostInference());
	inferences.push_back(new EstimateNumberOfAnswersInference());
	inferences.push_back(new TableDerefAndIncrementInference());
	inferences.push_back(new IntDerefAndIncrementInference());
	inferences.push_back(new StringDerefAndIncrementInference());
	inferences.push_back(new DoubleDerefAndIncrementInference());
	inferences.push_back(new CompoundDerefAndIncrementInference());
	inferences.push_back(new GlobalHelpInference());
	inferences.push_back(new HelpInference());
	inferences.push_back(new GroundAndPrintInference());
	inferences.push_back(new OptimizationInference());
	inferences.push_back(new SimplifyInference());
	return inferences;
}

#endif /* ALLCOMMANDS_HPP_ */
