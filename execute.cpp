/************************************
	execute.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <set>
#include <iostream>
#include <cstdlib>
#include "lua.hpp"
#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "external/MonitorInterface.hpp"
#include "theory.hpp"
#include "options.hpp"
#include "namespace.hpp"
#include "execute.hpp"
#include "error.hpp"
#include "print.hpp"
#include "ground.hpp"
#include "ecnf.hpp"
#include "fobdd.hpp"
#include "propagate.hpp"
#include "generator.hpp"
#include "internalluaargument.hpp"
using namespace std;
using namespace LuaConnection;

extern void parsefile(const string&);

typedef MinisatID::WrappedPCSolver SATSolver;

/******************************
	User defined procedures
******************************/

int UserProcedure::_compilenumber = 0;

/**************************
	Internal procedures
**************************/

/**
 * Constructor to convert a domain element to an InternalArgument
 */
InternalArgument::InternalArgument(const DomainElement* el) {
	switch(el->type()) {
		case DET_INT:
			_type = AT_INT;
			_value._int = el->value()._int;
			break;
		case DET_DOUBLE:
			_type = AT_DOUBLE;
			_value._double = el->value()._double;
			break;
		case DET_STRING:
			_type = AT_STRING;
			_value._string = StringPointer(*(el->value()._string));
			break;
		case DET_COMPOUND:
			_type = AT_COMPOUND;
			_value._compound = el->value()._compound;
			break;
		default:
			assert(false);
	}
}

/********************************************
	Implementation of internal procedures
********************************************/

//TODO separate from lua state! (by first converting and calling with the converted types?)

string help(Namespace* ns) {
	stringstream sstr;
	if(ns->procedures().empty()) {
		if(ns->isGlobal()) sstr << "There are no procedures in the global namespace\n";
		else {
			sstr << "There are no procedures in namespace ";
			ns->putname(sstr);
			sstr << '\n';
		}
	}
	else {
		sstr << "The following procedures are available:\n\n";
		stringstream prefixs;
		ns->putname(prefixs);
		string prefix = prefixs.str();
		if(prefix != "") prefix += "::";
		for(map<string,UserProcedure*>::const_iterator it = ns->procedures().begin(); it != ns->procedures().end(); ++it) {
			sstr << "    * " << prefix << it->second->name() << '(';
			if(!it->second->args().empty()) {
				sstr << it->second->args()[0];
				for(unsigned int n = 1; n < it->second->args().size(); ++n) {
					sstr << ',' << it->second->args()[n];
				}
			}
			sstr << ")\n";
			sstr << "        " << it->second->description() << "\n";
		}
	}
	if(!ns->subspaces().empty()) {
		sstr << "\nThe following subspaces are available:\n\n";
		for(map<string,Namespace*>::const_iterator it = ns->subspaces().begin(); it != ns->subspaces().end(); ++it) {
			sstr << "    * ";
			it->second->putname(sstr); 
			sstr << '\n';
		}
		sstr << "\nType help(<subspace>) for information on procedures in namespace <subspace>\n";
	}
	return sstr.str();
}

InternalArgument globalhelp(const vector<InternalArgument>&, lua_State* L) {
	string str = help(Namespace::global());
	lua_getglobal(L,"print");
	lua_pushstring(L,str.c_str());
	lua_call(L,1,0);
	return nilarg();
}

InternalArgument help(const vector<InternalArgument>& args, lua_State* L) {
	Namespace* ns = args[0].space();
	string str = help(ns);
	lua_getglobal(L,"print");
	lua_pushstring(L,str.c_str());
	lua_call(L,1,0);
	return nilarg();
}

/********************************
	Model expansion inference
********************************/

//FIXME this has to become a monitor => an extra internal argument to which can be written!

/**
 * Class to output the trace of a solver
 **/
class SATTraceWriter {
	private:
		GroundTranslator*	_translator;
		lua_State*			_state;
		string*				_registryindex;
		static int			_tracenr;

	public:
		SATTraceWriter(GroundTranslator* trans, lua_State* L) : _translator(trans), _state(L) { 
			++_tracenr;
			_registryindex = StringPointer(string("sat_trace_") + toString(_tracenr));
			lua_newtable(L);
			lua_setfield(L,LUA_REGISTRYINDEX,_registryindex->c_str());
		}

		void backtrack(int dl){
			lua_getglobal(_state,"table");
			lua_getfield(_state,-1,"insert");
			lua_getfield(_state,LUA_REGISTRYINDEX,_registryindex->c_str());
			lua_newtable(_state);
			lua_pushstring(_state,"backtrack");
			lua_setfield(_state,-2,"type");
			lua_pushinteger(_state,dl);
			lua_setfield(_state,-2,"dl");
			lua_call(_state,2,0);
			lua_pop(_state,1);
		}

		void propagate(MinisatID::Literal lit, int dl){
			lua_getglobal(_state,"table");
			lua_getfield(_state,-1,"insert");
			lua_getfield(_state,LUA_REGISTRYINDEX,_registryindex->c_str());
			lua_newtable(_state);
			lua_pushstring(_state,"assign");
			lua_setfield(_state,-2,"type");
			lua_pushinteger(_state,dl);
			lua_setfield(_state,-2,"dl");
			lua_pushboolean(_state,!lit.hasSign());
			lua_setfield(_state,-2,"value");
			PFSymbol* s = _translator->symbol(lit.getAtom().getValue());
			if(s) {
				const ElementTuple& args = _translator->args(lit.getAtom().getValue());
				const DomainAtom* atom = DomainAtomFactory::instance()->create(s,args);
				InternalArgument ia(atom);
				LuaConnection::convertToLua(_state,ia);
			}
			else {
				lua_pushstring(_state,_translator->printAtom(lit.getAtom().getValue()).c_str());
			}
			lua_setfield(_state,-2,"atom");
			lua_call(_state,2,0);
			lua_pop(_state,1);
		}

		string* index() const { return _registryindex; }
};

int SATTraceWriter::_tracenr = 0;

SATSolver* createsolver(Options* options) {
	MinisatID::SolverOption modes;
	modes.nbmodels = options->nrmodels();
	modes.verbosity = options->satverbosity();
	modes.remap = false;
	return new SATSolver(modes);
}

SATTraceWriter* addTraceMonitor(AbstractGroundTheory* grounding, SATSolver* solver, lua_State* L) {
	SATTraceWriter* trw = new SATTraceWriter(grounding->translator(),L);
	cb::Callback1<void, int> callbackback(trw, &SATTraceWriter::backtrack);
	cb::Callback2<void, MinisatID::Literal, int> callbackprop(trw, &SATTraceWriter::propagate);
	MinisatID::Monitor* m = new MinisatID::Monitor();
	m->setBacktrackCB(callbackback);
	m->setPropagateCB(callbackprop);
	solver->addMonitor(m);
	return trw;
}

MinisatID::Solution* initsolution(Options* options) {
	MinisatID::ModelExpandOptions opts;
	opts.nbmodelstofind = options->nrmodels();
	opts.printmodels = MinisatID::PRINT_NONE;
	opts.savemodels = MinisatID::SAVE_ALL;
	opts.search = MinisatID::MODELEXPAND;
	return new MinisatID::Solution(opts);
}

void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init) {
	for(vector<MinisatID::Literal>::const_iterator literal = model->literalinterpretations.begin();
		literal != model->literalinterpretations.end(); ++literal) {
		int atomnr = literal->getAtom().getValue();
		PFSymbol* symbol = translator->symbol(atomnr);
		if(symbol) {
			const ElementTuple& args = translator->args(atomnr);
			if(typeid(*symbol) == typeid(Predicate)) {
				Predicate* pred = dynamic_cast<Predicate*>(symbol);
				if(literal->hasSign()) init->inter(pred)->makeFalse(args);
				else init->inter(pred)->makeTrue(args);
			}
			else {
				Function* func = dynamic_cast<Function*>(symbol);
				if(literal->hasSign()) init->inter(func)->graphinter()->makeFalse(args);
				else init->inter(func)->graphinter()->makeTrue(args);
			}
		}
	}
}

void addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init) {
cerr << "Adding terms based on var-val pairs from CP solver, pairs are { ";
	for(vector<MinisatID::VariableEqValue>::const_iterator cpvar = model->variableassignments.begin();
			cpvar != model->variableassignments.end(); ++cpvar) {
cerr << cpvar->variable << '=' << cpvar->value;
		Function* function = termtranslator->function(cpvar->variable);
		if(function) {
			const vector<GroundTerm>& gtuple = termtranslator->args(cpvar->variable);
			ElementTuple tuple;
			for(vector<GroundTerm>::const_iterator it = gtuple.begin(); it != gtuple.end(); ++it) {
				if(it->_isvarid) {
					int value = model->variableassignments[it->_varid].value;
					tuple.push_back(DomainElementFactory::instance()->create(value));
				} else {
					tuple.push_back(it->_domelement);
				}
			}
			tuple.push_back(DomainElementFactory::instance()->create(cpvar->value));
cerr << '=' << function->name() << tuple;
			init->inter(function)->graphinter()->makeTrue(tuple);
		}
cerr << ' ';
	}
cerr << '}' << endl;
}

InternalArgument modelexpand(const vector<InternalArgument>& args, lua_State* L) {
	AbstractTheory* theory = args[0].theory();
	AbstractStructure* structure = args[1].structure();
	Options* options = args[2].options();

	// Create solver and grounder
	SATSolver* solver = createsolver(options);
	GrounderFactory grounderfactory(structure,options);
	TopLevelGrounder* grounder = grounderfactory.create(theory,solver);

	// Run grounder
	grounder->run();
	SolverTheory* grounding = dynamic_cast<SolverTheory*>(grounder->grounding());

	// Add information that is abstracted in the grounding
	grounding->addFuncConstraints();
	grounding->addFalseDefineds();

	// Run solver
	MinisatID::Solution* abstractsolutions = initsolution(options);
	SATTraceWriter* trw = 0;
	if(options->trace()) trw = addTraceMonitor(grounding,solver,L);
	solver->solve(abstractsolutions);
	
	// Collect solutions
	vector<AbstractStructure*> solutions;
	for(vector<MinisatID::Model*>::const_iterator model = abstractsolutions->getModels().begin();
		model != abstractsolutions->getModels().end(); ++model) {
		AbstractStructure* newsolution = structure->clone();
		addLiterals(*model,grounding->translator(),newsolution);
		addTerms(*model,grounding->termtranslator(),newsolution);
		newsolution->clean();
		solutions.push_back(newsolution);
	}

	// Convert to internal arguments
	InternalArgument result; 
	result._type = AT_TABLE;
	result._value._table = new vector<InternalArgument>();
	for(vector<AbstractStructure*>::const_iterator it = solutions.begin(); it != solutions.end(); ++it) {
		result._value._table->push_back(InternalArgument(*it));
	}
	if(trw) {
		InternalArgument randt;
		randt._type = AT_MULT;
		randt._value._table = new vector<InternalArgument>(1,result);
		InternalArgument trace;
		trace._type = AT_REGISTRY;
		trace._value._string = trw->index();
		randt._value._table->push_back(trace);
		result = randt;
	}

	// Cleanup
	grounding->recursiveDelete();
	delete(solver);
	delete(abstractsolutions);
	if(trw) delete(trw);
	
	// Return
	return result;
}

AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure, Options* options) {
	GrounderFactory factory(structure,options);
	TopLevelGrounder* grounder = factory.create(theory);
	grounder->run();
	AbstractGroundTheory* grounding = grounder->grounding();
	grounding->addFuncConstraints();
	delete(grounder);
	return grounding;
}

InternalArgument ground(const vector<InternalArgument>& args, lua_State*) {
	AbstractTheory* grounding = ground(args[0].theory(),args[1].structure(),args[2].options());
	InternalArgument result(grounding); 
	return result;
}

InternalArgument completion(const vector<InternalArgument>& args, lua_State* ) {
	AbstractTheory* theory = args[0].theory();
	TheoryUtils::completion(theory);
	return nilarg();
}

InternalArgument tobdd(const vector<InternalArgument>& args, lua_State*) {
	Formula* f = dynamic_cast<Formula*>(args[0]._value._formula);
	FOBDDManager manager;
	FOBDDFactory m(&manager);
	f->accept(&m);
	const FOBDD* bdd = m.bdd();
	stringstream sstr; 
	manager.put(sstr,bdd);
	InternalArgument ia; ia._type = AT_STRING;
	ia._value._string = StringPointer(sstr.str());
	return ia;
}

InternalArgument estimatenrans(const vector<InternalArgument>& args, lua_State* ) {
	Query* q = args[0]._value._query;
	AbstractStructure* structure = args[1].structure();
	FOBDDManager manager;
	FOBDDFactory m(&manager);
	set<Variable*> sv(q->variables().begin(),q->variables().end());
	set<const FOBDDVariable*> svbdd = manager.getVariables(sv);
	set<const FOBDDDeBruijnIndex*> si;
	q->query()->accept(&m);
	const FOBDD* bdd = m.bdd();
	InternalArgument ia; ia._type = AT_DOUBLE;
	ia._value._double = manager.estimatedNrAnswers(bdd,svbdd,si,structure);
	return ia;
}

InternalArgument estimatecost(const vector<InternalArgument>& args, lua_State* ) {
	Query* q = args[0]._value._query;
	AbstractStructure* structure = args[1].structure();
	FOBDDManager manager;
	FOBDDFactory m(&manager);
	set<Variable*> sv(q->variables().begin(),q->variables().end());
	set<const FOBDDVariable*> svbdd = manager.getVariables(sv);
	set<const FOBDDDeBruijnIndex*> si;
	q->query()->accept(&m);
	const FOBDD* bdd = m.bdd();
	InternalArgument ia; ia._type = AT_DOUBLE;
	manager.optimizequery(bdd,svbdd,si,structure);
	ia._value._double = manager.estimatedCostAll(bdd,svbdd,si,structure);
	return ia;
}

InternalArgument query(const vector<InternalArgument>& args, lua_State* ) {
	// FIXME: watch out for partial functions!


	Query* q = args[0]._value._query;
	AbstractStructure* structure = args[1].structure();

	// translate the formula to a bdd
	FOBDDManager manager;
	FOBDDFactory factory(&manager);
	set<Variable*> vars(q->variables().begin(),q->variables().end());
	set<const FOBDDVariable*> bddvars = manager.getVariables(vars);
	set<const FOBDDDeBruijnIndex*> bddindices;
	q->query()->accept(&factory);
	const FOBDD* bdd = factory.bdd();

	// optimize the query
	manager.optimizequery(bdd,bddvars,bddindices,structure);

	// create a generator
	vector<const DomainElement**> genvars;	
	vector<const FOBDDVariable*> vbddvars;
	vector<bool> pattern;
	for(vector<Variable*>::const_iterator it = q->variables().begin(); it != q->variables().end(); ++it) {
		pattern.push_back(false);
		genvars.push_back(new const DomainElement*());
		vbddvars.push_back(manager.getVariable(*it));
	}
	BDDToGenerator btg(&manager);
	InstGenerator* generator = btg.create(bdd,pattern,genvars,vbddvars,structure);
	
	// Create an empty table
	EnumeratedInternalPredTable* interntable = new EnumeratedInternalPredTable();
	vector<SortTable*> vst;
	for(vector<Variable*>::const_iterator it = q->variables().begin(); it != q->variables().end(); ++it) {
		vst.push_back(structure->inter((*it)->sort()));
	}
	Universe univ(vst);
	PredTable* result = new PredTable(interntable,univ);
	
	// execute the query
	ElementTuple currtuple(q->variables().size());
	if(generator->first()) {
		for(unsigned int n = 0; n < q->variables().size(); ++n) {
			currtuple[n] = *(genvars[n]);
		}
		result->add(currtuple);
		while(generator->next()) {
			for(unsigned int n = 0; n < q->variables().size(); ++n) {
				currtuple[n] = *(genvars[n]);
			}
			result->add(currtuple);
		}
	}

	// return the result
	InternalArgument arg;
	arg._type = AT_PREDTABLE;
	arg._value._predtable = result;
	return arg;
}

InternalArgument propagate(const vector<InternalArgument>& args, lua_State* ) {
	AbstractTheory*	theory = args[0].theory();
	AbstractStructure* structure = args[1].structure();
	
	map<PFSymbol*,InitBoundType> mpi;
	Vocabulary* v = theory->vocabulary();
	for(map<string,Predicate*>::const_iterator it = v->firstpred(); it != v->lastpred(); ++it) {
		set<Predicate*> spi = it->second->nonbuiltins();
		for(set<Predicate*>::iterator jt = spi.begin(); jt != spi.end(); ++jt) {
			if(structure->vocabulary()->contains(*jt)) {
				PredInter* pinter = structure->inter(*jt);
				if(pinter->approxtwovalued()) mpi[*jt] = IBT_TWOVAL;
				else {
					// TODO
					mpi[*jt] = IBT_NONE;
				}
			}
			else mpi[*jt] = IBT_NONE;
		}
	}
	for(map<string,Function*>::const_iterator it = v->firstfunc(); it != v->lastfunc(); ++it) {
		set<Function*> sfi = it->second->nonbuiltins();
		for(set<Function*>::iterator jt = sfi.begin(); jt != sfi.end(); ++jt) {
			if(structure->vocabulary()->contains(*jt)) {
				FuncInter* finter = structure->inter(*jt);
				if(finter->approxtwovalued()) mpi[*jt] = IBT_TWOVAL;
				else {
					// TODO
					mpi[*jt] = IBT_NONE;
				}
			}
			else mpi[*jt] = IBT_NONE;
		}
	}

	FOPropBDDDomainFactory* domainfactory = new FOPropBDDDomainFactory();
	FOPropScheduler* scheduler = new FOPropScheduler();
	FOPropagatorFactory propfactory(domainfactory,scheduler,true,mpi,args[2].options());
	FOPropagator* propagator = propfactory.create(theory);
	propagator->run();

	// TODO: free allocated memory
	// TODO: return a structure (instead of nil)
	return nilarg();
}

InternalArgument createtuple(const vector<InternalArgument>& , lua_State*) {
	InternalArgument ia;
	ia._type = AT_TUPLE;
	ia._value._tuple = 0;
	return ia;
}

InternalArgument derefandincrement(const vector<InternalArgument>& args, lua_State* ) {
	TableIterator* it = args[0]._value._tableiterator;
	if(it->hasNext()) {
		ElementTuple* tuple = new ElementTuple(*(*it));
		it->operator++();
		InternalArgument ia; ia._type = AT_TUPLE; ia._value._tuple = tuple;
		return ia;
	}
	else return nilarg();
}

InternalArgument domderefandincrement(const vector<InternalArgument>& args, lua_State* ) {
	SortIterator* it = args[0]._value._sortiterator;
	if(it->hasNext()) {
		const DomainElement* element = *(*it);
		it->operator++();
		InternalArgument ia(element);
		return ia;
	}
	else return nilarg();
}

InternalArgument tableiterator(const vector<InternalArgument>& args, lua_State* ) {
	const PredTable* pt = args[0]._value._predtable;
	TableIterator* tit = new TableIterator(pt->begin());
	InternalArgument ia; ia._type = AT_TABLEITERATOR;
	ia._value._tableiterator = tit;
	return ia;
}

InternalArgument domainiterator(const vector<InternalArgument>& args, lua_State* ) {
	const SortTable* st = args[0]._value._domain;
	SortIterator* it = new SortIterator(st->sortbegin());
	InternalArgument ia; ia._type = AT_DOMAINITERATOR;
	ia._value._sortiterator = it;
	return ia;
}

InternalArgument changevocabulary(const vector<InternalArgument>& args, lua_State* ) {
	AbstractStructure* s = args[0].structure();
	Vocabulary* v = args[1].vocabulary();
	s->vocabulary(v);
	return nilarg();
}

ElementTuple toTuple(vector<InternalArgument>* tab, lua_State* L) {
	ElementTuple tup;
	for(vector<InternalArgument>::const_iterator it = tab->begin(); it != tab->end(); ++it) {
		switch(it->_type) {
			case AT_INT: tup.push_back(DomainElementFactory::instance()->create(it->_value._int)); break;
			case AT_DOUBLE: tup.push_back(DomainElementFactory::instance()->create(it->_value._double)); break;
			case AT_STRING: tup.push_back(DomainElementFactory::instance()->create(it->_value._string)); break;
			case AT_COMPOUND: tup.push_back(DomainElementFactory::instance()->create(it->_value._compound)); break;
			default:
				lua_pushstring(L,"Wrong value in a tuple. Expected an integer, double, string, or compound");
				lua_error(L);
		}
	}
	return tup;
}

InternalArgument maketrue(const vector<InternalArgument>& args, lua_State* ) {
	PredInter* pri = args[0]._value._predinter;
	ElementTuple* tup = args[1]._value._tuple;
	pri->makeTrue(*tup);
	return nilarg();
}

InternalArgument maketabtrue(const vector<InternalArgument>& args, lua_State* L) {
	PredInter* pri = args[0]._value._predinter;
	pri->makeTrue(toTuple(args[1]._value._table,L));
	return nilarg();
}

InternalArgument makefalse(const vector<InternalArgument>& args, lua_State* ) {
	PredInter* pri = args[0]._value._predinter;
	ElementTuple* tup = args[1]._value._tuple;
	pri->makeFalse(*tup);
	return nilarg();
}

InternalArgument maketabfalse(const vector<InternalArgument>& args, lua_State* L) {
	PredInter* pri = args[0]._value._predinter;
	pri->makeFalse(toTuple(args[1]._value._table,L));
	return nilarg();
}

InternalArgument makeunknown(const vector<InternalArgument>& args, lua_State* ) {
	PredInter* pri = args[0]._value._predinter;
	ElementTuple* tup = args[1]._value._tuple;
	pri->makeUnknown(*tup);
	return nilarg();
}

InternalArgument maketabunknown(const vector<InternalArgument>& args, lua_State* L) {
	PredInter* pri = args[0]._value._predinter;
	pri->makeUnknown(toTuple(args[1]._value._table,L));
	return nilarg();
}

InternalArgument clean(const vector<InternalArgument>& args, lua_State*) {
	AbstractStructure* s = args[0].structure();
	s->clean();
	return InternalArgument(s);
}

InternalArgument Options::getvalue(const string& opt) const {
	map<string,bool>::const_iterator bit = _booloptions.find(opt);
	if(bit != _booloptions.end()) {
		return InternalArgument(bit->second);
	}
	map<string,IntOption*>::const_iterator iit = _intoptions.find(opt);
	if(iit != _intoptions.end()) {
		return InternalArgument(iit->second->value());
	}
	map<string,FloatOption*>::const_iterator fit = _floatoptions.find(opt);
	if(fit != _floatoptions.end()) {
		return InternalArgument(fit->second->value());
	}
	map<string,StringOption*>::const_iterator sit = _stringoptions.find(opt);
	if(sit != _stringoptions.end()) {
		return InternalArgument(StringPointer(sit->second->value()));
	}
	
	InternalArgument ia; ia._type = AT_NIL;
	return ia;
}
