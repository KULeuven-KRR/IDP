#pragma once

#ifdef __MINGW32__
#include <windows.h>
#define sleep(n) Sleep(1000*n)
#endif

#include <functional>
#include "unistd.h"
#include <iostream>

#ifdef __MINGW32__
#include <windows.h>
#define sleep(n) Sleep(1000*n)
#endif
//#include <thread>
#include <tinythread.h>
using namespace tthread;

template<class CallForTimeBound, class CallOnTimeout>
class Timer {
public:
	CallForTimeBound call_for_timebound;
	CallOnTimeout call_on_timeout;

	bool stoptiming;
	bool _hasTimedOut;

	// NOTE: timer can take 1000 millisecs to join!
	Timer(CallForTimeBound call_for_timebound, CallOnTimeout call_on_timeout)
			: 	call_for_timebound(call_for_timebound),
				call_on_timeout(call_on_timeout),
				stoptiming(false),
				_hasTimedOut(false) {
	}

	bool requestedToStop() const {
		return stoptiming;
	}

	void requestStop() {
		stoptiming = true;
	}

	bool hasTimedOut() const {
		return _hasTimedOut;
	}
};

typedef Timer<std::function<long(void)>, std::function<void (void)>> basicTimer;

void timerLoop(void* t);
