#pragma once

#ifdef __MINGW32__
#include <windows.h>
#define sleep(n) Sleep(1000*n)
#endif

#include <functional>
#include "unistd.h"
#include <iostream>

#include <tinythread.h>

template<class CallForTimeBound, class CallForMemoryBound, class CallOnTimeout>
class ResourceMonitor {
public:
	CallForTimeBound call_for_timebound;
	CallForMemoryBound call_for_memorybound;
	CallOnTimeout call_on_timeout;

	bool stopmonitoring;
	bool _wentOut;

	// NOTE: timer can take 1000 millisecs to join!
	ResourceMonitor(CallForTimeBound call_for_timebound, CallForMemoryBound call_for_memorybound, CallOnTimeout call_on_timeout)
			: 	call_for_timebound(call_for_timebound),
			  	call_for_memorybound(call_for_memorybound),
				call_on_timeout(call_on_timeout),
				stopmonitoring(false),
				_wentOut(false) {
	}

	bool requestedToStop() const {
		return stopmonitoring;
	}

	void requestStop() {
		stopmonitoring = true;
	}

	bool outOfResources() const {
		return _wentOut;
	}
};

typedef ResourceMonitor<std::function<long(void)>, std::function<long(void)>, std::function<void (void)>> basicResourceMonitor;

void resourceMonitorLoop(void* t);
