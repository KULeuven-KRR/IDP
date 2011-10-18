#ifndef TESTMX_HPP_
#define TESTMX_HPP_

#include <vector>
#include <string>
#include <iostream>
#include "commandinterface.hpp"
#include "rungidl.hpp"

class TestMXInference: public Inference {
public:
	TestMXInference(): Inference("testmx", false, true) {
		add(AT_INT);
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		int nbmodels = args[0]._value._int;
		AbstractTheory* theory = args[1].theory()->clone();
		AbstractStructure* structure = args[2].structure()->clone();
		Options* options = args[3].options();

		auto mx = new ModelExpandInference();
		auto models = mx->doModelExpansion(theory, structure, options);
		if(models.size()==nbmodels){
			setTestStatus(Status::SUCCESS);
		}else{
			std::cerr <<"Expected = " <<nbmodels <<", found = " <<models.size() <<".\n";
			setTestStatus(Status::FAIL);
		}
		delete(mx);

		auto ia = InternalArgument();
		ia._type = AT_BOOLEAN;
		ia._value._boolean = getTestStatus()==Status::SUCCESS;
		return ia;
	}
};


#endif /* TESTMX_HPP_ */
