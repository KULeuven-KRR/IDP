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

#ifndef HELP_HPP_
#define HELP_HPP_

#include <sstream>
#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "monitors/interactiveprintmonitor.hpp"
#include "utils/StringUtils.hpp"

typedef TypedInference<LIST(Namespace*)> HelpInferenceBase;
class HelpInference: public HelpInferenceBase {
public:
	HelpInference()
			: HelpInferenceBase("help", "Print information about the given namespace, when left blank, print information about the global namespace.", true) {
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

	std::string printProcedure(const std::string& name, const std::vector<std::string>& args, const std::string& description) const {
		std::stringstream sstr;
		sstr << name << '(';
		printList(sstr, args, ",");
		sstr << ")\n";
		auto tempdesc = replaceAllAndTrimEachLine(description, "\n", "\n\t\t\t");
		sstr << "\t\t\t" << tempdesc;
		return sstr.str();
	}

	template<class List>
	void printSubBlockInfo(const List& subblocks, const std::string& blocktype, std::ostream& ss) const {
		if (subblocks.empty()) {
			return;
		}
		ss <<"\t" <<blocktype <<":\n";
		for (auto it = subblocks.cbegin(); it != subblocks.cend(); ++it) {
			ss << "\t\t* " <<it->first <<"\n";
		}
	}

	std::string help(Namespace *ns) const {
		if (ns == NULL) {
			return "";
		}
		std::stringstream sstr;
		if(ns!=getGlobal()->getGlobalNamespace()){
			sstr <<"\tNamespace " << ns->name() << " consists of: \n";
		}
		printSubBlockInfo(ns->subspaces(), "Namespaces", sstr);
		if(not ns->subspaces().empty()){
			sstr <<"\t\t\tFor more information on a nested namespace, type help(<namespacename>)\n";
		}
		printSubBlockInfo(ns->vocabularies(), "Vocabularies", sstr);
		printSubBlockInfo(ns->theories(), "Theories", sstr);
		printSubBlockInfo(ns->structures(), "Structures", sstr);
		printSubBlockInfo(ns->terms(), "Terms", sstr);
		printSubBlockInfo(ns->queries(), "Queries", sstr);
		// FIXME additional blocks are not detected automatically!

		// Printing procedures
		std::map<std::string, std::string> procedures;
		// Get text for each internal procedure in the given namespace
		for (auto i = getAllInferences().cbegin(); i < getAllInferences().cend(); ++i) {
			if ((*i)->getNamespace() == ns->name()) {
				std::vector<std::string> args;
				for (auto j = (*i)->getArgumentTypes().cbegin(); j < (*i)->getArgumentTypes().cend(); ++j) {
					args.push_back(toCString(*j));
				}
				auto text = printProcedure((*i)->getName(), args, (*i)->getDescription());
				procedures.insert({text, ""});
			}
		}
		// Get text for each user defined procedure in the given namespace
		for (auto it = ns->procedures().cbegin(); it != ns->procedures().cend(); ++it) {
			auto text = printProcedure(it->first, it->second->args(), it->second->description());
			procedures.insert({text, ""});
		}

		if (procedures.empty()) {
			if (ns->isGlobal()) {
				sstr << "\t\tThere are no procedures in the global namespace\n";
			} else {
				sstr << "\t\tThere are no procedures in namespace ";
				ns->putName(sstr);
				sstr << '\n';
			}
		} else {
			printSubBlockInfo(procedures, "Procedures", sstr);
		}

		return sstr.str();
	}
};

#endif /* HELP_HPP_ */
