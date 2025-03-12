/**
 * @file main.cpp
 * @brief A program to test MonitorFile, a C++ class to monitor a file in the
 *        filesystem and allow triggering an action when it changes.
 *
 * This software is distributed under the MIT License. See LICENSE.md for details.
 *
 * Copyright (C) 2025 Lee C. Bussy (@LBussy). All rights reserved.
 */

#include "monitorfile.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

void debugPause()
{
    std::cout << "Press any key to continue..." << std::flush;
    std::cin.get(); // Wait for key press
}

/// Global instance of MonitorFile
MonitorFile monitor;

namespace fs = std::filesystem;

/// Atomic flag to control program execution
std::atomic<bool> running(false);

/**
 * @brief Callback handler for file monitor.
 *
 * This function is called when the monitored file changes.
 */
void onFileChanged()
{
    std::cout << "[Callback] File has changed." << std::endl;
}

/**
 * @brief Signal handler for graceful shutdown.
 *
 * Captures termination signals (e.g., SIGINT) and updates the running flag.
 *
 * @param signum The signal number received.
 */
void signalHandler(int signum)
{
    std::cout << "Caught signal " << signum << ", stopping gracefully." << std::endl;
    running = false;
}

/**
 * @brief Simulated worker thread function.
 *
 * Each worker runs a computational loop while the `running` flag is true.
 *
 * @param id Unique identifier for the worker thread.
 */
void workerThread(int id)
{
    while (running)
    {
        for (volatile int i = 0; i < 1000000; ++i)
            ;

        if (id == 0) // Log only from one worker to reduce spam
        {
            std::cout << "[Worker " << id << "] Still running." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

/**
 * @brief Create or update the DTS of a file.
 *
 * Creates the file if it does not existm otherwise does a "touch."
 *
 * @param filename Path (optional) and filename to be used.
 */
void touchFile(const std::string &filename)
{
    using namespace std::filesystem;
    using namespace std::chrono;

    // Ensure the file exists before updating its timestamp
    if (!exists(filename))
    {
        std::ofstream file(filename); // Create the file
        file.close();                 // Close it properly
        std::cout << "[Touch   ] File created." << std::endl;
    }
    else
    {
        // Force update the last modified timestamp
        last_write_time(filename, file_time_type::clock::now());
        std::cout << "[Touch   ] File timestamp updated." << std::endl;
    }
}

/**
 * @brief Main function of the program.
 *
 * Initializes the file monitor, starts worker threads, and waits for termination.
 *
 * @return 0 on successful execution, nonzero on failure.
 */
int main()
{
    // Register signal handler for graceful exit
    std::signal(SIGINT, signalHandler);

    const std::string testFileName = "testfile.txt";

    // Start worker threads first
    std::vector<std::thread> workers;
    const int num_workers = 4;

    std::cout << "[Main    ] Launching worker threads." << std::endl;
    for (int i = 0; i < num_workers; ++i)
    {
        workers.emplace_back(workerThread, i);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Allow threads to start

    // Ensure the file exists before starting monitoring
    touchFile(testFileName);

    // Start monitoring the file
    MonitorState state = monitor.filemon(testFileName, onFileChanged);

    if (state == MonitorState::FILE_NOT_FOUND)
    {
        std::cout << "[Main    ] File not found. Monitoring not started." << std::endl;
        running = false;
    }
    else
    {
        std::cout << "[Main    ] Monitoring file: " << testFileName << std::endl;
        running = true;
    }

    // Modify the test file after starting monitoring to ensure detection
    std::this_thread::sleep_for(std::chrono::seconds(3));
    touchFile(testFileName);
    //
    std::this_thread::sleep_for(std::chrono::seconds(4));
    touchFile(testFileName);

    // Keep running until interrupted
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Graceful shutdown
    std::cout << "[Main    ] Stopping File Monitor." << std::endl;
    monitor.stop();

    std::cout << "[Main    ] Waiting for worker threads to finish." << std::endl;
    for (auto &worker : workers)
    {
        worker.join();
    }

    // Cleanup
    std::cout << "[Main    ] Cleaning up test file." << std::endl;
    std::filesystem::remove(testFileName);

    std::cout << "[Main    ] All threads stopped. Exiting." << std::endl;
    return 0;
}
