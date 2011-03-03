#ifndef INSTGENERATOR_H
#define INSTGENERATOR_H

#include "structure.hpp"
#include "checker.hpp"

class InstGenerator {
	public:
		InstGenerator() { }
		virtual bool first() const = 0;
		virtual bool next() const = 0; 
};

class TableInstGenerator : public InstGenerator { 
	private:
		PredTable*				_table;
		vector<domelement*>		_outvars;
		mutable unsigned int	_currpos;
	public:
		TableInstGenerator(PredTable* t, const vector<domelement*>& out) : _table(t), _outvars(out) { }
		bool first() const;
		bool next() const;
};

class SortInstGenerator : public InstGenerator { 
	private:
		SortTable*				_table;
		domelement*				_outvar;
		mutable unsigned int	_currpos;
	public:
		SortInstGenerator(SortTable* t, domelement* out) : _table(t), _outvar(out) { }
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
		InstanceChecker*				_checker;
		vector<domelement*>				_outvars;
		vector<SortTable*>				_tables;
		mutable vector<unsigned int>	_currpositions;
		mutable vector<domelement>		_currargs;
		GeneratorNode*					_left;
		GeneratorNode*					_right;
	
	public:
		// Constructor
		TwoChildGeneratorNode(InstanceChecker* t, const vector<domelement*>& ov, const vector<SortTable*>& tbs, GeneratorNode* l, GeneratorNode* r) :
			GeneratorNode(), _checker(t), _outvars(ov), _tables(tbs), _currpositions(tbs.size()), _currargs(_outvars.size()), _left(l), _right(r) 
			{ _left->parent(this); _right->parent(this); }

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

class GeneratorFactory {
	public:
		InstGenerator*	create(const vector<domelement*>&, const vector<SortTable*>&);
};

#endif
