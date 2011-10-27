/************************************
	EqualGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EQUALGENERATOR_HPP_
#define EQUALGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

/**
 * Given two variables x and y, generate all tuples such that x=y
 * TODO loop over the smallest of both?
 * TODO eliminate equalinternatiterator (same performance as this)
 */
class EqualGenerator : public InstGenerator {
private:
	const DomElemContainer *left, *right;
	SortTable				*leftdom, *rightdom;
	SortIterator			_leftcurr;
public:
	EqualGenerator(const DomElemContainer* left, const DomElemContainer* right, SortTable* leftdom, SortTable* rightdom) :
		left(left), right(right), leftdom(leftdom), rightdom(rightdom) { }

	void reset(){
		_leftcurr = leftdom->sortBegin();

		while(not _leftcurr.isAtEnd()){
			auto domelem = *_leftcurr;
			++_leftcurr;
			if(rightdom->contains(domelem)){
				return;
			}
		}
		if(_leftcurr.isAtEnd()){
			notifyAtEnd();
		}
	}

	void next(){
		while(not _leftcurr.isAtEnd()){
			auto domelem = *_leftcurr;
			++_leftcurr;
			if(rightdom->contains(domelem)){
				*left = domelem;
				*right = domelem;
				break;
			}
		}
		if(_leftcurr.isAtEnd()){
			notifyAtEnd();
		}
	}
};

#endif /* EQUALGENERATOR_HPP_ */
