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

#include <vector>

std::vector<Inference*> getAllInferences(){
	std::vector<Inference*> inferences;
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
	return inferences;
}

#endif /* ALLCOMMANDS_HPP_ */
