#include "WorkerPool.h"

namespace WPEFramework {

    namespace Core {

	IWorkerPool* _workerPoolInstance = nullptr;

        /* static */ void IWorkerPool::Assign(IWorkerPool* instance) {
            ASSERT ( (_workerPoolInstance == nullptr) ^ (instance == nullptr) );
            _workerPoolInstance = instance;
        }
        /* static */ IWorkerPool& IWorkerPool::Instance() {
            ASSERT(_workerPoolInstance != nullptr);
            return (*_workerPoolInstance);
        }
        /* static */ bool IWorkerPool::IsAvailable() {
            return (_workerPoolInstance != nullptr);
        }
    }
}
