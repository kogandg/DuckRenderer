#include "ThreadPool.h"

ThreadPool::ThreadPool(int numThreads)
{
	if (numThreads > std::thread::hardware_concurrency())
	{
		std::println("Threads requested are greater than threads available");
		std::println("Threads to be used set to max: {}", std::thread::hardware_concurrency());
		numThreads = std::thread::hardware_concurrency();
	}

	this->stop = false;

	for (int i = 0; i < numThreads; i++)
	{
		workers.emplace_back([this]()
			{
				while (true)
				{
					std::function<void()> job;
					{
						std::unique_lock<std::mutex> lock(queueMutex);
						queueCV.wait(lock, [this]() { return stop || !jobQueue.empty(); });

						if (stop && jobQueue.empty()) return;

						job = jobQueue.front();
						jobQueue.pop();
					}

					job();
				}
			});
	}
}

ThreadPool::~ThreadPool()
{
	Shutdown();
}

void ThreadPool::Enqueue(const std::function<void()>& job)
{
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		jobQueue.push(job);
	}
	queueCV.notify_one();
}

void ThreadPool::Cancel()
{
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		stop = true;

		while (!jobQueue.empty()) 
		{
			jobQueue.pop();
		}
	}
	queueCV.notify_all();
}

void ThreadPool::Shutdown()
{
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		stop = true;
	}
	queueCV.notify_all();

	for (std::thread& worker : workers)
	{
		if (worker.joinable())
		{
			worker.join();
		}
	}

	workers.clear();
}
