/**
 * @file main.cpp
 * @brief A program to test MonitorFile, a C++ class to monitor a file in the
 *        filesystem and allow triggering an action when it changes.
 *
 * This software is distributed under the MIT License. See LICENSE.md for
 * details.
 *
 * Copyright (C) 2025 Lee C. Bussy (@LBussy). All rights reserved.
 */

 #include "monitorfile.hpp"
 #include <iostream>
 #include <fstream>
 #include <filesystem>
 #include <chrono>
 #include <thread>
 #include <exception>

 namespace fs = std::filesystem;

 /**
  * @brief Main function for debugging and testing the MonitorFile class.
  */
 int main()
 {
     MonitorFile monitor;
     const std::string testFileName = "testfile.txt";

     // Create or modify the test file for demonstration purposes
     {
         std::ofstream testFile(testFileName);
         testFile << "Initial content.\n";
     } // File is closed when exiting this scope (RAII)

     // Attempt to monitor the file and handle errors
     try
     {
         monitor.filemon(testFileName);
     }
     catch (const std::exception &e)
     {
         std::cerr << "Error: " << e.what() << std::endl;
         return 1; // Exit if file cannot be monitored
     }

     std::cout << "Monitoring file: " << testFileName << "\n";

     for (int i = 0; i < 5; ++i)
     {
         std::this_thread::sleep_for(std::chrono::seconds(3));

         // Modify the file to trigger a change
         if (i % 2 == 1)
         {
             std::ofstream modifyFile(testFileName, std::ios::app);
             modifyFile << "Modification " << i << "\n";
         } // File is closed at the end of this block

         // Check for file changes and handle errors
         try
         {
             if (monitor.changed())
             {
                 std::cout << "File has been modified\n";
             }
             else
             {
                 std::cout << "No changes detected.\n";
             }
         }
         catch (const std::exception &e)
         {
             std::cerr << "Error: " << e.what() << std::endl;
             break; // Stop monitoring if an error occurs
         }
     }

     // Cleanup: Delete the test file if it exists
     if (fs::exists(testFileName))
     {
         fs::remove(testFileName);
     }
     else
     {
         std::cerr << "Warning: Test file already deleted.\n";
     }

     return 0;
 }
