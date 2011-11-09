#ifndef FOBDDTERM_HPP_
#define FOBDDTERM_HPP_

class Sort;
class FOBDDVisitor;

class FOBDDArgument {
public:
	virtual ~FOBDDArgument() {
	}
	virtual bool containsDeBruijnIndex(unsigned int index) const = 0;
	bool containsFreeDeBruijnIndex() const {
		return containsDeBruijnIndex(0);
	}

	virtual void accept(FOBDDVisitor*) const = 0;
	virtual const FOBDDArgument* acceptchange(FOBDDVisitor*) const = 0;

	virtual Sort* sort() const = 0;
};

#endif /* FOBDDTERM_HPP_ */
