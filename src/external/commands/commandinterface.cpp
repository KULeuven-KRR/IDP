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

#include "commandinterface.hpp"
#include "namespace/namespace.hpp"
class Theory;
class Structure;
class Vocabulary;

std::string getInferenceNamespaceName(){
	return "inferences";
}

std::string getVocabularyNamespaceName(){
	return "vocabulary";
}

std::string getTheoryNamespaceName(){
	return "theory";
}

std::string getTermNamespaceName(){
	return "term";
}

std::string getQueryNamespaceName(){
	return "query";
}

std::string getOptionsNamespaceName(){
	return "options";
}

std::string getStructureNamespaceName(){
	return "structure";
}

template<>
std::string getNamespaceName<Structure*>(){
	return getStructureNamespaceName();
}

template<>
std::string getNamespaceName<AbstractTheory*>(){
	return getTheoryNamespaceName();
}

template<>
std::string getNamespaceName<Vocabulary*>(){
	return getVocabularyNamespaceName();
}

void Inference::setNameSpace(const std::string& namespacename) {
	_space = namespacename;
	auto ns = getGlobal()->getStdNamespace();
	if(ns->subspaces().find(namespacename)==ns->subspaces().cend()){
		auto newns = new Namespace(namespacename, ns, ParseInfo());
		ns->add(newns);
	}
}
