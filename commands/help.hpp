/************************************
	help.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef HELP_HPP_
#define HELP_HPP_

#include <string>
#include <sstream>
#include "commands/commandinterface.hpp"
#include "namespace.hpp"
#include "monitors/interactiveprintmonitor.hpp"

std::string help(Namespace* ns) {
	std::stringstream sstr;
	if(ns->procedures().empty()) {
		if(ns->isGlobal()) { sstr << "There are no procedures in the global namespace\n"; }
		else {
			sstr << "There are no procedures in namespace ";
			ns->putName(sstr);
			sstr << '\n';
		}
	}
	else {
		sstr << "The following procedures are available:\n\n";
		std::stringstream prefixs;
		ns->putName(prefixs);
		std::string prefix = prefixs.str();
		if(prefix != "") { prefix += "::"; }
		for(auto it = ns->procedures().begin(); it != ns->procedures().end(); ++it) {
			sstr << "    * " << prefix << it->second->name() << '(';
			if(not it->second->args().empty()) {
				sstr << it->second->args()[0];
				for(size_t n = 1; n < it->second->args().size(); ++n) {
					sstr << ',' << it->second->args()[n];
				}
			}
			sstr << ")\n";
			sstr << "        " << it->second->description() << "\n";
		}
	}
	if(not ns->subspaces().empty()) {
		sstr << "\nThe following subspaces are available:\n\n";
		for(auto it = ns->subspaces().begin(); it != ns->subspaces().end(); ++it) {
			sstr << "    * ";
			it->second->putName(sstr);
			sstr << '\n';
		}
		sstr << "\nType help(<subspace>) for information on procedures in namespace <subspace>\n";
	}
	return sstr.str();
}

class GlobalHelpInference: public Inference {
public:
	GlobalHelpInference(): Inference("globalhelp") {
	}

	InternalArgument execute(const std::vector<InternalArgument>&) const {
		std::string str = help(Namespace::global());
		printmonitor()->print(str);
		return nilarg();
	}
};

class HelpInference: public Inference {
public:
	HelpInference(): Inference("help", true) {
		add(AT_NAMESPACE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Namespace* ns = args[0].space();
		std::string str = help(ns);
		printmonitor()->print(str);
		return nilarg();
	}
};

#endif /* HELP_HPP_ */
