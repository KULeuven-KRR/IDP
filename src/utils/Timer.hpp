#pragma once

#ifdef __MINGW32__
#include <windows.h>
#define sleep(n) Sleep(1000*n)
#endif

#include <thread>
#include "unistd.h"
#include <iostream>

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

	void time() {
		long long time = 0;
		int sleep = 10;
		while (not requestedToStop()) {
			time += sleep;
#ifdef __MINGW32__
			Sleep(sleep);
#else
			usleep(sleep * 1000);
#endif

			if (sleep < 1000) {
				if (sleep < 100) {
					sleep += 10;
				} else {
					sleep += 100;
				}
			}

			if (call_for_timebound() < time / 1000) {
				_hasTimedOut = true;
				call_on_timeout();
				break;
			}
			if(call_for_timebound()==0){
				return;
			}
		}
	}
};

typedef Timer<std::function<long(void)>, std::function<void (void)>> basicTimer;
