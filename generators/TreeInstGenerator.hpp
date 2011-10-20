/************************************
	TreeInstGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TREEINSTGENERATOR_HPP_
#define TREEINSTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class GeneratorNode;

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

class GeneratorNode {
protected:
	GeneratorNode*	_parent;

public:
	GeneratorNode() : _parent(0) { }
	virtual ~GeneratorNode() {}

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
		InstGenerator*		_checker;
		InstGenerator*		_generator;
		GeneratorNode*		_left;
		GeneratorNode*		_right;

	public:
		// Constructor
		TwoChildGeneratorNode(InstGenerator* c, InstGenerator* g, GeneratorNode* l, GeneratorNode* r);

		// Generate instances
		GeneratorNode*	first()	const;
		GeneratorNode*	next()	const;
};

#endif /* TREEINSTGENERATOR_HPP_ */
