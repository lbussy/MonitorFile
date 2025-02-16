# MonitorFile

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)

MonitorFile is a lightweight **C++ header-only library** for monitoring file changes in the filesystem. It provides an easy-to-use API to detect modifications to a file based on its **last write time**.

## 🚀 Features

- **Header-only** – No compilation required, just include `monitorfile.hpp` in your project.
- **Minimal dependencies** – Uses standard C++17 `<filesystem>`.
- **Exception-safe** – Handles missing or deleted files gracefully.
- **Cross-platform** – Works on **Linux**, **macOS**, and **Windows** (C++17 required).

---

## 📂 Repository Structure

``` text
MonitorFile/
│── src/                 # Source files
│   ├── monitorfile.hpp  # The header-only MonitorFile class
│   ├── main.cpp         # Test program for monitoring file changes
│   ├── Makefile         # Build system for testing
│── LICENSE.md           # MIT License
│── README.md            # Project documentation
```

---

## 🔧 Installation

MonitorFile is header-only, so no installation is required. Just copy `monitorfile.hpp` into your project.

To test the functionality, clone the repository and compile the test program:

```bash
git clone https://github.com/yourusername/MonitorFile.git
cd MonitorFile/src
make test
```

---

## 🛠 Usage

Include the Library

``` c++
#include "monitorfile.hpp"
```

Basic Example

``` c++
#include "monitorfile.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

int main()
{
    MonitorFile monitor;
    const std::string testFile = "testfile.txt";

    // Create a test file
    {
        std::ofstream file(testFile);
        file << "Initial content.\n";
    }

    // Start monitoring
    try
    {
        monitor.filemon(testFile);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Monitoring file: " << testFile << "\n";

    for (int i = 0; i < 5; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Modify the file
        if (i % 2 == 1)
        {
            std::ofstream file(testFile, std::ios::app);
            file << "Modification " << i << "\n";
        }

        try
        {
            if (monitor.changed())
            {
                std::cout << "File has been modified!\n";
            }
            else
            {
                std::cout << "No changes detected.\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            break;
        }
    }

    return 0;
}
```

---

## 🏗 Building & Testing

To compile and run the test program:

``` bash
make test
```

To clean up compiled files:

``` bash
make clean
```

To run static analysis (requires `cppcheck`):

``` bash
make lint
```

---

## 🔒 Error Handling

MonitorFile throws std::runtime_error in these cases:

- The file does not exist when calling `filemon()`.
- The file is deleted while monitoring.

To prevent crashes, always wrap calls to `filemon()` and `changed()` in a try-catch block.

---

## 📜 License

This project is licensed under the MIT License. See [LICENSE.md](LICENSE.md) for details.

---

## 🤝 Contributing

Pull requests and feature suggestions are welcome.

To contribute:

1. Fork the repository.
2. Create a feature branch (git checkout -b feature-name).
3. Commit your changes (git commit -m "Add new feature").
4. Push to the branch (git push origin feature-name).
5. Open a Pull Request.

---

## 📬 Contact

- For issues, please open an Issue.
- For questions, reach out to Lee C. Bussy (@LBussy).
