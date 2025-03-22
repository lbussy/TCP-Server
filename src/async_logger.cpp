#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

// A simple asynchronous logger.
class AsyncLogger {
public:
    AsyncLogger() : stopFlag(false) {
        workerThread = std::thread(&AsyncLogger::worker, this);
    }

    ~AsyncLogger() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stopFlag = true;
        }
        cv.notify_all();
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    // Non-blocking log; caller just enqueues the message.
    void log(const std::string& msg) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(msg);
        }
        cv.notify_one();
    }

private:
    std::queue<std::string> messageQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    bool stopFlag;
    std::thread workerThread;

    // The worker thread prints messages from the queue.
    void worker() {
        while (true) {
            std::string msg;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                cv.wait(lock, [this] { return !messageQueue.empty() || stopFlag; });
                if (stopFlag && messageQueue.empty())
                    break;
                msg = std::move(messageQueue.front());
                messageQueue.pop();
            }
            // Print the whole message atomically.
            std::cout << msg << std::endl;
        }
    }
};
