#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

void sleep(unsigned milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

class MeanCounterBase {
public:
    virtual ~MeanCounterBase() { }

    virtual double mean() = 0;
    virtual void add(int value) = 0;

    bool check(double expectedMean) const {
        return std::abs(expectedMean - calcMean()) <
            std::numeric_limits<double>::epsilon();
    }

protected:
    double calcMean() const {
        sleep(1000);
        return (double)sum / count;
    }

    void doAdd(int value) {
        sleep(1000);
        sum += value;
        sleep(1000);
        count += 1;
    }

private:
    long sum = 0;
    long count = 0;
};

void writer(int id, MeanCounterBase& counter, int value) {
    std::printf("[%d] Writer started\n", id);
    counter.add(value);
    std::printf("[%d] Added value: %d\n", id, value);
}

void reader(int id, MeanCounterBase& counter) {
    std::printf("[%d] Reader started\n", id);
    double mean = counter.mean();
    std::printf("[%d] Read mean: %f\n", id, mean);
}

bool check(MeanCounterBase& counter) {
    return counter.check(counter.mean());
}

// === DO NOT REMOVE THIS LINE ===
//// Your solution below

#include <mutex>
#include <condition_variable>

class MeanCounter : public MeanCounterBase {
public:
    double mean() override {
        std::unique_lock<std::mutex> lock(mutex_);
        ++waiting_readers_;
        rcv_.wait(lock, [this] {
            return waiting_writers_ == 0;
        });
        --waiting_readers_;
        ++readers_;
        lock.unlock();
        double mean = calcMean();
        lock.lock();
        --readers_;
        if (!readers_) wcv_.notify_one();
        return mean;
    }

    void add(int value) override {
        std::unique_lock<std::mutex> lock(mutex_);
        ++waiting_writers_;
        wcv_.wait(lock, [this] {
            return readers_ == 0;
        });
        --waiting_writers_;
        doAdd(value);
        if (waiting_readers_) rcv_.notify_all();
        else wcv_.notify_one();

    }

private:
    std::mutex mutex_;
    std::condition_variable wcv_;
    std::condition_variable rcv_;
    int readers_ = 0;
    int waiting_writers_ = 0;
    int waiting_readers_ = 0;
};

//// Your solution above
// === DO NOT REMOVE THIS LINE ===

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: %s trace_file\n", argv[0]);
        return 1;
    }

    MeanCounter counter;
    std::vector<std::thread> threads;

    std::ifstream f(argv[1]);
    int thread_id = 0;
    while(f) {
        std::string op;
        f >> op;
        if (op == "W") {
            int value;
            f >> value;
            threads.emplace_back(writer, thread_id, std::ref(counter), value);
        } else if (op == "R") {
            threads.emplace_back(reader, thread_id, std::ref(counter));
        } else if (!op.empty()) {
            std::fprintf(stderr, "Unknown op: %s\n", op.c_str());
            break;
        }
        thread_id++;
        sleep(500);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    if (!check(counter)) {
        std::fprintf(stderr, "Make sure you are using MeanCounterBase\n");
        return 2;
    }
}
