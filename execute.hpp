/************************************
	execute.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EXECUTE_HPP
#define EXECUTE_HPP

#include "namespace.hpp"
#include <sstream>

/*******************************************
	Argument types for inference methods
*******************************************/

enum InfArgType { 
	IAT_VOID, IAT_THEORY, IAT_STRUCTURE, IAT_VOCABULARY, IAT_NAMESPACE, IAT_OPTIONS, IAT_ERROR,
	IAT_NIL, IAT_NUMBER, IAT_BOOLEAN, IAT_STRING, IAT_TABLE, IAT_FUNCTION, IAT_USERDATA, IAT_THREAD, IAT_LIGHTUSERDATA,
	IAT_SET_OF_STRUCTURES
};


namespace BuiltinProcs {
	void initialize();
}

/** Lua procedures **/

class LuaProcedure {
	private:
		string				_name;
		ParseInfo			_pi;
		vector<string>		_innames;
		stringstream		_code;
	public:
		LuaProcedure() { }
		LuaProcedure(const string& name, const ParseInfo& pi) :
			_name(name), _pi(pi), _innames(0) { }

		// Mutators
		void addarg(const string& name) { _innames.push_back(name);	}
		void add(char* s) { _code << s;	}
		void add(const string& s)	{ _code << s;	}
		
		// Inspectors
		const ParseInfo&	pi()	const { return _pi;		}
		const string&		name()	const { return _name;	}
		unsigned int		arity()	const { return _innames.size();	}
		string				code()	const { return _code.str();		}

};

/** Possible argument or return value of an execute statement **/
union InfArg {
	Vocabulary*					_vocabulary;
	AbstractStructure*			_structure;
	AbstractTheory*				_theory;
	Namespace*					_namespace;
	double						_number;
	bool						_boolean;
	string*						_string;
	InfOptions*					_options;
	vector<AbstractStructure*>*	_setofstructures;
};

/** An execute statement **/
class Inference {
	protected:
		vector<InfArgType>	_intypes;		// types of the input arguments
		InfArgType			_outtype;		// type of the return value
		string				_description;	// description of the inference
		bool				_reload;		// true if after completing the inference,
											// new components are added to the global namespace
	public:
		virtual ~Inference() { }
		virtual InfArg execute(const vector<InfArg>& args) const = 0;	// execute the statement
		const vector<InfArgType>&	intypes()		const { return _intypes;		}
		InfArgType					outtype()		const { return _outtype;		}
		unsigned int				arity()			const { return _intypes.size();	}
		string						description()	const { return _description;	}
		bool						reload()		const { return _reload;			}
};

class LoadFile : public Inference {
	public:
		LoadFile() { _intypes = vector<InfArgType>(1,IAT_STRING);	
					 _outtype = IAT_VOID;
					 _description = "Load the given file";
					 _reload = true;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class SetOption : public Inference {
	public:
		SetOption(InfArgType t);
		InfArg execute(const vector<InfArg>& args) const;
};

class GetOption : public Inference {
	public:
		GetOption();
		InfArg execute(const vector<InfArg>& args) const;
};

class NewOption : public Inference {
	public:
		NewOption(bool b) {	
			_intypes = b ? vector<InfArgType>(1,IAT_OPTIONS) : vector<InfArgType>(0);
			_outtype = IAT_OPTIONS;
			_description = "Create a clone of the given options";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class PrintTheory : public Inference {
	public:
		PrintTheory(bool opts) { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			if(opts) _intypes.push_back(IAT_OPTIONS);
			_outtype = IAT_STRING;
			_description = "Print the theory";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class PrintVocabulary : public Inference {
	public:
		PrintVocabulary(bool opts) { 
			_intypes = vector<InfArgType>(1,IAT_VOCABULARY); 
			if(opts) _intypes.push_back(IAT_OPTIONS);
			_outtype = IAT_STRING;	
			_description = "Print the vocabulary";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class PrintStructure : public Inference {
	public:
		PrintStructure(bool opts) { 
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE); 
			if(opts) _intypes.push_back(IAT_OPTIONS);
			_outtype = IAT_STRING;	
			_description = "Print the structure";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class PrintNamespace : public Inference {
	public:
		PrintNamespace(bool opts) { 
			_intypes = vector<InfArgType>(1,IAT_NAMESPACE); 
			if(opts) _intypes.push_back(IAT_OPTIONS);
			_outtype = IAT_STRING;	
			_description = "Print the namespace";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class PushNegations : public Inference {
	public:
		PushNegations() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Push all negations inside until they are in front of atoms";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class RemoveEquivalences : public Inference {
	public:
		RemoveEquivalences() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Rewrite equivalences into pairs of implications";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class RemoveEqchains : public Inference {
	public:
		RemoveEqchains() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Rewrite chains of (in)equalities to conjunctions of atoms";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;

};

class FlattenFormulas : public Inference {
	public:
		FlattenFormulas() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Rewrite ((A & B) & C) to (A & B & C), rewrite (! x : ! y : phi(x,y)) to (! x y : phi(x,y)), etc.";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class GroundingInference : public Inference {
	public:
		GroundingInference();
		InfArg execute(const vector<InfArg>& args) const;
};

class GroundingWithResult : public Inference {
	public:
		GroundingWithResult();
		InfArg execute(const vector<InfArg>& args) const;
};

class FastGrounding : public Inference {
	public:
		FastGrounding();
		InfArg execute(const vector<InfArg>& args) const;
};

class ModelExpansionInference : public Inference {
	public:
		ModelExpansionInference(bool opts);
		InfArg execute(const vector<InfArg>& args) const;
};

class FastMXInference : public Inference {
	public:
		FastMXInference(bool opts);
		InfArg execute(const vector<InfArg>& args) const;
};

class StructToTheory : public Inference {
	public:
		StructToTheory() {
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE); 
			_outtype = IAT_THEORY;	
			_description = "Rewrite a structure to a theory";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class MoveQuantifiers : public Inference {
	public:
		MoveQuantifiers() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_VOID;
			_description = "Move universal (existential) quantifiers inside conjunctions (disjunctions)";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class ApplyTseitin : public Inference {
	public:
		ApplyTseitin() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_VOID;
			_description = "Apply the tseitin transformation to a theory";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class GroundReduce : public Inference {
	public:
		GroundReduce();
		InfArg execute(const vector<InfArg>& args) const;
};

class MoveFunctions : public Inference {
	public:
		MoveFunctions() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_VOID;
			_description = "Move functions until no functions are nested";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class CloneStructure : public Inference {
	public:
		CloneStructure() { 
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE);
			_outtype = IAT_STRUCTURE;
			_description = "Make a copy of this structure";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class CloneTheory : public Inference {
	public:
		CloneTheory() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_THEORY;
			_description = "Make a copy of this theory";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class ForceTwoValued : public Inference {
	public:
		ForceTwoValued() {
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE);
			_outtype = IAT_STRUCTURE;
			_description = "Force structure to be two-valued";
			_reload = false;
		}
		InfArg execute(const vector<InfArg>& args) const;
};

#endif
