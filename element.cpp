/************************************
	element.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "element.hpp"
#include "vocabulary.hpp"
#include "common.hpp"

#include <cassert>

using namespace std;

/**********************
	Domain elements
**********************/

string compound::to_string() const {
	if(_function) {
		string s = _function->to_string();
		if(!_args.empty()) {
			s = s + '(' + ElementUtil::ElementToString(_args[0]);
			for(unsigned int n = 1; n < _args.size(); ++n) {
				s = s + ',' + ElementUtil::ElementToString(_args[n]);
			}
			s = s + ')';
		}
		return s;
	}
	else {
		assert(_args.size() == 1);
		return ElementUtil::ElementToString(_args[0]);
	}
}

namespace ElementUtil {

	ElementType resolve(ElementType t1, ElementType t2) {
		return (t1 < t2) ? t2 : t1;
	}

	ElementType leasttype() { return ELINT;	}

	ElementType reduce(Element e, ElementType t) {
		switch(t) {
			case ELINT: 
				break;
			case ELDOUBLE:
				if(double(int(e._double)) == e._double) return ELINT;
				break;
			case ELSTRING:
				if(isInt(*(e._string))) return ELINT;
				else if(isDouble(*(e._string))) return ELDOUBLE;
				break;
			case ELCOMPOUND:
				if(!(e._compound->_function)) return reduce((e._compound->_args)[0]._element,(e._compound->_args)[0]._type);
				break;
			default:
				assert(false);
		}
		return t;
	}

	ElementType reduce(TypedElement te) {
		return reduce(te._element,te._type);
	}

	string ElementToString(Element e, ElementType t) {
		switch(t) {
			case ELINT:
				return itos(e._int);
			case ELDOUBLE:
				return dtos(e._double);
			case ELSTRING:
				return *(e._string);
			case ELCOMPOUND:
				return e._compound->to_string();
			default:
				assert(false); return "???";
		}
	}

	string ElementToString(TypedElement e) {
		return ElementToString(e._element,e._type);
	}

	Element nonexist(ElementType t) {
		Element e;
		switch(t) {
			case ELINT:
				e._int = MAX_INT;
				break;
			case ELDOUBLE:
				e._double = MAX_DOUBLE;
				break;
			case ELSTRING:
				e._string = 0;
				break;
			case ELCOMPOUND:
				e._compound = 0;
				break;
			default:
				assert(false);
		}
		return e;
	}

	bool exists(Element e, ElementType t) {
		switch(t) {
			case ELINT:
				return e._int != MAX_INT;
			case ELDOUBLE:
				return e._double != MAX_DOUBLE;
			case ELSTRING:
				return e._string != 0;
			case ELCOMPOUND:
				return e._compound != 0;
			default:
				assert(false); return false;
		}
	}
	
	bool exists(TypedElement e) {
		return exists(e._element,e._type);
	}

	Element convert(Element e, ElementType oldtype, ElementType newtype) {
		if(oldtype == newtype) return e;
		Element ne;
		switch(oldtype) {
			case ELINT:
				if(newtype == ELSTRING) {
					ne._string = IDPointer(itos(e._int));
				}
				else if(newtype == ELDOUBLE) {
					ne._double = double(e._int);
				}
				else {
					assert(newtype == ELCOMPOUND);
					TypedElement te(e,oldtype);
					ne._compound = CPPointer(te);
				}
				break;
			case ELDOUBLE:
				if(newtype == ELINT) {
					if(isInt(e._double)) {
						ne._int = int(e._double);
					}
					else return nonexist(newtype);
				}
				else if(newtype == ELSTRING) {
					ne._string = IDPointer(dtos(e._double));
				}
				else {
					assert(newtype == ELCOMPOUND);
					TypedElement te(e,oldtype);
					ne._compound = CPPointer(te);
				}
				break;
			case ELSTRING:
				if(newtype == ELINT) {
					if(isInt(*(e._string))) {
						ne._int = stoi(*(e._string));
					}
					else return nonexist(newtype);
				}
				else if(newtype == ELDOUBLE) {
					if(isDouble(*(e._string))) {
						ne._double = stod(*(e._string));
					}
					else return nonexist(newtype);
				}
				else {
					assert(newtype == ELCOMPOUND);
					TypedElement te(e,oldtype);
					ne._compound = CPPointer(te);
				}
				break;
			case ELCOMPOUND:
				if(e._compound->_function == 0) 
					return convert((e._compound->_args)[0],newtype);
				else return nonexist(newtype);
				break;
			default:
				assert(false);
		}
		return ne;
	}

	Element	convert(TypedElement te, ElementType t) {
		return convert(te._element,te._type,t);
	}

	vector<TypedElement> convert(const vector<domelement>& vd) {
		vector<TypedElement> vte(vd.size());
		for(unsigned int n = 0; n < vd.size(); ++n) {
			if(vd[n]->_function) {
				vte[n]._type = ELCOMPOUND;
				vte[n]._element._compound = vd[n];
			}
			else vte[n] = (vd[n]->_args)[0];
		}
		return vte;
	}

	inline bool equal(Element e1, ElementType t1, Element e2, ElementType t2) {
		switch(t1) {
			case ELINT:
				switch(t2) {
					case ELINT: return e1._int == e2._int;
					case ELDOUBLE: return double(e1._int) == e2._double;
					case ELSTRING: return (isInt(*(e2._string)) && e1._int == stoi(*(e2._string)));
					case ELCOMPOUND: return ((e2._compound)->_function == 0 && 
											  equal(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELDOUBLE:
				switch(t2) {
					case ELINT: return e1._double == double(e2._int);
					case ELDOUBLE: return e1._double == e2._double;
					case ELSTRING: return (isDouble(*(e2._string)) && e1._double == stod(*(e2._string)));
					case ELCOMPOUND: return ((e2._compound)->_function == 0 && 
											  equal(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELSTRING:
				switch(t2) {
					case ELINT: return (isInt(*(e1._string)) && e2._int == stoi(*(e1._string)));
					case ELDOUBLE: return (isDouble(*(e1._string)) && e2._double == stod(*(e1._string)));
					case ELSTRING: return e1._string == e2._string;
					case ELCOMPOUND: return ((e2._compound)->_function == 0 && 
											  equal(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELCOMPOUND:
				switch(t2) {
					case ELINT: return ((e1._compound)->_function == 0 && 
											  equal(e2,t2,((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type));
					case ELDOUBLE: return ((e1._compound)->_function == 0 && 
											  equal(e2,t2,((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type));
					case ELSTRING: return ((e1._compound)->_function == 0 && 
											  equal(e2,t2,((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type));
					case ELCOMPOUND: 
						if((e1._compound)->_function != (e2._compound)->_function) return false;
						else {
							for(unsigned int n = 0; n < (e1._compound)->_function->arity(); ++n) {
								if(!(((e1._compound)->_args)[n] ==  ((e2._compound)->_args)[n])) return false;
							}
							return true;
						}
					default: assert(false); return false;
				}
			default:
				assert(false); return false;
		}
	}

	inline bool strlessthan(Element e1, ElementType t1, Element e2, ElementType t2) {
		switch(t1) {
			case ELINT:
				switch(t2) {
					case ELINT: return e1._int < e2._int;
					case ELDOUBLE: return double(e1._int) < e2._double;
					case ELSTRING: return ((!isDouble(*(e2._string))) || double(e1._int) < stod(*(e2._string)));
					case ELCOMPOUND: return ((e2._compound)->_function != 0 || 
											  strlessthan(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELDOUBLE:
				switch(t2) {
					case ELINT: return e1._double < double(e2._int);
					case ELDOUBLE: return e1._double < e2._double;
					case ELSTRING: return ((!isDouble(*(e2._string))) || e1._double < stod(*(e2._string)));
					case ELCOMPOUND: return ((e2._compound)->_function != 0 || 
											  strlessthan(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELSTRING:
				switch(t2) {
					case ELINT: return (isDouble(*(e1._string)) && stod(*(e1._string)) < double(e2._int));
					case ELDOUBLE: return (isDouble(*(e1._string)) && stod(*(e1._string)) < e2._double);
					case ELSTRING: {
						if(isDouble(*(e1._string))) {
							if(isDouble(*(e2._string))) return stod(*(e1._string)) < stod(*(e2._string));
							else return true;
						}
						else if(isDouble(*(e2._string))) return false;
						else return e1._string < e2._string;
					}
					case ELCOMPOUND: return ((e2._compound)->_function != 0 || 
											  strlessthan(e1,t1,((e2._compound)->_args)[0]._element,((e2._compound)->_args)[0]._type));
					default: assert(false); return false;
				}
			case ELCOMPOUND:
				switch(t2) {
					case ELINT: return ((e1._compound)->_function == 0 && 
											  strlessthan(((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type,e2,t2));
					case ELDOUBLE: return ((e1._compound)->_function == 0 && 
											  strlessthan(((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type,e2,t2));
					case ELSTRING: return ((e1._compound)->_function == 0 && 
											  strlessthan(((e1._compound)->_args)[0]._element,((e1._compound)->_args)[0]._type,e2,t2));
					case ELCOMPOUND: 
						if((e1._compound)->_function == 0) {
							if((e2._compound)->_function == 0) return ((e1._compound)->_args)[0] < ((e2._compound)->_args)[0];
							else return true;
						}
						else if((e2._compound)->_function == 0) return false;
						else if((e1._compound)->_function < (e2._compound)->_function) return true;
						else if((e1._compound)->_function > (e2._compound)->_function) return false;
						else {
							for(unsigned int n = 0; n < (e1._compound)->_function->arity(); ++n) {
								if((((e1._compound)->_args)[n] <  ((e2._compound)->_args)[n])) return true;
								if((((e2._compound)->_args)[n] <  ((e1._compound)->_args)[n])) return false;
							}
							return false;
						}
					default: assert(false); return false;
				}
			default:
				assert(false); return false;
		}
	}


	inline bool lessthanorequal(Element e1, ElementType t1, Element e2, ElementType t2) {
		return (strlessthan(e1,t1,e2,t2) || equal(e1,t1,e2,t2));
	}

	bool equal(const vector<TypedElement>& vte, const vector<Element>& ve, const vector<ElementType>& vt) {
		for(unsigned int n = 0; n < vte.size(); ++n) {
			if(!(equal(vte[n]._element,vte[n]._type,ve[n],vt[n]))) return false;
		}
		return true;
	}
}

bool operator==(TypedElement e1, TypedElement e2)	{ return ElementUtil::equal(e1._element,e1._type,e2._element,e2._type);				}
bool operator!=(TypedElement e1, TypedElement e2)	{ return !(e1==e2);	}
bool operator<=(TypedElement e1, TypedElement e2)	{ return ElementUtil::lessthanorequal(e1._element,e1._type,e2._element,e2._type);	}
bool operator<(TypedElement e1, TypedElement e2)	{ return ElementUtil::strlessthan(e1._element,e1._type,e2._element,e2._type);		}

