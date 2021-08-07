// MIT License
//
// Copyright (c) 2021 Daniel Robertson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef HX711_WATCHER_H_CFBFD856_ADA7_4F6A_9D3E_B7F6D39527D5
#define HX711_WATCHER_H_CFBFD856_ADA7_4F6A_9D3E_B7F6D39527D5

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <pthread.h>
#include <sched.h>
#include "HX711.h"
#include "Value.h"
#include "ValueStack.h"

namespace HX711 {

enum class WatchState {
    NONE,
    NORMAL,
    PAUSE,
    END
};

class Watcher {

protected:

    static const int _PINWATCH_SCHED_POLICY = SCHED_FIFO;
    
    static constexpr auto _DEFAULT_PAUSE_SLEEP = std::chrono::duration_cast
        <std::chrono::nanoseconds>(std::chrono::milliseconds(100));

    static constexpr auto _DEFAULT_POLL_SLEEP = std::chrono::duration_cast
        <std::chrono::nanoseconds>(std::chrono::milliseconds(10));
    
    static constexpr auto _DEFAULT_NOT_READY_SLEEP = std::chrono::duration_cast
        <std::chrono::nanoseconds>(std::chrono::microseconds(7));

    HX711* _hx;
    WatchState _watchState;
    std::mutex _pinWatchLock;
    pthread_t _watchThreadId;
    std::chrono::nanoseconds _pauseSleep;
    std::chrono::nanoseconds _notReadySleep;
    std::chrono::nanoseconds _pollSleep;

    static void* _watchPin(void* const watcherPtr);
    void _changeWatchState(const WatchState state);
    void _recoverHX711(const std::chrono::nanoseconds maxWait);


public:
    ValueStack values;
    std::mutex valuesLock;
    explicit Watcher(HX711* const hx) noexcept;
    ~Watcher();
    void begin();
    void watch();
    void pause();

};
};
#endif
