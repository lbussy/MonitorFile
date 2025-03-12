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
    fs::file_time_type last_reported_time;
    bool first_check_done = false;

    while (!stop_monitoring)
    {
        cv.wait_for(lock, polling_interval, [this]
                    { return stop_monitoring.load(); });

        if (stop_monitoring.load())
            return;

        if (!fs::exists(file_name))
        {
            monitoring_state.store(MonitorState::FILE_NOT_FOUND);
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow OS to update timestamp

        auto last_write = fs::last_write_time(file_name);

        // Ensure we properly detect the first modification
        if (!first_check_done && last_write > org_time.value())
        {
            first_check_done = true;
            org_time = last_write;
            continue;
        }

        if (last_write > org_time.value())
        {
            stable_checks = 0;
            org_time = last_write;
        }
        else
        {
            stable_checks++;
        }

        if (stable_checks >= 3 && last_write != last_reported_time)
        {
            last_reported_time = last_write;
            monitoring_state.store(MonitorState::FILE_CHANGED);

            if (callback)
            {
                lock.unlock();
                callback();
                lock.lock();
            }

            monitoring_state.store(MonitorState::MONITORING);
            stable_checks = 0;
        }
    }
}
