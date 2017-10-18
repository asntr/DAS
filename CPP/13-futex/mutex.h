#pragma once

#include <linux/futex.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
// Atomically do the following:
//    if (*value == expected_value) {
//        sleep_on_address(value)
//    }
void FutexWait(int *value, int expected_value) {
    syscall(SYS_futex, value, FUTEX_WAIT_PRIVATE, expected_value);
}

// Wakeup 'count' threads sleeping on address of value
void FutexWake(int *value, int count) {
    syscall(SYS_futex, value, FUTEX_WAKE_PRIVATE, count);
}

class Mutex {
public:
    void lock() {
        int c = 0;
        if (!__atomic_compare_exchange_n(&val, &c, 1, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            if (c != 2) {
                c = __atomic_exchange_n(&val, 2, __ATOMIC_SEQ_CST);
            }
            while (c != 0) {
                FutexWait(&val, 2);
                c = __atomic_exchange_n(&val, 2, __ATOMIC_SEQ_CST);
            }
        }
    }

    void unlock() {
        if (__atomic_fetch_sub(&val, 1, __ATOMIC_SEQ_CST) != 1) {
            val = 0;
            FutexWake(&val, 1);
        }
    }
private:
    int val = 0;
};
