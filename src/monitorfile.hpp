/**
 * @file monitorfile.hpp
 * @brief Header file for MonitorFile class - Monitors file changes.
 *
 * This software is distributed under the MIT License. See LICENSE.md for details.
 *
 * Copyright (C) 2025 Lee C. Bussy (@LBussy). All rights reserved.
 */

#ifndef _MONITORFILE_HPP
#define _MONITORFILE_HPP

#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <optional>
#include <condition_variable>
#include <functional>

namespace fs = std::filesystem;

/**
 * @brief Represents the possible states of the file monitoring process.
 */
enum class MonitorState
{
    NOT_MONITORING, ///< Monitoring has been stopped.
    MONITORING,     ///< Monitoring is active, file exists, no recent changes.
    FILE_NOT_FOUND, ///< The file does not exist.
    FILE_CHANGED    ///< The file was modified and has stabilized.
};

/**
 * @class MonitorFile
 * @brief Monitors a file for changes in a background thread.
 */
class MonitorFile
{
public:
    /**
     * @brief Constructs a MonitorFile object.
     */
    MonitorFile();

    /**
     * @brief Destroys the MonitorFile object.
     *
     * Ensures monitoring is stopped before destruction.
     */
    ~MonitorFile();

    /**
     * @brief Starts monitoring a specified file.
     *
     * @param fileName The name of the file to monitor.
     * @param cb Optional callback function to be executed when the file changes.
     * @return MonitorState::MONITORING if monitoring starts successfully,
     *         MonitorState::FILE_NOT_FOUND if the file does not exist.
     */
    MonitorState filemon(const std::string &fileName, std::function<void()> cb = nullptr);

    /**
     * @brief Stops the file monitoring thread.
     */
    void stop();

    /**
     * @brief Sets the polling interval for monitoring.
     *
     * @param interval The new polling interval in milliseconds.
     */
    void set_polling_interval(std::chrono::milliseconds interval);

    /**
     * @brief Gets the current state of the file monitor.
     *
     * @return The current monitoring state.
     */
    MonitorState get_state();

    /**
     * @brief Sets a callback function to be executed when the file changes.
     *
     * @param func The callback function.
     */
    void set_callback(std::function<void()> func);

private:
    /**
     * @brief Runs the monitoring loop to detect file changes.
     */
    void monitor_loop();

    std::string file_name;                      ///< Name of the file being monitored.
    std::optional<fs::file_time_type> org_time; ///< Last known modification time of the file.
    std::thread monitoring_thread;              ///< Thread for background monitoring.
    std::atomic<bool> stop_monitoring;          ///< Flag to indicate if monitoring should stop.
    std::chrono::milliseconds polling_interval; ///< Polling interval for file monitoring.
    std::atomic<MonitorState> monitoring_state; ///< Tracks the current monitoring state.
    std::function<void()> callback;             ///< Callback function executed on file change.
    mutable std::shared_mutex mutex;            ///< Protects shared data (allows multiple readers).
    std::condition_variable_any cv;             ///< Condition variable for thread synchronization.
    int stable_checks = 0;                      ///< Tracks the number of stable checks before detecting a change.
};

#endif // _MONITORFILE_HPP
