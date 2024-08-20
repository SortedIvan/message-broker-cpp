#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <functional> 
#include <thread>
#include <iostream>

class ThreadPool {
private:
	std::vector<std::thread> pool;
	std::queue<std::function<void()>> tasks;
	std::mutex resourceGuard;
	std::condition_variable cv;
	bool isRunning = false;
public:
	void execute();
	void terminate();
	void initialize(int nrOfThreads);
	ThreadPool();
	ThreadPool(int nrOfThreads);
	~ThreadPool();
	
	template<class F, class... Args>
	void addTask(F&& f, Args&&... args) {
		auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		{
			std::unique_lock<std::mutex> lock(resourceGuard);
			tasks.emplace([task]() { task(); });
		}
		cv.notify_one();
	}
};