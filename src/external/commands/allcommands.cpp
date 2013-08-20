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

#include "allcommands.hpp"

// NOTE: all inferences available externally should be included here and added to the vector of inferences.
#include "internalargument.hpp"
#include "printblocks.hpp"
#include "printasbdd.hpp"
#include "printoptions.hpp"
#include "newstructure.hpp"
#include "modelexpand.hpp"
#include "clone.hpp"
#include "mergetheories.hpp"
#include "mergestructures.hpp"
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
#include "getvocabulary.hpp"
#include "help.hpp"
#include "iterators.hpp"
#include "entails.hpp"
#include "tablesize.hpp"
#include "twovaluedextensions.hpp"
#include "calculatedefinitions.hpp"
#include "structureproperties.hpp"
#include "options.hpp"
#include "createdummytuple.hpp"
#include "minimize.hpp"
#include "parse.hpp"
#include "theoryquery.hpp"
#include "unsatcore.hpp"

#include "answer.hpp" //easter egg

#include <vector>

using namespace std;

vector<shared_ptr<Inference>> inferences; // TODO move to globaldata and delete them there!

// Note: pointer ownership is transferred to the receiver, so should be heap allocated data!
const vector<shared_ptr<Inference>>& getAllInferences() {
	if (inferences.size() != 0) {
		return inferences;
	}
	// make_shared creates a new object based on a default constructor. Some inference classes can construct multiple inference objects, so make_shared should not be applied.
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
	inferences.push_back(make_shared<PrintOptionInference>());
	inferences.push_back(make_shared<PrintInference<LIST(const PredTable*)>>());
	inferences.push_back(make_shared<PrintInference<LIST(AbstractTheory*)>>());
	inferences.push_back(make_shared<PrintInference<LIST(Namespace*)>>());
	inferences.push_back(make_shared<PrintInference<LIST(Query*)>>());
	inferences.push_back(make_shared<PrintInference<LIST(Formula*)>>());
	inferences.push_back(make_shared<PrintInference<LIST(Term*)>>());
	inferences.push_back(make_shared<PrintInference<LIST(Vocabulary*)>>());
	inferences.push_back(make_shared<PrintInference<LIST(Structure*)>>());
	inferences.push_back(make_shared<PrintAsBDDInference>());
	inferences.push_back(make_shared<ModelExpandInference>());
	inferences.push_back(make_shared<ModelExpandWithOutputVocInference>());
	inferences.push_back(make_shared<NewOptionsInference>());
	inferences.push_back(make_shared<NewStructureInference>());
	inferences.push_back(make_shared<CloneStructureInference>());
	inferences.push_back(make_shared<CloneTheoryInference>());
	inferences.push_back(make_shared<IdpTypeInference>());
	inferences.push_back(make_shared<FlattenInference>());
	inferences.push_back(make_shared<MergeTheoriesInference>());
	inferences.push_back(make_shared<MergeStructuresInference>());
	inferences.push_back(make_shared<PushNegationsInference>());
	inferences.push_back(make_shared<QueryInference>());
	inferences.push_back(make_shared<CreateRangeInference>());
	inferences.push_back(make_shared<GroundInference>());
	inferences.push_back(make_shared<CompletionInference>());
	inferences.push_back(make_shared<CleanInference>());
	inferences.push_back(make_shared<ChangeVocabularyInference>());
	inferences.push_back(make_shared<GetVocabularyInference<LIST(Structure*)>>());
	inferences.push_back(make_shared<GetVocabularyInference<LIST(AbstractTheory*)>>());
	inferences.push_back(make_shared<GetVocabularyInference<LIST(Term*)>>());
	inferences.push_back(make_shared<GetVocabularyInference<LIST(Query*)>>());
	inferences.push_back(make_shared<HelpInference>());
	inferences.push_back(make_shared<PrintGroundingInference>());
	inferences.push_back(make_shared<EntailsInference>());
	inferences.push_back(make_shared<RemoveNestingInference>());
	// TODO issue #50 bdds inferences.push_back(new SimplifyInference());
	inferences.push_back(make_shared<TwoValuedExtensionsOfTableInference>());
	inferences.push_back(make_shared<TwoValuedExtensionsOfStructureInference>());
	inferences.push_back(make_shared<CalculateDefinitionInference>());
	inferences.push_back(make_shared<IsConsistentInference>());
	inferences.push_back(make_shared<StructureEqualityInference>());
	inferences.push_back(make_shared<SetOptionsInference>());
	inferences.push_back(make_shared<CreateTupleInference>());
	inferences.push_back(make_shared<MinimizeInference>());
	inferences.push_back(make_shared<GetOptionsInference>());
	inferences.push_back(make_shared<ParseInference>());
	inferences.push_back(make_shared<TheoryQueryInference>());
	inferences.push_back(make_shared<UnsatCoreInference>());

	inferences.push_back(make_shared<AnswerInference>());
	return inferences;
}
