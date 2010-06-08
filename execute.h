/************************************
	execute.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EXECUTE_H
#define EXECUTE_H

#include "namespace.h"

enum InfArgType { IAT_VOID, IAT_THEORY, IAT_STRUCTURE, IAT_VOCABULARY, IAT_NAMESPACE };

union InfArg {
	Vocabulary*	_vocabulary;
	Structure*	_structure;
	Theory*		_theory;
	Namespace*	_namespace;
};

class Inference {
	protected:
		vector<InfArgType>	_intypes;
		InfArgType			_outtype;
	public:
		virtual ~Inference() { }
		virtual void execute(const vector<InfArg>& args, InfArg res) const = 0;
		const vector<InfArgType>&	intypes()	const { return _intypes;		}
		InfArgType					outtype()	const { return _outtype;		}
		unsigned int				arity()		const { return _intypes.size();	}
};

class PrintTheory : public Inference {
	public:
		PrintTheory() { _intypes = vector<InfArgType>(1,IAT_THEORY); _outtype = IAT_VOID;	}
		void execute(const vector<InfArg>& args, InfArg res) const;
};

class PrintVocabulary : public Inference {
	public:
		PrintVocabulary() { _intypes = vector<InfArgType>(1,IAT_VOCABULARY); _outtype = IAT_VOID;	}
		void execute(const vector<InfArg>& args, InfArg res) const;
};

class PrintStructure : public Inference {
	public:
		PrintStructure() { _intypes = vector<InfArgType>(1,IAT_STRUCTURE); _outtype = IAT_VOID;	}
		void execute(const vector<InfArg>& args, InfArg res) const;
};

class PrintNamespace : public Inference {
	public:
		PrintNamespace() { _intypes = vector<InfArgType>(1,IAT_NAMESPACE); _outtype = IAT_VOID;	}
		void execute(const vector<InfArg>& args, InfArg res) const;
};

class PushNegations : public Inference {
	public:
		PushNegations() { _intypes = vector<InfArgType>(1,IAT_THEORY); _outtype = IAT_VOID;	}
		void execute(const vector<InfArg>& args, InfArg res) const;
};

#endif
