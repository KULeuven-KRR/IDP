#ifndef GTGenerator_HPP_
#define GTGenerator_HPP_

#include "common.hpp"
#include "generators/InstGenerator.hpp"
#include "structure.hpp"

class SortTable;
class DomElemContainer;

enum class Input {
	BOTH, NONE, LEFT, RIGHT
};

/**
 * Formula: x op y, with op one of {=<, <, =>, >, =}, possibly different domains
 *
 * TODO can halve runtime in the case of NONE input and of enumerating according to < order (by storing the previous first match).
 */
class ComparisonGenerator: public InstGenerator {
private:
	SortTable *_leftsort, *_rightsort;
	const DomElemContainer *_leftvar, *_rightvar;
	CompType comparison;
	Input _input; // NOTE: is never RIGHT after initialization
	SortIterator _left, _right;

	enum class CompResult {
		VALID, INVALID
	};

	bool _reset, increaseouter;

public:
	ComparisonGenerator(SortTable* leftsort, SortTable* rightsort, const DomElemContainer* leftvalue, const DomElemContainer* rightvalue, Input input,
			CompType type) :
			_leftsort(leftsort), _rightsort(rightsort), _leftvar(leftvalue), _rightvar(rightvalue), comparison(type), _input(input), _left(
					leftsort->sortBegin()), _right(rightsort->sortBegin()), _reset(true), increaseouter(false) {
		if (_input == Input::RIGHT) {
			_leftsort = rightsort;
			_rightsort = leftsort;
			_leftvar = rightvalue;
			_rightvar = leftvalue;
			_input = Input::LEFT;
			comparison = invertComp(comparison);
		}
	}

	virtual void put(std::ostream& stream){
		stream <<_leftvar <<"[" <<toString(_leftsort) <<"]" <<(_input!=Input::NONE?"(in)":"(out)");
		stream <<toString(comparison);
		stream <<_rightvar <<"[" <<toString(_rightsort) <<"]" <<(_input==Input::BOTH?"(in)":"(out)");
	}

	ComparisonGenerator* clone() const{
		return new ComparisonGenerator(*this);
	}

	void reset() {
		_reset = true;
		increaseouter = false;
	}

	/**
	 * The first argument is the finite one of such is available.
	 * NOTE: optimized for EQ comparison
	 */
	void findNext(SortIterator* finiteside, SortIterator* undefinedside, SortTable* undefinedSort, const DomElemContainer* finiteContainer, const DomElemContainer* undefinedContainer){
		bool stop = false;
		for(; not finiteside->isAtEnd() && not stop; ++(*finiteside)){
			if(comparison!=CompType::EQ){
				for (; not (*undefinedside).isAtEnd() && not stop; ++(*undefinedside)) {
					if (checkAndSet() == CompResult::VALID) {
						stop = true;
						break; // NOTE: essential to prevent ++
					}
				}
				if (stop) {
					break;
				}
				*undefinedside = undefinedSort->sortBegin();
			}else{
				if(undefinedSort->contains(**finiteside)){
					finiteContainer->operator =(**finiteside);
					undefinedContainer->operator =(**finiteside);
					increaseouter = true;
					return;
				}
			}
		}
		if ((*finiteside).isAtEnd()) {
			notifyAtEnd();
		}
	}

	void next() {
		switch (_input) {
		case Input::BOTH:
			if(_reset && correct()){
				_reset = false;
			}else{
				notifyAtEnd();
			}
			break;
		case Input::NONE:{
			if(_reset){
				_reset = false;
				_left = _leftsort->sortBegin();
				_right = _rightsort->sortBegin();
			}else{
				if(_leftsort->finite()){
					if(increaseouter){
						++_left;
						increaseouter = false;
					}else{
						++_right;
					}
				}else{
					if(increaseouter){
						++_right;
						increaseouter = false;
					}else{
						++_left;
					}
				}
			}
			if(_leftsort->finite()){ // NOTE: optimized for looping over non-finite sort first (if available)
				findNext(&_left, &_right, _rightsort, _leftvar, _rightvar);
			}else{
				findNext(&_right, &_left, _leftsort, _rightvar, _leftvar);
			}
			break;}
		case Input::LEFT: // NOTE: optimized EQ comparison
			if(_reset){
				_reset = false;
				if(comparison==CompType::EQ){
					if(_rightsort->contains(_leftvar->get())){
						_rightvar->operator =(_leftvar->get());
					}else{
						notifyAtEnd();
					}
					return;
				}else{
					_right = _rightsort->sortBegin();
				}
			}else{
				if(comparison==CompType::EQ){
					notifyAtEnd();
					break;
				}
				++_right;
			}
			for(; not _right.isAtEnd(); ++_right){
				if (checkAndSet() == CompResult::VALID) {
					break;
				}
			}
			if (_right.isAtEnd()) {
				notifyAtEnd();
			}
			break;
		case Input::RIGHT:
			Assert(false); // Guaranteed not to happen
			break;
		}
	}

private:
	bool leftIsInput() const{
		return _input==Input::BOTH || _input==Input::LEFT;
	}
	bool rightIsInput() const{
		return _input==Input::BOTH;
	}

	CompResult checkAndSet() {
		if (correct()) {
			if(not leftIsInput()){
				_leftvar->operator =(*_left);
			}
			if(not rightIsInput()){
				_rightvar->operator =(*_right);
			}
			return CompResult::VALID;
		}
		return CompResult::INVALID;
	}

	bool correct(){
		const DomainElement *leftelem, *rightelem;
		if(leftIsInput()){
			leftelem = _leftvar->get();
		}else{
			leftelem = *_left;
		}
		if(rightIsInput()){
			rightelem = _rightvar->get();
		}else{
			rightelem = *_right;
		}
		bool compsuccess = false;
		switch (comparison) {
		case CompType::LT:
			if (*leftelem < *rightelem) {
				compsuccess = true;
			}
			break;
		case CompType::LEQ:
			if (*leftelem <= *rightelem) {
				compsuccess = true;
			}
			break;
		case CompType::GT:
			if (*leftelem > *rightelem) {
				compsuccess = true;
			}
			break;
		case CompType::GEQ:
			if (*leftelem >= *rightelem) {
				compsuccess = true;
			}
			break;
		case CompType::EQ:
			if (*leftelem == *rightelem) {
				compsuccess = true;
			}
			break;
		case CompType::NEQ:
			if (*leftelem != *rightelem) {
				compsuccess = true;
			}
			break;
		}
		return compsuccess;
	}
};

#endif /* GTGenerator_HPP_ */
