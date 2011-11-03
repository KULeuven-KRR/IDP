#ifndef GROUNDER_HPP_
#define GROUNDER_HPP_

class Grounder{
private:
	AbstractGroundTheory* _grounding;
	int _verbosity;
public:
	Grounder(AbstractGroundTheory* gt, int verb) :
			_grounding(gt), _verbosity(verb) {
	}
	virtual ~TopLevelGrounder() {
	}
	virtual Lit run() const = 0;

	AbstractGroundTheory* grounding() const {
		return _grounding;
	}
};

#endif /* GROUNDER_HPP_ */
