#pragma once

#ifdef __MINGW32__
#include <windows.h>
#define sleep(n) Sleep(1000*n)
#endif

#include <thread>
#include "unistd.h"

template <class Exec>
class Timer{
public:
	long long timeout_in_sec;
	Exec call_on_timeout;

	bool stoptiming;
	bool _hasTimedOut;

	Timer(long timeout_in_seconds, Exec call_on_timeout): timeout_in_sec(timeout_in_seconds), call_on_timeout(call_on_timeout),
			stoptiming(false), _hasTimedOut(false){

	}

	bool requestedToStop() const {
		return stoptiming;
	}

	void requestStop() {
		stoptiming = true;
	}

	bool hasTimedOut() const{
		return _hasTimedOut;
	}

	void time() {
		if (timeout_in_sec == 0) {
			return;
		}
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
			if (timeout_in_sec < time / 1000) {
				_hasTimedOut = true;
				call_on_timeout();
				break;
			}
		}
	}
};
