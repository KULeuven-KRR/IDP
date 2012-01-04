/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "allcommands.hpp"

#include "internalargument.hpp"
#include "printblocks.hpp"
#include "printdomainatom.hpp"
#include "printoptions.hpp"
#include "newstructure.hpp"
#include "modelexpand.hpp"
#include "newoptions.hpp"
#include "clone.hpp"
#include "mergetheories.hpp"
#include "idptype.hpp"
#include "setatomvalue.hpp"
#include "settablevalue.hpp"
#include "propagate.hpp"
#include "query.hpp"
#include "createrange.hpp"
#include "ground.hpp"
#include "transformations.hpp"
#include "clean.hpp"
#include "setvocabulary.hpp"
#include "help.hpp"
#include "iterators.hpp"
#include "entails.hpp"
// TODO issue #50 bdds #include "simplify.hpp"
#include "tablesize.hpp"
#include "twovaluedextensions.hpp"
#include "calculatedefinitions.hpp"
#include "structureproperties.hpp"
#include "setAsCurrentOptions.hpp"
#include "createdummytuple.hpp"

#include <vector>

std::vector<Inference*> inferences; // TODO move to globaldata and delete them there!

// Important: pointer owner is transferred to receiver!
const std::vector<Inference*>& getAllInferences() {
	if (inferences.size() == 0) {
		inferences.push_back(SetAtomValueInference::getMakeAtomTrueInference());
		inferences.push_back(SetAtomValueInference::getMakeAtomFalseInference());
		inferences.push_back(SetAtomValueInference::getMakeAtomUnknownInference());
		inferences.push_back(SetTableValueInference::getMakeTableTrueInference());
		inferences.push_back(SetTableValueInference::getMakeTableFalseInference());
		inferences.push_back(SetTableValueInference::getMakeTableUnknownInference());
		inferences.push_back(new TableSizeInference());
		inferences.push_back(new DomainIteratorInference());
		inferences.push_back(new TableIteratorInference());
		inferences.push_back(new TableDerefAndIncrementInference());
		inferences.push_back(new DomainDerefAndIncrementInference<int>());
		inferences.push_back(new DomainDerefAndIncrementInference<double>());
		inferences.push_back(new DomainDerefAndIncrementInference<std::string*>());
		inferences.push_back(new DomainDerefAndIncrementInference<const Compound*>());
		inferences.push_back(new OptimalPropagateInference());
		inferences.push_back(new GroundPropagateInference());
		inferences.push_back(new PropagateInference());
		inferences.push_back(new PrintVocabularyInference());
		inferences.push_back(new PrintTheoryInference());
		inferences.push_back(new PrintDomainAtomInference());
		inferences.push_back(new PrintFormulaInference());
		inferences.push_back(new PrintNamespaceInference());
		inferences.push_back(new PrintOptionInference());
		inferences.push_back(new PrintStructureInference());
		inferences.push_back(new ModelExpandInference());
		inferences.push_back(new NewOptionsInference());
		inferences.push_back(new NewStructureInference());
		inferences.push_back(new CloneStructureInference());
		inferences.push_back(new CloneTheoryInference());
		inferences.push_back(new IdpTypeInference());
		inferences.push_back(new FlattenInference());
		inferences.push_back(new MergeTheoriesInference());
		inferences.push_back(new PushNegationsInference());
		inferences.push_back(new QueryInference());
		inferences.push_back(new CreateRangeInference());
		inferences.push_back(new GroundInference());
		inferences.push_back(new CompletionInference());
		inferences.push_back(new CleanInference());
		inferences.push_back(new ChangeVocabularyInference());
		inferences.push_back(new HelpInference());
		inferences.push_back(new PrintGroundingInference());
		inferences.push_back(new EntailsInference());
		inferences.push_back(new RemoveNestingInference());
		// TODO issue #50 bdds inferences.push_back(new SimplifyInference());
		inferences.push_back(new TwoValuedExtensionsOfTableInference());
		inferences.push_back(new TwoValuedExtensionsOfStructureInference());
		inferences.push_back(new CalculateDefinitionInference());
		inferences.push_back(new IsConsistentInference());
		inferences.push_back(new SetOptionsInference());
		inferences.push_back(new CreateTupleInference());
	}
	return inferences;
}
