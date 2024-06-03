#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <iostream>
#include <thread>

// Custom implementation of a thread pool. Enqueue tasks on the queue to get
// one of the worker threads to run it.
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads) : stop(false), tasks_busy(0) {
        for (size_t i = 0; i < numThreads; ++i) {
            // Add a worker thread
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        task_condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        // If the threads have been explicitly stopped or the queue is empty
                        // then kill the worker thread
                        if (stop && tasks.empty()) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop();
                        tasks_busy++;

                        lock.unlock();
                        try {
                            // Run the task here
                            task();
                        } catch (const std::exception& e) {
                            std::cout << "Exception in thread: " << e.what() << std::endl;
                        } catch (...) {
                            std::cout << "Unknown exception in thread" << std::endl;
                        }

                        lock.lock();
                        tasks_busy--;
                        pool_finished_condition.notify_one();
                    }
                }
            });
        }
        std::cout << "Done creating " << numThreads << " workers" << std::endl;
    }

    // Enqueue a task onto the queue for the worker threads to run
    template<class F>
    void enqueue(F&& task) {
        // Add the task to the queue - but first wait to acquire the queue lock
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (stop) {
                std::cout << "Enqueue on stopped ThreadPool" << std::endl;
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks.emplace(std::forward<F>(task));
        }
        // Notify one of the worker threads to wake up and run one of the tasks from
        // the queue
        task_condition.notify_one();
    }

    // Returns once the worker threads have all finished and returned
    void wait_finished() {
        std::unique_lock<std::mutex> lock(queueMutex);
        std::cout << "Waiting for ThreadPool to finish" << std::endl;
        pool_finished_condition.wait(lock, [this] { return tasks.empty() && tasks_busy == 0; });
        std::cout << "ThreadPool finished" << std::endl;
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stop = true;
        }
        std::cout << "Thread pool destructor called" << std::endl;
        task_condition.notify_all();
        for (std::thread &worker : workers) {
            worker.join();
        }
    }

private:
    // Collection of worker threads
    std::vector<std::thread> workers;
    // Collection of tasks enqueued for the worker threads to run
    std::queue<std::function<void()>> tasks;
    // Mutex to access the queue
    std::mutex queueMutex;
    // Set when a task is enqueued
    std::condition_variable task_condition;
    // Set when the thread pool has concluded
    std::condition_variable pool_finished_condition;
    // Number of threads busy running tasks
    unsigned int tasks_busy;
    // Set this to get the threads in the pool to all return
    bool stop;
};

#endif // THREAD_POOL_H