/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef HELP_HPP_
#define HELP_HPP_

#include <sstream>
#include "commands/commandinterface.hpp"
#include "namespace.hpp"
#include "monitors/interactiveprintmonitor.hpp"
#include "utils/StringUtils.hpp"

typedef TypedInference<LIST(Namespace*)> HelpInferenceBase;
class HelpInference: public HelpInferenceBase {
public:
	HelpInference() :
		HelpInferenceBase("help", "Print information about the given namespace, when left blank, print information about the global namespace.", true) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto ns = get<0>(args);
		std::string str = help(ns);
		printmonitor()->print(str);
		return nilarg();
	}
private:
	std::string getPrefix(const Namespace* ns) const {
		std::stringstream prefixs;
		ns->putName(prefixs);
		std::string prefix = prefixs.str();
		if (prefix != "") {
			prefix += "::";
		}
		return prefix;
	}

	std::string printProcedure(std::string prefix, const std::string& name, const std::vector<std::string>& args,
			const std::string& description) const {
		std::stringstream sstr;
		sstr << "\t* " << prefix << name << '(';
		bool begin = true;
		for (auto i = 0; i < args.size(); ++i) {
			if (!begin) {
				sstr << ',';
			}
			begin = false;
			sstr << args[i];
		}

		sstr << ")\n";
		auto tempdesc = replaceAllAndTrimEachLine(description, "\n", "\n\t\t");
		if(tempdesc.at(tempdesc.length()-1)!='\n'){
			tempdesc+='\n';
		}
		sstr << "\t\t" << tempdesc;
		return sstr.str();
	}

	std::string help(Namespace *ns) const {
		if (ns == NULL) {
			return "";
		}
		std::stringstream sstr;
		if (!ns->subspaces().empty()) {
			sstr << "Available subspaces (type help(<subspace>) for more information on a subspace):\n";
			for (auto it = ns->subspaces().cbegin(); it != ns->subspaces().cend(); ++it) {
				sstr << "\t* ";
				it->second->putName(sstr);
				sstr << "\n";
			}
		}

		sstr << "Available procedures:\n";
		std::string prefix = getPrefix(ns);
		int count = 0;
		std::set<std::string> procedures;
		for (auto i = getAllInferences().cbegin(); i < getAllInferences().cend(); ++i) {
			if ((*i)->getNamespace() == ns->name()) {
				std::vector<std::string> args;
				for (auto j = (*i)->getArgumentTypes().cbegin(); j < (*i)->getArgumentTypes().cend(); ++j) {
					args.push_back(toCString(*j));
				}
				auto text = printProcedure(prefix, (*i)->getName(), args, (*i)->getDescription());
				procedures.insert(text);
				count++;
			}
		}

		if (ns->procedures().empty() && count == 0) {
			if (ns->isGlobal()) {
				sstr << "\tThere are no procedures in the global namespace";
			} else {
				sstr << "\tThere are no procedures in namespace ";
				ns->putName(sstr);
			}
			sstr << '\n';
		} else {
			std::string prefix = getPrefix(ns);
			for (auto it = ns->procedures().cbegin(); it != ns->procedures().cend(); ++it) {
				auto text = printProcedure(prefix, it->first, it->second->args(), it->second->description());
				procedures.insert(text);
			}
		}
		for(auto i=procedures.cbegin(); i!=procedures.cend(); ++i){
			sstr <<*i;
		}
		return sstr.str();
	}
};

#endif /* HELP_HPP_ */
