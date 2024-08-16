#include "thread_pool.hpp"

void ThreadPool::execute() {
	while (isRunning) {
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(resourceGuard);
			// Wait until there is a task or we need to stop
			cv.wait(lock, [this] { return !isRunning  || !tasks.empty(); });

			if (!isRunning && tasks.empty()) {
				return; // Exit if stopping and no tasks left
			}

			// Get the next task
			task = std::move(tasks.front());
			tasks.pop();
		}
		
		// unique_locks get released when they go outside of scope, no need for manual unlocking
		task();
	}
}

ThreadPool::ThreadPool(int nrOfThreads) {
	isRunning = true;
	for (int i = 0; i < nrOfThreads; ++i) {
		pool.emplace_back(
			std::thread(&ThreadPool::execute, this)
		);
	}
}

ThreadPool::~ThreadPool() {
	terminate();
}

void ThreadPool::terminate() {
    {
        std::unique_lock<std::mutex> lock(resourceGuard);
        isRunning = false;
    }
    cv.notify_all();  // Wake up all threads to stop them.

    // Join all threads to ensure proper cleanup.
    for (auto& thread : pool) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
