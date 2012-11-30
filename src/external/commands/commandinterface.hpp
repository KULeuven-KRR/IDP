/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef COMMANDINTERFACE_HPP_
#define COMMANDINTERFACE_HPP_

#include <vector>
#include <string>

#include <internalargument.hpp>
#include <monitors/interactiveprintmonitor.hpp>
#include "common.hpp"

#include "loki/Typelist.h"

std::string getInferenceNamespaceName();
std::string getVocabularyNamespaceName();
std::string getTheoryNamespaceName();
std::string getTermNamespaceName();
std::string getQueryNamespaceName();
std::string getOptionsNamespaceName();
std::string getStructureNamespaceName();

//TODO refactor the monitors as extra arguments

template<typename T> struct Type2Value;

#define LIST(...) Loki::TL::MakeTypelist<__VA_ARGS__>::Result

#define MAPPING(object, enumvalue)\
template<>\
struct Type2Value<object>{\
	static ArgType get(){\
		return enumvalue;\
	}\
};

// NOTE: also implement the appropriate get method in internalargument!
MAPPING(AbstractTheory*, AT_THEORY)
MAPPING(int, AT_INT)
MAPPING(double, AT_DOUBLE)
MAPPING(std::string*, AT_STRING)
MAPPING(const Compound*, AT_COMPOUND)
MAPPING(AbstractStructure*, AT_STRUCTURE)
MAPPING(Vocabulary*, AT_VOCABULARY)
MAPPING(SortIterator*, AT_DOMAINITERATOR)
MAPPING(TableIterator*, AT_TABLEITERATOR)
MAPPING(LuaTraceMonitor*, AT_TRACEMONITOR)
MAPPING(Formula*, AT_FORMULA)
MAPPING(Namespace*, AT_NAMESPACE)
MAPPING(const PredTable*, AT_PREDTABLE)
MAPPING(SortTable*, AT_DOMAIN)
MAPPING(const DomainAtom*, AT_DOMAINATOM)
MAPPING(Options*, AT_OPTIONS)
MAPPING(Query*, AT_QUERY)
MAPPING(PredInter*, AT_PREDINTER)
MAPPING(ElementTuple*, AT_TUPLE)
MAPPING(std::vector<InternalArgument>*, AT_TABLE)
MAPPING(bool, AT_BOOLEAN)
MAPPING(Term*, AT_TERM)

class Inference {
private:
	std::string _name; //!< the name of the procedure
	std::string _description;
	std::string _space;
	std::vector<ArgType> _argtypes; //!< types of the input arguments
	bool needprintmonitor_, needtracemonitor_;
	InteractivePrintMonitor* printmonitor_;

protected:
	void addType(ArgType arg) {
		_argtypes.push_back(arg);
	}

	InteractivePrintMonitor* printmonitor() const {
		return printmonitor_;
	}

	void setNameSpace(const std::string& namespacename);

public:
	Inference(const std::string& name, const std::string& description, bool needprintmonitor = false)
			: _name(name), _description(description), _space(getGlobalNamespaceName()), needprintmonitor_(needprintmonitor), needtracemonitor_(false), printmonitor_(NULL) {
	}
	virtual ~Inference() {
	}

	const std::vector<ArgType>& getArgumentTypes() const {
		return _argtypes;
	}

	const std::string& getName() const {
		return _name;
	}
	const std::string& getDescription() const {
		return _description;
	}
	const std::string& getNamespace() const {
		return _space;
	}

	virtual InternalArgument execute(const std::vector<InternalArgument>&) const = 0;

	bool needPrintMonitor() const {
		return needprintmonitor_;
	}
	void addPrintMonitor(InteractivePrintMonitor* monitor) {
		printmonitor_ = monitor;
	}

	void clean() {
		if (printmonitor_ != NULL) {
			delete (printmonitor_);
			printmonitor_ = NULL;
		}
	}

	ArgType getArgType(int index) const {
		return _argtypes[index];
	}
};

template<typename T>
class TypedInference;
template<typename RemainingList, typename FullList>
struct AddTypes {
	static void add(TypedInference<FullList>* inf);
};

template<typename T>
class TypedInference: public Inference {
public:
	TypedInference(const std::string& name, const std::string& description, bool needprintmonitor = false)
			: Inference(name, description, needprintmonitor) {
		AddTypes<T, T>::add(this);
	}
	virtual ~TypedInference() {
	}

	template<typename RemainingList, typename FullList> friend class AddTypes;

	template<int index>
	typename Loki::TL::TypeAt<T, index>::Result get(const std::vector<InternalArgument>& args) const {
		auto arg = args[index];
		Assert(arg._type==getArgType(index));
		return arg.get<typename Loki::TL::TypeAt<T, index>::Result>();
	}
};

template<typename RemainingList, typename FullList>
void AddTypes<RemainingList, FullList>::add(TypedInference<FullList>* inf) {
	inf->addType(Type2Value<typename RemainingList::Head>::get());
	AddTypes<typename RemainingList::Tail, FullList>::add(inf);
}

template<typename FullList>
struct AddTypes<Loki::NullType, FullList> {
	static void add(TypedInference<FullList>*) {
	}
};

typedef TypedInference<LIST()> EmptyBase;
typedef TypedInference<LIST(AbstractStructure*)> StructureBase;
typedef TypedInference<LIST(Term*)> TermBase;
typedef TypedInference<LIST(std::string*)> StringBase;
typedef TypedInference<LIST(AbstractTheory*)> TheoryBase;
typedef TypedInference<LIST(AbstractTheory*, AbstractStructure*)> TheoryStructureBase;
typedef TypedInference<LIST(SortTable*)> SortTableBase;
typedef TypedInference<LIST(const PredTable*)> PredTableBase;
typedef TypedInference<LIST(Query*)> QueryBase;
typedef TypedInference<LIST(Options*)> OptionsBase;

#endif /* COMMANDINTERFACE_HPP_ */
