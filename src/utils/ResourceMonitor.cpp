#include "ResourceMonitor.hpp"
#include "utils/MemoryUsage.hpp"

void resourceMonitorLoop(void* t) {
	auto timer = (basicResourceMonitor*)t;
	long long time = 0;
	int sleep = 10;
	while (not timer->requestedToStop()) {
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
		if (timer->call_for_timebound() < time / 1000) {
			timer->_wentOut = true;
			timer->call_on_timeout();
			break;
		}
		if(timer->call_for_memorybound() < (long)getCurrentRSS()/1024/1024){
			timer->_wentOut = true;
			timer->call_on_timeout();
			break;
		}
	}
}
