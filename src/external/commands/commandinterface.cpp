#include "commandinterface.hpp"
#include "namespace/namespace.hpp"

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

void Inference::setNameSpace(const std::string& namespacename) {
	_space = namespacename;
	auto ns = getGlobal()->getStdNamespace();
	if(ns->subspaces().find(namespacename)==ns->subspaces().cend()){
		auto newns = new Namespace(namespacename, ns, ParseInfo());
		ns->add(newns);
	}
}
