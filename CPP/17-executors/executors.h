#include <memory>
#include <chrono>
#include <vector>
#include <list>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <unordered_set>
#include <iostream>

class VeryExecutor;

class Task : public std::enable_shared_from_this<Task> {
public:
    virtual ~Task() {}

    virtual void run() = 0;

    void addDependency(std::shared_ptr<Task> dep);
    void addTrigger(std::shared_ptr<Task> dep);
    void setTimeTrigger(std::chrono::system_clock::time_point at);
    
    // Task::run() completed without throwing exception
    bool isCompleted();

    // Task::run() throwed exception
    bool isFailed();

    // Task was canceled
    bool isCanceled();

    // Task either completed, failed or was canceled
    bool isFinished();
    
    std::exception_ptr getError();

    void cancel();

    void wait();

private:
    friend class VeryExecutor;
    enum TaskState {
        COMPLETED, CANCELED, FAILED, PREPARED, READY, PREPARING
    };
    bool hasNoTriggers();
    void processDependentAndTriggeredTasks();
    void nolockCancel();
    TaskState state_ = PREPARING;
    std::condition_variable finish_signal_;
    std::mutex mutex_;
    std::exception_ptr error_ = {nullptr};
    bool has_time_trigger_ = false;
    std::chrono::system_clock::time_point deadline_;
    int number_of_unfinished_dependencies_ = 0;
    bool has_task_triggers_ = false;
    std::list<std::shared_ptr<Task>> depend_on_me_;
    std::list<std::shared_ptr<Task>> triggered_by_me;
    bool triggered_ = false;
    std::shared_ptr<VeryExecutor> mine_exec_;
};

template<class T>
class Future;

template<class T>
using FuturePtr = std::shared_ptr<Future<T>>;

// Used instead of void in generic code
struct Unit {};

class Executor {
public:
    virtual ~Executor() {}
    virtual void submit(std::shared_ptr<Task> task) = 0;
    virtual void startShutdown() = 0;
    virtual void waitShutdown() = 0;

    template<class T>
    FuturePtr<T> invoke(std::function<T()> fn);
    template<class Y, class T>
    FuturePtr<Y> then(FuturePtr<T> input, std::function<Y()> fn);
    template<class T>
    FuturePtr<std::vector<T>> whenAll(std::vector<FuturePtr<T>> all);
    template<class T>
    FuturePtr<T> whenFirst(std::vector<FuturePtr<T>> all);
    template<class T>
    FuturePtr<std::vector<T>> whenAllBeforeDeadline(std::vector<FuturePtr<T>> all,
                                                    std::chrono::system_clock::time_point deadline);
};

class VeryExecutor : public Executor, public std::enable_shared_from_this<VeryExecutor> {
public:
    VeryExecutor();
    VeryExecutor(size_t num_of_threads);
    virtual ~VeryExecutor();

    virtual void submit(std::shared_ptr<Task> task);

    virtual void startShutdown();
    virtual void waitShutdown();

private:
    friend class Task;
    void threadLoop(int id);
    void finishTask(std::shared_ptr<Task> task, Task::TaskState state, std::exception_ptr error = nullptr);
    void enqueue(std::shared_ptr<Task> task);
    bool threadsFinished();
    std::chrono::system_clock::time_point deadline();
    std::vector<std::thread> pool_;
    std::condition_variable queue_checker_;
    std::condition_variable finish_waiter_;
    std::mutex queue_mutex_;
    std::mutex finish_mutex_;
    std::vector<bool> finish_flags_;
    std::deque<std::shared_ptr<Task>> task_queue_;
    std::vector<std::shared_ptr<Task>> timers_heap_;
    size_t num_of_threads_;
    const size_t DEFAULT_CONCURRENCY_LEVEL = 8;
    bool stop_ = false;
};

std::shared_ptr<Executor> MakeThreadPoolExecutor(int num_threads);

template<class T>
class Future : public Task {
public:
    Future() = delete;
    Future(std::function<T()>);
    void run();
    T get();
private:
    T result_;
    std::function<T()> proc_;
};

template<class T>
T Future<T>::get() {
    wait();
    return result_;
}

template<class T>
void Future<T>::run() {
    result_ = proc_();
}

template<class T>
Future<T>::Future(std::function<T()> fn) {
    proc_ = fn;
}

template<class T>
FuturePtr<T> Executor::invoke(std::function<T()> fn) {
    FuturePtr<T> res = std::make_shared<Future<T>>(fn);
    submit(res);
    return res;
}

template<class Y, class T>
FuturePtr<Y> Executor::then(FuturePtr<T> input, std::function<Y()> fn) {
    input->wait();
    return invoke(fn);
}

template<class T>
FuturePtr<std::vector<T>> Executor::whenAll(std::vector<FuturePtr<T>> all) {
    if (all.size() == 0) throw std::runtime_error("no futures to process");
    FuturePtr<std::vector<T>> res = std::make_shared<Future<std::vector<T>>>([=]() {
        std::vector<T> result(all.size());
        for (size_t i = 0; i < all.size(); ++i) {
            result[i] = all[i]->get();
        }
        return result;
    });
    for (size_t i = 0; i < all.size(); ++i) {
        res->addDependency(all[i]);
    }
    submit(res);
    return res;
}

template<class T>
FuturePtr<T> Executor::whenFirst(std::vector<FuturePtr<T>> all) {
    if (all.size() == 0) throw std::runtime_error("no futures to process");
    std::vector<FuturePtr<Unit>> signals(all.size());
    int id_first = -1;
    bool blocked = false;
    for (size_t i = 0; i < all.size(); ++i) {
        signals[i] = std::make_shared<Future<Unit>>([&id_first, &blocked, i]() mutable{
            if (!blocked) {
                id_first = i;
                blocked = true;
            }
            return Unit();
        });
        signals[i]->addTrigger(all[i]);
        submit(signals[i]);
    }
    FuturePtr<T> res = std::make_shared<Future<T>>([=, &id_first]() mutable {
        return all[id_first]->get();
    });
    for (size_t i = 0; i < all.size(); ++i) {
        res->addTrigger(signals[i]);
    }
    submit(res);
    return res;
}

template<class T>
FuturePtr<std::vector<T>>
Executor::whenAllBeforeDeadline(std::vector<FuturePtr<T>> all, std::chrono::system_clock::time_point deadline) {
    if (all.size() == 0) throw std::runtime_error("no futures to process");
    auto flags = std::make_shared<std::vector<bool>>(all.size(), false);
    std::vector<FuturePtr<Unit>> signals(all.size());
    for (size_t i = 0; i < all.size(); ++i) {
        signals[i] = std::make_shared<Future<Unit>>([all, &flags, i]() mutable {
            if (all[i]->isFinished()) (*flags)[i] = true;
            return Unit();
        });
        signals[i]->setTimeTrigger(deadline);
        submit(signals[i]);
    }
    FuturePtr<std::vector<T>> res = std::make_shared<Future<std::vector<T>>>([all, flags]() {
        std::vector<T> result;
        for (size_t i = 0; i < all.size(); ++i) {
            if ((*flags)[i]) {
                result.push_back(all[i]->get());
            }
        }
        return result;
    });
    for (size_t i = 0; i < all.size(); ++i) {
        res->addDependency(signals[i]);
    }
    res->setTimeTrigger(deadline);
    submit(res);
    return res;
}
