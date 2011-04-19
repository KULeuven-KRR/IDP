/************************************
	builtin.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef BUILTIN_HPP
#define BUILTIN_HPP

#include "vocabulary.hpp"

/* 
 * The standard builtin vocabulary
 *		sorts:			nat, int, float, char, string
 *		predicates:		=/2, </2, >/2, SUCC/2, 
 *		functions:		+/2, -/2, * /2, //2, ^/2, %/2, abs/1, -/1, MIN/0, MAX/0
 */
class StdBuiltin : public Vocabulary {
	private:
		static StdBuiltin* _instance;	
		StdBuiltin();
	public:
		static StdBuiltin* instance();
		~StdBuiltin() { }

		Sort* natsort() 	const { return *(sort("nat")->begin()); 	}
		Sort* intsort() 	const { return *(sort("int")->begin()); 	}
		Sort* floatsort()	const { return *(sort("float")->begin()); 	}
		Sort* charsort() 	const { return *(sort("char")->begin()); 	}
		Sort* stringsort() 	const { return *(sort("string")->begin()); 	}
};

#endif
