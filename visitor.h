/************************************
	visitor.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef VISITOR_H
#define VISITOR_H

#include "theory.h"

class Visitor {
	
	public:

		Visitor() { }
		virtual ~Visitor() { }

		void traverse(Formula* f);
		void traverse(Term* t);
		void traverse(Rule* r);
		void traverse(Definition* d);
		void traverse(FixpDef* f);
		void traverse(SetExpr* s);

		// Formulas 
		virtual void visit(PredForm* a)			{ traverse(a);	}
		virtual void visit(EqChainForm* a)		{ traverse(a);	}
		virtual void visit(EquivForm* a)		{ traverse(a);	}
		virtual void visit(BoolForm* a)			{ traverse(a);	}
		virtual void visit(QuantForm* a)		{ traverse(a);	}

		// Definitions 
		virtual void visit(Rule* a)				{ traverse(a);	}
		virtual void visit(Definition* a)		{ traverse(a);	}
		virtual void visit(FixpDef* a)			{ traverse(a);	}

		// Terms
		virtual void visit(VarTerm* a)			{ traverse(a);	}
		virtual void visit(FuncTerm* a)			{ traverse(a);	}
		virtual void visit(DomainTerm* a)		{ traverse(a);	}
		virtual void visit(AggTerm* a)			{ traverse(a);	}

		// Set expressions
		virtual void visit(EnumSetExpr* a)		{ traverse(a);	}
		virtual void visit(QuantSetExpr* a)		{ traverse(a);	}
};

#endif
