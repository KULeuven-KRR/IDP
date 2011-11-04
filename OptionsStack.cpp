#include <stack>

#include "OptionsStack.hpp"
#include "options.hpp"

using namespace std;

deque<Options*> optionlist;

// FIXME cleanup on reset
// FIXME add default options?

Options* Option::getCurrentOptions(){
	optionlist.back();
}
void Option::pushOptions(Options* options){
	optionlist.push_back(options);
}
void Option::popOptions(){
	optionlist.pop_back();
}
