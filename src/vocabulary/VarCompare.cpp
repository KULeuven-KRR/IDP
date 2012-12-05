#include "VarCompare.hpp"
#include "vocabulary.hpp"

using namespace std;

bool VarCompare::operator()(const Variable* lhs, const Variable* rhs) const {
	if(lhs==NULL){
		return true;
	}
	if(rhs==NULL){
		return false;
	}
	if(lhs->name()<rhs->name()){
		return true;
	}else if(lhs->name()>rhs->name()){
		return false;
	}else{
		return lhs<rhs;
	}
}
