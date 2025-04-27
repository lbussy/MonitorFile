/**
 * @file monitorfile.cpp
 * @brief Implementation file for MonitorFile class.
 *
 * This software is distributed under the MIT License. See LICENSE.md for details.
 *
 * Copyright (C) 2025 Lee C. Bussy (@LBussy). All rights reserved.
 */

#include "monitorfile.hpp"
#include <iostream>

/**
 * @brief Constructs the MonitorFile object.
 *
 * Initializes monitoring flags and polling interval.
 */
MonitorFile::MonitorFile()
    : stop_monitoring(false),
      polling_interval(std::chrono::seconds(1)),
      monitoring_state(MonitorState::NOT_MONITORING)
{
}

/**
 * @brief Destroys the MonitorFile object.
 *
 * Ensures monitoring stops cleanly before destruction.
 */
MonitorFile::~MonitorFile()
{
    stop();
}

/**
 * @brief Starts monitoring a specified file.
 *
 * @param fileName Name of the file to monitor.
 * @param cb Optional callback function to invoke when the file changes.
 * @return MonitorState::MONITORING if monitoring starts successfully,
 *         MonitorState::FILE_NOT_FOUND if the file does not exist.
 */
MonitorState MonitorFile::filemon(const std::string &fileName, std::function<void()> cb)
{
    std::unique_lock<std::shared_mutex> lock(mutex);

    if (monitoring_thread.joinable())
    {
        stop();
    }

    if (!fs::exists(fileName))
    {
        monitoring_state.store(MonitorState::FILE_NOT_FOUND);
        return MonitorState::FILE_NOT_FOUND;
    }

    file_name = fileName;
    org_time = fs::last_write_time(file_name);
    stable_checks = 0;
    monitoring_state.store(MonitorState::MONITORING);

    if (cb)
    {
        callback = std::move(cb);
    }

    stop_monitoring.store(false);
    monitoring_thread = std::thread(&MonitorFile::monitor_loop, this);

    return MonitorState::MONITORING;
}

/**
 * @brief Sets the scheduling policy and priority for the monitoring thread.
 *
 * @details
 * Uses `pthread_setschedparam()` to configure the monitoring thread’s
 * scheduling behavior. This is useful when monitoring must respond quickly
 * under real-time or high-priority conditions.
 *
 * @param schedPolicy The desired scheduling policy (e.g., `SCHED_FIFO`, `SCHED_RR`, `SCHED_OTHER`).
 * @param priority The priority level associated with the policy.
 *
 * @return `true` if the scheduling parameters were successfully applied.
 * @return `false` if the thread is not running or if `pthread_setschedparam()` fails.
 *
 * @note
 * - Requires appropriate system privileges (e.g., `CAP_SYS_NICE`) to apply real-time policies.
 * - This function should be called after the monitoring thread has started.
 */
bool MonitorFile::setPriority(int schedPolicy, int priority)
{
    // Ensure that the server thread is running.
    if (!monitoring_thread.joinable())
    {
        return false;
    }

    sched_param sch_params;
    sch_params.sched_priority = priority;
    // Attempt to apply the scheduling policy and priority
    int ret = pthread_setschedparam(monitoring_thread.native_handle(), schedPolicy, &sch_params);

    return (ret == 0);
}

/**
 * @brief Stops the file monitoring thread.
 */
void MonitorFile::stop()
{
    bool expected = false;
    if (!stop_monitoring.compare_exchange_strong(expected, true))
    {
        return;
    }

    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        monitoring_state.store(MonitorState::NOT_MONITORING);
    }

    cv.notify_all();

    if (monitoring_thread.joinable())
    {
        monitoring_thread.join();
    }
}

/**
 * @brief Sets the polling interval for monitoring.
 *
 * @param interval The new polling interval in milliseconds.
 */
void MonitorFile::set_polling_interval(std::chrono::milliseconds interval)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    polling_interval = interval;
    cv.notify_all();
}

/**
 * @brief Gets the current state of the file monitor.
 *
 * @return The current monitoring state.
 */
MonitorState MonitorFile::get_state()
{
    std::shared_lock<std::shared_mutex> lock(mutex);
    return monitoring_state;
}

/**
 * @brief Sets a callback function to be called when the file changes.
 *
 * @param func The callback function.
 */
void MonitorFile::set_callback(std::function<void()> func)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    callback = std::move(func);
}

/**
 * @brief Runs the monitoring loop to detect file changes.
 */
void MonitorFile::monitor_loop()
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    // Initialize last_reported_time so we never treat the very first timestamp
    // as “new” when it's actually just our starting point.
    fs::file_time_type last_reported_time = org_time.value();

    // This flag tracks whether we've seen a write > org_time yet.
    bool change_detected = false;
    stable_checks = 0;

    while (!stop_monitoring.load())
    {
        // wait_for returns true if predicate (stop) becomes true
        cv.wait_for(lock, polling_interval, [this] {
            return stop_monitoring.load();
        });
        if (stop_monitoring.load())
            return;

        if (!fs::exists(file_name))
        {
            monitoring_state.store(MonitorState::FILE_NOT_FOUND);
            continue;
        }

        // release lock while we sleep to allow set_polling_interval / stop()
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lock.lock();

        auto last_write = fs::last_write_time(file_name);

        if (!change_detected)
        {
            // Haven't seen any write > org_time yet
            if (last_write > org_time.value())
            {
                change_detected = true;
                org_time = last_write;
                stable_checks = 0;      // start counting stability from here
            }
            // ELSE: still no change — keep waiting
            continue;
        }

        // Once we've detected a write, watch for stable intervals
        if (last_write > org_time.value())
        {
            // file changed again before stabilizing
            org_time = last_write;
            stable_checks = 0;
        }
        else
        {
            // file unchanged since last write
            if (++stable_checks >= 3 && last_write != last_reported_time)
            {
                last_reported_time = last_write;
                monitoring_state.store(MonitorState::FILE_CHANGED);

                // invoke callback outside the lock
                if (callback)
                {
                    lock.unlock();
                    callback();
                    lock.lock();
                }

                // reset for the next change
                monitoring_state.store(MonitorState::MONITORING);
                stable_checks = 0;
                change_detected = false;
            }
        }
    }
}
