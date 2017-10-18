#include <executors.h>
#include <cassert>
#include <iostream>
#include <algorithm>

void Task::addDependency(std::shared_ptr<Task> dep) {
    if (dep.get() == this) throw std::runtime_error("task can't be dependent on itself");
    std::unique_lock<std::mutex> cur_lock(mutex_, std::defer_lock), dep_lock(dep->mutex_, std::defer_lock);
    std::lock(cur_lock, dep_lock);
    if (state_ != PREPARING) throw std::runtime_error("task is already submitted");
    if (dep->isFinished()) return;
    dep->depend_on_me_.push_back(shared_from_this());
    ++number_of_unfinished_dependencies_;
}

void Task::addTrigger(std::shared_ptr<Task> dep) {
    if (dep.get() == this) throw std::runtime_error("task can't be dependent on itself");
    std::unique_lock<std::mutex> cur_lock(mutex_, std::defer_lock), dep_lock(dep->mutex_, std::defer_lock);
    std::lock(cur_lock, dep_lock);
    if (state_ != PREPARING) throw std::runtime_error("task is already submitted");
    has_task_triggers_ = true;
    if (dep->isFinished()) {
        triggered_ = true;
        return;
    }
    dep->triggered_by_me.push_back(shared_from_this());
}

void Task::setTimeTrigger(std::chrono::system_clock::time_point at) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != PREPARING) throw std::runtime_error("task is already submitted");
    deadline_ = at;
    has_time_trigger_ = true;
    if (std::chrono::system_clock::now() >= deadline_) {
        triggered_ = true;
    }
}

bool Task::isCompleted() {
    return state_ == COMPLETED;
}

bool Task::isFailed() {
    return state_ == FAILED;
}

bool Task::isCanceled() {
    return state_ == CANCELED;
}

bool Task::isFinished() {
    return isCanceled() || isCompleted() || isFailed();
}

bool Task::hasNoTriggers() {
    return !(has_task_triggers_ || has_time_trigger_);
}

std::exception_ptr Task::getError() {
    return error_;
}

void Task::cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    nolockCancel();
}

void Task::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    finish_signal_.wait(lock, [this] {
        return isFinished();
    });
}

void Task::nolockCancel() {
    state_ = CANCELED;
    finish_signal_.notify_all();
    processDependentAndTriggeredTasks();
}

VeryExecutor::VeryExecutor():VeryExecutor(DEFAULT_CONCURRENCY_LEVEL) {}

VeryExecutor::VeryExecutor(size_t num_of_threads):num_of_threads_(num_of_threads) {
    pool_.reserve(num_of_threads_);
    finish_flags_.resize(num_of_threads_, false);
    for (size_t i = 0; i < num_of_threads_; ++i) {
        pool_.emplace_back([i, this] {
            threadLoop(i);
        });
    }
}

void VeryExecutor::threadLoop(int id) {
    while (!stop_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (!queue_checker_.wait_until(lock, deadline(), [this] {
            return !task_queue_.empty() || stop_;
        })) {
            if (stop_) break;
            if (!timers_heap_.empty() && std::chrono::system_clock::now() >= timers_heap_.front()->deadline_) {
                std::pop_heap(timers_heap_.begin(), timers_heap_.end(), [](const std::shared_ptr<Task>& l, const std::shared_ptr<Task>& r) {
                    return l->deadline_ > r->deadline_;
                });
                auto task = timers_heap_.back();
                std::unique_lock<std::mutex> task_lock(task->mutex_);
                if (task->isCanceled()) continue;
                task->triggered_ = true;
                task->state_ = Task::READY;
                timers_heap_.pop_back();
                task_queue_.push_back(task);
                queue_checker_.notify_one();
            }
            continue;
        };
        if (stop_) break;
        std::shared_ptr<Task> task = task_queue_.front();
        task_queue_.pop_front();
        std::unique_lock<std::mutex> task_lock(task->mutex_);
        if (task->isCanceled()) continue;
        task_lock.unlock();
        lock.unlock();
        try {
            task->run();
        } catch (...) {
            finishTask(task, Task::FAILED, std::current_exception());
            continue;
        }
        finishTask(task, Task::COMPLETED);
    }
    std::lock_guard<std::mutex> lock(finish_mutex_);
    finish_flags_[id] = true;
    finish_waiter_.notify_all();
}

void VeryExecutor::finishTask(std::shared_ptr<Task> task, Task::TaskState state, std::exception_ptr error) {
    std::unique_lock<std::mutex> lock(task->mutex_);
    if (task->isCanceled()) return;
    if (error) task->error_ = error;
    task->state_ = state;
    task->finish_signal_.notify_all();
    lock.unlock();
    task->processDependentAndTriggeredTasks();
}

std::chrono::system_clock::time_point VeryExecutor::deadline() {
    if (timers_heap_.empty()) {
        return std::chrono::system_clock::time_point::max();
    }
    return timers_heap_.front()->deadline_;
}

void Task::processDependentAndTriggeredTasks() {
    for (auto& task : depend_on_me_) {
        std::unique_lock<std::mutex> task_lock(task->mutex_, std::defer_lock), lock(task->mine_exec_->queue_mutex_, std::defer_lock);
        std::lock(task_lock, lock);
        --task->number_of_unfinished_dependencies_;
        if (task->state_ == Task::PREPARED) {
            task->mine_exec_->enqueue(task);
        }
    }
    for (auto& task : triggered_by_me) {
        std::unique_lock<std::mutex> task_lock(task->mutex_, std::defer_lock), lock(task->mine_exec_->queue_mutex_, std::defer_lock);
        std::lock(task_lock, lock);
        task->triggered_ = true;
        if (task->state_ == Task::PREPARED) {
            task->mine_exec_->enqueue(task);
        }
    }
}

void VeryExecutor::submit(std::shared_ptr<Task> task) {
    std::unique_lock<std::mutex> task_lock(task->mutex_, std::defer_lock), lock(queue_mutex_, std::defer_lock);
    std::lock(task_lock, lock);
    if (task->state_ != Task::PREPARING) throw std::runtime_error("task is no more submittable");
    task->state_ = Task::PREPARED;
    task->mine_exec_ = shared_from_this();
    if (stop_) {
        task->nolockCancel();
        return;
    }
    if (task->triggered_ || ((task->number_of_unfinished_dependencies_ == 0) && task->hasNoTriggers())) {
        task->state_ = Task::READY;
        task_queue_.push_back(task);
        queue_checker_.notify_one();
    }
    if (task->has_time_trigger_) {
        timers_heap_.push_back(task);
        std::push_heap(timers_heap_.begin(), timers_heap_.end(), [](const std::shared_ptr<Task>& l, const std::shared_ptr<Task>& r) {
            return l->deadline_ > r->deadline_;
        });
        queue_checker_.notify_one();
    }
}

void VeryExecutor::enqueue(std::shared_ptr<Task> task) {
    if (task->state_ != Task::PREPARED) throw std::runtime_error("task is not prepared!!!");
    if (stop_) {
        task->nolockCancel();
        return;
    }
    if (task->triggered_ || ((task->number_of_unfinished_dependencies_ == 0) && task->hasNoTriggers())) {
        task->state_ = Task::READY;
        task_queue_.push_back(task);
        queue_checker_.notify_one();
    }
}

void VeryExecutor::startShutdown() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;
    queue_checker_.notify_all();
    lock.unlock();
    for (auto& task : task_queue_) {
        task->cancel();
    }
    task_queue_.clear();
    for (auto& task : timers_heap_) {
        task->cancel();
    }
    timers_heap_.clear();
}

void VeryExecutor::waitShutdown() {
    std::unique_lock<std::mutex> lock(finish_mutex_);
    finish_waiter_.wait(lock, [this] {
        return threadsFinished();
    });
}

bool VeryExecutor::threadsFinished() {
    for (bool flag : finish_flags_) {
        if (!flag) return false;
    }
    return true;
}

VeryExecutor::~VeryExecutor() {
    if(!stop_) startShutdown();
    waitShutdown();
    for (auto &thread : pool_) {
        thread.join();
    }
}

std::shared_ptr<Executor> MakeThreadPoolExecutor(int num_threads) {
    assert(num_threads > 0);
    return std::make_shared<VeryExecutor>(num_threads);
}
