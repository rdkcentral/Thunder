#include "WorkerPool.h"

namespace WPEFramework {

	namespace Core {

	    /* static */ WorkerPool* WorkerPool::_instance = nullptr;

		WorkerPool::WorkerPool(const uint8_t threadCount, uint32_t* counters)
			: _handleQueue(16)
			, _occupation(0)
			, _timer(1024 * 1024, _T("WorkerPool::Timer"))
		{
			ASSERT(_instance == nullptr);

			_metadata.Slots = threadCount;
			_metadata.Slot = counters;
			_instance = this;
		}

		WorkerPool ::~WorkerPool()
		{
			_handleQueue.Disable();
			_instance = nullptr;
		}
	}
}
