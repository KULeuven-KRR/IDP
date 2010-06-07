/************************************
	execute.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "execute.h"
#include <iostream>

namespace Execute {

	void print(Theory* t) {
		// TODO
		string s = t->to_string();
		cout << s;
	}

	void print(Vocabulary* v) {
		// TODO
		string s = v->to_string();
		cout << s;
	}

	void print(Structure* i) {
		// TODO
		string s = i->to_string();
		cout << s;
	}

}
