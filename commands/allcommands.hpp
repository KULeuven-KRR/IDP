/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ALLCOMMANDS_HPP_
#define ALLCOMMANDS_HPP_

#include "internalargument.hpp"
#include "commands/printtheory.hpp"
#include "commands/printformula.hpp"
#include "commands/printdomainatom.hpp"
#include "commands/printstructure.hpp"
#include "commands/printvocabulary.hpp"
#include "commands/printoptions.hpp"
#include "commands/printnamespace.hpp"
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
#include "commands/ground.hpp"
#include "commands/completion.hpp"
#include "commands/tobdd.hpp"
#include "commands/clean.hpp"
#include "commands/changevocabulary.hpp"
#include "commands/estimatecosts.hpp"
#include "commands/derefandincrement.hpp"
#include "commands/help.hpp"

#include <vector>

// Important: pointer owner is transferred to receiver!
std::vector<Inference*> getAllInferences(){
	std::vector<Inference*> inferences;
	inferences.push_back(new MakeFalseInference());
	inferences.push_back(new MakeTrueInference());
	inferences.push_back(new MakeUnknownInference());
	inferences.push_back(new MakeTableFalseInference());
	inferences.push_back(new MakeTableTrueInference());
	inferences.push_back(new MakeTableUnknownInference());
	inferences.push_back(new DomainIteratorInference());
	inferences.push_back(new TableIteratorInference());
	inferences.push_back(new PropagateInference());
	inferences.push_back(new PrintTheoryInference());
	inferences.push_back(new PrintDomainAtomInference());
	inferences.push_back(new PrintFormulaInference());
	inferences.push_back(new PrintNamespaceInference());
	inferences.push_back(new PrintOptionInference());
	inferences.push_back(new PrintVocabularyInference());
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
	inferences.push_back(new GroundInference());
	inferences.push_back(new CompletionInference());
	inferences.push_back(new CleanInference());
	inferences.push_back(new ToBDDInference());
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
	return inferences;
}

#endif /* ALLCOMMANDS_HPP_ */