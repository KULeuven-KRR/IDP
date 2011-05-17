/************************************
	generator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INSTGENERATOR_HPP
#define INSTGENERATOR_HPP

#include <vector>

class PredTable;
class PredInter;
class SortTable;
class DomainElement;
class InstanceChecker;

class InstGenerator {
	public:
		InstGenerator() { }
		virtual bool first() const = 0;
		virtual bool next() const = 0; 
};

class TableInstGenerator : public InstGenerator { 
	private:
		PredTable*							_table;
		std::vector<const DomainElement**>	_outvars;
		mutable TableIterator				_currpos;
	public:
		TableInstGenerator(PredTable* t, const std::vector<const DomainElement**>& out) : 
			_table(t), _outvars(out), _currpos(t->begin()) { }
		bool first() const;
		bool next() const;
};

class SimpleLookupGenerator : public InstGenerator {
	private:
		PredTable*									_table;
		std::vector<const DomainElement**>			_invars;
		mutable std::vector<const DomainElement*>	_currargs;
	public:
		SimpleLookupGenerator(PredTable* t, const std::vector<const DomainElement**> in) :
			_table(t), _invars(in), _currargs(in.size()) { }
		bool first()	const;
		bool next()		const { return false;	}
};

class StrictWeakTupleOrdering;
typedef std::map<std::vector<const DomainElement*>,std::vector<std::vector<const DomainElement*> >,StrictWeakTupleOrdering>
	LookupTable;

class EnumLookupGenerator : public InstGenerator {
	private:
		const LookupTable&							_table;
		std::vector<const DomainElement**>			_invars;
		std::vector<const DomainElement**>			_outvars;
		mutable std::vector<const DomainElement*>	_currargs;
		mutable LookupTable::const_iterator										_currpos;
		mutable std::vector<std::vector<const DomainElement*> >::const_iterator	_iter;
	public:
		EnumLookupGenerator(const LookupTable&, const std::vector<const DomainElement**> in, const std::vector<const DomainElement**> out);
		bool first()	const;
		bool next()		const;
};

class SortInstGenerator : public InstGenerator { 
	private:
		SortTable*				_table;
		const DomainElement**	_outvar;
		mutable SortIterator	_currpos;
	public:
		SortInstGenerator(SortTable* t, const DomainElement** out) : 
			_table(t), _outvar(out), _currpos(t->sortbegin()) { }
		bool first() const;
		bool next() const;
};


/********************************************
	Tree structure for generating instances
********************************************/

class GeneratorNode {
	protected:
		GeneratorNode*	_parent;

	public:
		// Constructors
		GeneratorNode() : _parent(0) { }

		// Mutators
		void	parent(GeneratorNode* n) { _parent = n;	}

		// Inspectors
		GeneratorNode*	parent()	const { return _parent;	}

		// Generate instances
		virtual	GeneratorNode*	first()	const = 0;
		virtual	GeneratorNode*	next()	const = 0;
};

class LeafGeneratorNode : public GeneratorNode {
	private:
		InstGenerator*	_generator;
		GeneratorNode*	_this;		//	equal to 'this'
	
	public:
		// Constructor
		LeafGeneratorNode(InstGenerator* gt) : GeneratorNode(), _generator(gt) { _this = this;	}

		// Generate instances
		GeneratorNode*	first()	const;
		GeneratorNode*	next()	const;
};

class OneChildGeneratorNode : public GeneratorNode {
	private:
		InstGenerator*	_generator;
		GeneratorNode*	_child;
	
	public:
		// Constructor
		OneChildGeneratorNode(InstGenerator* gt, GeneratorNode* c) : GeneratorNode(), _generator(gt), _child(c) { _child->parent(this);	}

		// Generate instances
		GeneratorNode*	first()	const;
		GeneratorNode*	next()	const;
};

class TwoChildGeneratorNode : public GeneratorNode {
	private:
		InstanceChecker*							_checker;
		std::vector<const DomainElement**>			_outvars;
		InstGenerator*								_currposition;
		mutable std::vector<const DomainElement*>	_currargs;
		GeneratorNode*								_left;
		GeneratorNode*								_right;
	
	public:
		// Constructor
		TwoChildGeneratorNode(InstanceChecker* t, const std::vector<const DomainElement**>& ov, const std::vector<SortTable*>& tbs, GeneratorNode* l, GeneratorNode* r);

		// Generate instances
		GeneratorNode*	first()	const;
		GeneratorNode*	next()	const;
};


/***************************************
	Root node for generating instances
***************************************/

class TreeInstGenerator : public InstGenerator {
	private:
				GeneratorNode*	_root;
		mutable	GeneratorNode*	_curr;	// Remember the last position of a match

	public:
		// Constructor
		TreeInstGenerator(GeneratorNode* r) : _root(r), _curr(0) { }

		// Generate instances
		bool	first()	const;
		bool	next()	const;
};

/**************
	Factory
**************/

class EnumeratedInternalPredTable;

class GeneratorFactory {
	private:
		void visit(EnumeratedInternalPredTable*	);
	public:
		InstGenerator*	create(const std::vector<const DomainElement**>&, const std::vector<SortTable*>&);
		InstGenerator*	create(PredTable*, std::vector<bool> pattern, const std::vector<const DomainElement**>&);
};

#endif
