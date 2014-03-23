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
#include <iostream>
#include "DWThreadPool.h"

/**
 * Init and active the threads in the pool.
 * call 'run()' to assign a new task.
 */
void DWThreadPool::start() throw (std::runtime_error)
{
	_is_running = true;

	// init threads array
	try {
		_threads.reserve(_thread_num);
		for (int i = 0; i < _thread_num; i++) {
			_threads.push_back(std::thread(
						std::bind(&DWThreadPool::dispatch, this)));
		}
	} catch(std::exception &e) {
		stop();
		std::cerr << e.what();
		std::runtime_error fail("Failed to init thread pool. Stopped.");	
		throw fail;
	}
}

void DWThreadPool::stop()
{
	{
		std::unique_lock<std::mutex> ulock(_queue_mutex);
		_is_running = false;
		_not_empty.notify_all();
	}

	for(auto &iter : _threads) {
		iter.join();
	}
}

/**
 * Add a new task to the pool.
 * The new task will be executed as soon as
 * there is at least one free thread.
 *
 * NOTE: This method MAY be blocked.
 */
void DWThreadPool::run(const Task &task)
{
	if (_threads.empty())
		// if there is no threads in the pool,
		// we run it on the main thread.
		task();
	else {
		// add the new task to the queue
		// and notify one free thread
		std::unique_lock<std::mutex> ulock(_queue_mutex);
		while (queueIsFull()) {
			// one may be blocked here
			_not_full.wait(ulock);
		}

		_task_queue.push_back(task);
		_not_empty.notify_one();
	}
}

bool DWThreadPool::hasRemain() const
{
	return (_task_queue.size() != 0);
}

bool DWThreadPool::queueIsFull() const
{
	return (_max_queue_size > 0 &&
			_task_queue.size() >= _max_queue_size);
}

/**
 * This is the main run-loop for each thread.
 * Each checks the task queue and selects one to execute.
 */
void DWThreadPool::dispatch()
{
	while (_is_running) {
		Task task(getTask());
		if (task) {
			task();
		}
	}
}

/**
 * Look up the task queue and dequeue a task to run.
 */
Task DWThreadPool::getTask()
{
	// wait until there is a available task
	std::unique_lock<std::mutex> ulock(_queue_mutex);
	while (_is_running && _task_queue.empty()) {
		_not_empty.wait(ulock);
	}

	Task task;
	if (!_task_queue.empty()) {
		task = _task_queue.front();
		_task_queue.pop_front();
		// notify the queue is not full now
		if (_max_queue_size > 0) {
			_not_full.notify_one();
		}
	}

	return task;
}

