/************************************
 Generators.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef INSTGENERATOR_HPP_
#define INSTGENERATOR_HPP_

class InstChecker{
public:
	// TODO enum?
	// FIXME do not set output variables
	virtual bool check() = 0; // Check whether current input + output is correct
	virtual bool checkInput() = 0; // Check whether current input still has at least one output instantiation
};

class InstGenerator: public InstChecker {
private:
	bool end;
protected:
	void notifyAtEnd(){
		end = true;
	}

	virtual void next() = 0;
	virtual void reset() = 0;

public:
	virtual ~InstGenerator(){}

	// Can also be used for resets
	// SETS the instance to the FIRST value if it exists
	void begin(){
		end = false;
		reset();
		if(not isAtEnd()){
			next();
		}
	}

	/**
	 * Returns true if the last element has already been set as an instance
	 */
	bool isAtEnd() const { return end; }

	void operator++() {
		assert(not isAtEnd());
		next();
	}
};

#endif /* INSTGENERATOR_HPP_ */
