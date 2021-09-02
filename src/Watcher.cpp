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

#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <pthread.h>
#include <sched.h>
#include <stdexcept>
#include "../include/GpioException.h"
#include "../include/IntegrityException.h"
#include "../include/TimeoutException.h"
#include "../include/Utility.h"
#include "../include/Value.h"
#include "../include/Watcher.h"

namespace HX711 {

void* Watcher::_watchPin(void* const watcherPtr) {
    
    /**
     * This is the thread loop function for watching when data is ready from
     * the HX711.
     * 
     * A state var is used to control what operation(s) this thread is
     * performing.
     *      NONE:   undefined state and should not be used
     *      NORMAL: normal operation of obtaining sensor values and sleeping
     *      PAUSE:  causes the thread to stop performing any operations without
     *              causing the thread to end
     *      END:    lets the thread exit - not recoverable
     * 
     * Two relevant locks are used: state and data ready
     * 
     * State Lock:
     * In order for this loop's state to be changed, a state var is used. But
     * the mechanism for changing state needs to be atomic - it must not be
     * altered while the current state is executing. Thefore, at the beginning
     * of each loop, the state is locked. This does not actually "lock" the var
     * but controls access to it with the _changeWatchState function. The state
     * lock should be unlocked prior to the next iteration of the loop, but
     * prior to [any] sleep/yield calls. This will give any other thread(s) the
     * best opportunity to acquire a lock and change the state while this
     * thread is asleep and/or yielding.
     * 
     * DataReady Lock:
     * This lock works in conjunction with a condition variable in order to
     * notify the caller when a new sensor reading is available for be used.
     * It should not be unlocked until a usable value has been obtained. A
     * caller can timeout while waiting if needed.
     */

    /**
     * Can <atomic> be used to sync state?
     * Update 1: Perhaps not. Need finer-grained control over WHEN
     * the state can be modified. Mutex serves that purpose.
     */

    Watcher* const self = static_cast<Watcher*>(watcherPtr);

    std::unique_lock<std::mutex> stateLock(self->_pinWatchLock, std::defer_lock);
    std::unique_lock<std::mutex> valsLock(self->valuesLock, std::defer_lock);
    Value v;

    for(;;) {

        switch(self->_watchState) {
        case WatchState::NORMAL:

            //once in normal state, lock to ensure the process of
            //obtaining sensor data is not interrupted
            stateLock.lock();

            /**
             * check if the sensor is ready to send data
             * if not, the thread should delay and check again
             * 
             * Note the use of delay here. The wait time for readiness is
             * very short such that is does not make sense to allow another
             * thread to take over.
             * 
             * Yes, the state is unlocked. But this is to prevent deadlock
             * (see below).
             * 
             * ISSUE: it is possible for this thread to retain a lock over
             * the data ready lock if the HX711 is never ready, but is this
             * actually a problem?
             */
            if(!self->_hx->isReady()) {
                stateLock.unlock();
                ::sched_yield();
                Utility::delay(self->_notReadySleep);
                continue;
            }

            //at this point, all OK to read the sensor's value
            try {
                v = self->_hx->readValue();
            }
            catch(const IntegrityException& ex) {

                /**
                 * This exception signifies a bit read failure; it is unercoverable
                 * The HX711 will assume power down mode in this case, so it needs to be
                 * powered up again. A read can allow this to occur.
                 */
                self->_recoverHX711(std::chrono::milliseconds(50));

                stateLock.unlock();
                continue;

            }
            catch(const GpioException& ex) { 
                
                /**
                 * An exception here is assumed to be at the hardware-level. That is,
                 * a hardware GPIO issue. The sensor read retry should be instant.
                 * 
                 * Technically, if the retry is instant, the state should remain
                 * locked. However, in the case that the GPIO issue is NOT
                 * momentary, not unlocking will lead to an infinite loop with
                 * this thread holding the lock with no way to break it.
                 * 
                 * So, even though the following unlock() will very quickly be
                 * followed-up with a lock() at the beginning of the NORMAL case,
                 * it does give an opportunity for it to be broken by other code
                 * wanting to change state (ie. an error detection mechanism).
                 */
                stateLock.unlock();
                continue;

            }

            self->valuesLock.lock();
            self->values.push(v);
            self->valuesLock.unlock();

            //after having read the value, let the other thread(s)
            //know it is ready, and then release the locks
            stateLock.unlock();

            //finally, sleep for a reasonable amount of time
            //to go through the process again
            Utility::sleep(self->_pollSleep);
            continue;


        case WatchState::PAUSE:
            
            //documentation recommends sched_yield over pthread_yield
            //https://man7.org/linux/man-pages/man3/pthread_yield.3.html#CONFORMING_TO
            ::sched_yield();
            Utility::sleep(self->_pauseSleep);
            continue;
            

        case WatchState::END:
        case WatchState::NONE:
        default:
            goto endthread;

        }

    }

    /**
     * Any thread cleanup stuff goes here
     * 
     * This is a label for a goto to break out of the switch
     * and for loop.
     */
    endthread:

    //return a code to stop the warning against non-return
    return EXIT_SUCCESS;

}

void Watcher::_changeWatchState(const WatchState state) {

    std::lock_guard<std::mutex> lck(this->_pinWatchLock);

    //check for a change in state and then whether that change is
    //to a normal or paused state to adjust thread priority
    if(state != this->_watchState) {
        if(state == WatchState::NORMAL || state == WatchState::PAUSE) {

            int (*priFunc)(int);

            if(state == WatchState::NORMAL) {
                priFunc = &::sched_get_priority_max;
            }
            else {
                priFunc = &::sched_get_priority_min;
            }

            Utility::setThreadPriority(
                priFunc(_PINWATCH_SCHED_POLICY),
                _PINWATCH_SCHED_POLICY,
                this->_watchThreadId);

        }
    }

    this->_watchState = state;

}

void Watcher::_recoverHX711(const std::chrono::nanoseconds maxWait) {

    using namespace std::chrono;

    const auto whenExceeded = high_resolution_clock::now() + maxWait;

    //essentially...

    for(;;) {

        try {
            //if the read succeeds, return and go back to normal processing
            this->_hx->readValue();
            return;
        }
        catch(const IntegrityException& ex) {

            //if the read fails...

            //...and the max wait is met, return anyway
            if(high_resolution_clock::now() >= whenExceeded) {
                return;
            }

            //otherwise, keep trying
            continue;

        }

    }

}

Watcher::Watcher(HX711* const hx) noexcept :
    _hx(hx),
    _watchState(WatchState::PAUSE),
    _watchThreadId(-1),
    _pauseSleep(_DEFAULT_PAUSE_SLEEP),
    _notReadySleep(_DEFAULT_NOT_READY_SLEEP),
    _pollSleep(_DEFAULT_POLL_SLEEP) {
}

Watcher::~Watcher() {
    this->_changeWatchState(WatchState::END);
}

void Watcher::begin() {

    if(!(
        ::pthread_create(&this->_watchThreadId, nullptr, &Watcher::_watchPin, this) == 0 &&
        ::pthread_detach(this->_watchThreadId) == 0
    )) {
        throw std::runtime_error("unable to watch data pin value");
    }

}

void Watcher::watch() {
    this->_changeWatchState(WatchState::NORMAL);
}

void Watcher::pause() {
    this->_changeWatchState(WatchState::PAUSE);
}

};
