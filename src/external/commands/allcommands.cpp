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
#include "options.hpp"
#include "createdummytuple.hpp"
#include "minimize.hpp"

#include <vector>

using namespace std;

vector<shared_ptr<Inference>> inferences; // TODO move to globaldata and delete them there!

// Important: pointer owner is transferred to receiver!
const vector<shared_ptr<Inference>>& getAllInferences() {
	if (inferences.size() == 0) {
		inferences.push_back(shared_ptr<Inference>(SetAtomValueInference::getMakeAtomTrueInference()));
		inferences.push_back(shared_ptr<Inference>(SetAtomValueInference::getMakeAtomFalseInference()));
		inferences.push_back(shared_ptr<Inference>(SetAtomValueInference::getMakeAtomUnknownInference()));
		inferences.push_back(shared_ptr<Inference>(SetTableValueInference::getMakeTableTrueInference()));
		inferences.push_back(shared_ptr<Inference>(SetTableValueInference::getMakeTableFalseInference()));
		inferences.push_back(shared_ptr<Inference>(SetTableValueInference::getMakeTableUnknownInference()));
		inferences.push_back(make_shared<TableSizeInference>());
		inferences.push_back(make_shared<DomainIteratorInference>());
		inferences.push_back(make_shared<TableIteratorInference>());
		inferences.push_back(make_shared<TableDerefAndIncrementInference>());
		inferences.push_back(make_shared<DomainDerefAndIncrementInference<int>>());
		inferences.push_back(make_shared<DomainDerefAndIncrementInference<double>>());
		inferences.push_back(make_shared<DomainDerefAndIncrementInference<std::string*>>());
		inferences.push_back(make_shared<DomainDerefAndIncrementInference<const Compound*>>());
		inferences.push_back(make_shared<OptimalPropagateInference>());
		inferences.push_back(make_shared<GroundPropagateInference>());
		inferences.push_back(make_shared<PropagateInference>());
		inferences.push_back(make_shared<PrintVocabularyInference>());
		inferences.push_back(make_shared<PrintTheoryInference>());
		inferences.push_back(make_shared<PrintDomainAtomInference>());
		inferences.push_back(make_shared<PrintFormulaInference>());
		inferences.push_back(make_shared<PrintNamespaceInference>());
		inferences.push_back(make_shared<PrintOptionInference>());
		inferences.push_back(make_shared<PrintStructureInference>());
		inferences.push_back(make_shared<ModelExpandInference>());
		inferences.push_back(make_shared<NewOptionsInference>());
		inferences.push_back(make_shared<NewStructureInference>());
		inferences.push_back(make_shared<CloneStructureInference>());
		inferences.push_back(make_shared<CloneTheoryInference>());
		inferences.push_back(make_shared<IdpTypeInference>());
		inferences.push_back(make_shared<FlattenInference>());
		inferences.push_back(make_shared<MergeTheoriesInference>());
		inferences.push_back(make_shared<PushNegationsInference>());
		inferences.push_back(make_shared<QueryInference>());
		inferences.push_back(make_shared<CreateRangeInference>());
		inferences.push_back(make_shared<GroundInference>());
		inferences.push_back(make_shared<CompletionInference>());
		inferences.push_back(make_shared<CleanInference>());
		inferences.push_back(make_shared<ChangeVocabularyInference>());
		inferences.push_back(make_shared<HelpInference>());
		inferences.push_back(make_shared<PrintGroundingInference>());
		inferences.push_back(make_shared<EntailsInference>());
		inferences.push_back(make_shared<RemoveNestingInference>());
		// TODO issue #50 bdds inferences.push_back(new SimplifyInference());
		inferences.push_back(make_shared<TwoValuedExtensionsOfTableInference>());
		inferences.push_back(make_shared<TwoValuedExtensionsOfStructureInference>());
		inferences.push_back(make_shared<CalculateDefinitionInference>());
		inferences.push_back(make_shared<IsConsistentInference>());
		inferences.push_back(make_shared<SetOptionsInference>());
		inferences.push_back(make_shared<CreateTupleInference>());
		inferences.push_back(make_shared<MinimizeInference>());
		inferences.push_back(make_shared<GetOptionsInference>());
	}
	return inferences;
}
