#include <gtest/gtest.h>

#include <executors.h>

class TestTask : public Task {
public:
    TestTask(int n) {
        assert(n >= 0);
        n_ = n;
    }
    void run() {
        for (int i = 1; i <= n_; ++i) {
            result_ += i;
        }
    }
    int result_ = 0;
private:
    int n_;
};

class FailTask : public Task {
public:
    void run() {
        throw std::runtime_error("very bad error");
    }
};

TEST(SimpleExecution, OkRun) {
    std::shared_ptr<TestTask> t1(new TestTask(50));
    std::shared_ptr<TestTask> t2(new TestTask(550));

    auto pool = MakeThreadPoolExecutor(2);
    pool->submit(t1);
    pool->submit(t2);
    t2->wait();
    t1->wait();
    ASSERT_EQ(51 * 25, t1->result_);
    ASSERT_TRUE(t2->isCompleted());
}

TEST(SimpleExecution, SubmitAfterShutdown) {
    std::shared_ptr<TestTask> t3(new TestTask(60));
    auto pool = MakeThreadPoolExecutor(2);
    pool->startShutdown();
    pool->waitShutdown();
    pool->submit(t3);
    t3->wait();
    ASSERT_TRUE(t3->isCanceled());
}

TEST(SimpleExecution, FailRun) {
    std::shared_ptr<FailTask> t(new FailTask);
    auto pool = MakeThreadPoolExecutor(4);

    pool->submit(t);
    t->wait();
    ASSERT_TRUE(t->isFailed());
}

TEST(SimpleExecution, SimpleDependency) {
    using namespace std::chrono_literals;
    std::shared_ptr<TestTask> t1(new TestTask(50));
    std::shared_ptr<TestTask> t2(new TestTask(550));
    t1->addDependency(t2);

    auto pool = MakeThreadPoolExecutor(4);
    pool->submit(t1);
    std::this_thread::sleep_for(100ms);
    ASSERT_FALSE(t1->isFinished());
    pool->submit(t2);
    t1->wait();
    ASSERT_TRUE(t1->isFinished());
}

TEST(SimpleExecution, SimpleTaskTrigger) {
    using namespace std::chrono_literals;
    std::shared_ptr<TestTask> t(new TestTask(60));
    std::shared_ptr<TestTask> triggered(new TestTask(60));
    triggered->addTrigger(t);
    auto pool = MakeThreadPoolExecutor(4);
    pool->submit(triggered);
    ASSERT_FALSE(triggered->isFinished());
    pool->submit(t);
    triggered->wait();
    ASSERT_TRUE(t->isFinished());
    ASSERT_TRUE(triggered->isFinished());
}

TEST(SimpleExecution, SimpleTimeTrigger) {
    using namespace std::chrono_literals;
    std::shared_ptr<TestTask> t(new TestTask(60));
    t->setTimeTrigger(std::chrono::system_clock::now() + 100ms);
    auto pool = MakeThreadPoolExecutor(4);
    pool->submit(t);
    std::this_thread::sleep_for(20ms);
    ASSERT_FALSE(t->isFinished());
    t->wait();
    ASSERT_TRUE(t->isFinished());
}

TEST(SimpleExecution, NoChangeAfterSubmit) {
    using namespace std::chrono_literals;
    std::shared_ptr<TestTask> t(new TestTask(60));
    auto pool = MakeThreadPoolExecutor(4);
    pool->submit(t);
    ASSERT_ANY_THROW(t->setTimeTrigger(std::chrono::system_clock::now() + 100ms));
}

TEST(SimpleExecution, NoResubmit) {
    using namespace std::chrono_literals;
    std::shared_ptr<TestTask> t(new TestTask(60));
    auto pool = MakeThreadPoolExecutor(4);
    pool->submit(t);
    ASSERT_ANY_THROW(pool->submit(t));
}

TEST(Futures, SimpleGet) {
    auto pool = MakeThreadPoolExecutor(4);
    auto sum = pool->invoke<int>([](){
        int res = 0;
        for (int i = 0; i <= 90; ++i) {
            res += i;
        }
        return res;
    });
    ASSERT_EQ(91 * 45, sum->get());
}

/*TEST(Futures, WhenAll) {
    auto pool = MakeThreadPoolExecutor(4);
    std::vector<FuturePtr<int>> futures;
    for (int i = 0; i < 8; ++i) {
        futures.push_back(pool->invoke<int>([i](){
            int res = 0;
            for (int j = 0; j <= (i+1)*10; ++j) {
                res += j;
            }
            return res;
        }));
    }
    auto result = pool->whenAll(futures);
    std::vector<int> result_vector = result->get();
    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(result_vector[i], ((i + 1) * 10 + 1) * 5 * (i + 1));
    }
}

TEST(Futures, WhenFirst) {
    using namespace std::chrono_literals;
    auto pool = MakeThreadPoolExecutor(4);
    std::vector<FuturePtr<int>> futures;
    for (int i = 0; i < 8; ++i) {
        futures.push_back(pool->invoke<int>([i](){
            std::this_thread::sleep_for(i * 1ms);
            return i;
        }));
    }
    auto result = pool->whenFirst(futures);
    int result_index = result->get();
    ASSERT_EQ(result_index, 0);
}

TEST(Futures, WhenAllBeforeDeadline) {
    using namespace std::chrono_literals;
    auto pool = MakeThreadPoolExecutor(4);
    std::vector<FuturePtr<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool->invoke<int>([i](){
            std::this_thread::sleep_for(i * 1ms);
            return i;
        }));
    }
    auto result = pool->whenAllBeforeDeadline(futures, std::chrono::system_clock::now() + 10ms);
    ASSERT_LE(result->get().size(), 10);
}*/