#ifndef OPTIONSSTACK_HPP_
#define OPTIONSSTACK_HPP_

class Options;

namespace Option{
	Options* getCurrentOptions();
	void pushOptions(Options* options);
	void popOptions();
}

#endif /* OPTIONSSTACK_HPP_ */
