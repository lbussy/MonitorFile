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
 #include <iostream>
 #include <thread>
 #include <vector>
 #include <atomic>
 #include <csignal>
 #include <fstream>
 
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
     std::cout << "[Monitor] File has changed." << std::endl;
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
         for (volatile int i = 0; i < 1000000; ++i);
         
         if (id == 0) // Log only from one worker to reduce spam
         {
             std::cout << "[Worker " << id << "] Still running." << std::endl;
         }
 
         std::this_thread::sleep_for(std::chrono::seconds(2));
     }
 }
 
 /**
  * @brief Function to modify the test file.
  *
  * Ensures a detectable change happens after monitoring starts.
  *
  * @param filename The name of the file to modify.
  */
 void modifyTestFile(const std::string &filename)
 {
     std::ofstream testFile(filename, std::ios::app); // Open in append mode
     if (testFile.is_open())
     {
         testFile << "Modifying file at " << std::time(nullptr) << std::endl;
         testFile.flush();
         testFile.close();
         std::cout << "[Test] File modified." << std::endl;
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
 
     std::cout << "[Main] Launching worker threads." << std::endl;
     for (int i = 0; i < num_workers; ++i)
     {
         workers.emplace_back(workerThread, i);
     }
 
     std::this_thread::sleep_for(std::chrono::seconds(1)); // Allow threads to start
 
     // Ensure the file exists before starting monitoring
     {
         std::ofstream testFile(testFileName, std::ios::app);
         testFile << "Initial content.\n";
         testFile.close();
     }
 
     // Start monitoring the file
     MonitorState state = monitor.filemon(testFileName, onFileChanged);
 
     if (state == MonitorState::FILE_NOT_FOUND)
     {
         std::cout << "[Main] File not found. Monitoring not started." << std::endl;
         running = false;
     }
     else
     {
         std::cout << "[Main] Monitoring file: " << testFileName << std::endl;
         running = true;
     }
 
     // Modify the test file after starting monitoring to ensure detection
     std::this_thread::sleep_for(std::chrono::seconds(3));
     modifyTestFile(testFileName);
 
     // Keep running until interrupted
     while (running)
     {
         std::this_thread::sleep_for(std::chrono::seconds(1));
     }
 
     // Graceful shutdown
     std::cout << "[Main] Stopping File Monitor." << std::endl;
     monitor.stop();
 
     std::cout << "[Main] Waiting for worker threads to finish." << std::endl;
     for (auto &worker : workers)
     {
         worker.join();
     }
 
     std::cout << "[Main] All threads stopped. Exiting." << std::endl;
     return 0;
 }
 