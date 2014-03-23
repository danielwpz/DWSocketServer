/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 danielwpz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <stdio.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>

using namespace std;

typedef std::function<void()> Task;

/**
 * In real world, you should configure the
 * following parameters to best fit your
 * device.
 *
 * The pool_size should be between 1 to 2
 * times of the number of CPUs in your 
 * computer.
 *
 * The max_task should be between 1 to 2
 * times of the maximum number of pending
 * tasks, you can resolve it according to 
 * your design.
 */
#define LIGHT_BURDEN_POOL_SIZE		4
#define DEFAULT_BURDEN_POOL_SIZE	6 
#define HEAVY_BURDEN_POOL_SIZE		8

#define LIGHT_BURDEN_MAX_TASK		64
#define DEFAULT_BURDEN_MAX_TASK		256
#define HEAVY_BURDEN_MAX_TASK		768

class DWThreadPool
{
private:
	int _thread_num;
	int _max_queue_size;
	bool _is_running;
	std::mutex _queue_mutex;
	std::condition_variable _not_empty;
	std::condition_variable _not_full;
	std::vector<std::thread> _threads;
	std::deque<Task> _task_queue;

public:
	DWThreadPool(int thread_num = LIGHT_BURDEN_POOL_SIZE, 
			int max_queue_size = LIGHT_BURDEN_MAX_TASK)
		:_thread_num(thread_num)
		,_max_queue_size(max_queue_size)
		,_is_running(false)
	{}

	~DWThreadPool()
	{
		if (_is_running)
			stop();
	}

	/* No copying or assignment is allowed. */
	DWThreadPool(const DWThreadPool &) = delete;
	void operator=(const DWThreadPool &) = delete;

	/**
	 * Init and active the threads in the pool.
	 * Call 'run()' to assign a new task.
	 */
	void start() throw (std::runtime_error);

	/**
	 * Stop the pool right now, wait for all
	 * running tasks to be finished.
	 *
	 * NOTE: This method will stop the tasks
	 *		 as soon as possible, it can't
	 *		 insure that all tasks have been
	 *		 finished.
	 */
	void stop();

	/**
	 * Add a new task to the pool.
	 * The new task will be executed as soon as
	 * there is at least one free thread.
	 *
	 * NOTE: This method MAY be blocked.
	 */
	void run(const Task &task);

	/**
	 * Checks whether there remains some task
	 * which have not been executed.(Never
	 * been assigned to any thread)
	 */
	bool hasRemain() const;

private:
	bool queueIsFull() const;
	/**
	 * This is the main run-loop for each thread.
	 * Each checks the task queue and selects one to execute.
	 */
	void dispatch();
	/**
	 * Look up the task queue and dequeue a task to run.
	 */
	Task getTask();
};
