# CADventory

CADventory is a 3D CAD file, categorical tagging, and associated
metadata inventory management system.

It is tested on Mac, Linux, and Windows, includes code coverage
testing, and code security testing.


## Installation

1) Install: CMake 3.25+, SQLite3, BRLCAD, and Qt6
2) Clone/Install BRLCAD from https://github.com/BRL-CAD/brlcad.git
3) Clone/Download CADventory from source
4) Compile (see .gitlab-ci.yml for variations):
     mkdir .build
     cd .build
     cmake .. -DCMAKE_INSTALL_PREFIX=/path -DCMAKE_BUILD_TYPE=Release -DQt6_DIR=/path/to/qt6 -DBRLCAD_ROOT=/path/to/BRLCAD
     cmake --build . --config Release

## Usage

Run (Mac and Linux):
  ./bin/cadventory
Run (Windows):
  .\Release\bin\cadventory.exe

Click + in GUI and navigate to a folder to index.

To clear all settings, run with --no-gui command-line option.

## Testing 

1. Create build directory from root of the project (mkdir build)
2. Navigate to build directory (cd build)
3. cmake -DCMAKE_BUILD_TYPE=Debug -DBRLCAD_DIR=/path/to/brlcad/build ..
4. make
5. ctest --output-on-failure
6. lcov --capture --directory . --output-file coverage.info
7. lcov --remove coverage.info '/usr/*' --output-file coverage_filtered.info
8. genhtml coverage_filtered.info --output-directory coverage_report
9. view coverage_report/index.html 

## Automated Test Suite via Python Script

1. Dependencies:

  Install CMake:
    On macOS: brew install cmake
    On Ubuntu: sudo apt install cmake
    On Windows: Install from the CMake website.
  Install LCOV and GenHTML:
    On macOS: brew install lcov
    On Ubuntu: sudo apt install lcov
    On Windows: Install LCOV and GenHTML through Cygwin or equivalent.
  Ensure make or mingw32-make is available.

2. Environment Variable for BRLCAD_ROOT:

  You can specify the BRLCAD_ROOT path interactively when the script runs.

3. Cross-Platform Commands:

  make: Uses mingw32-make on Windows.
  File Paths: Adapts to Linux/macOS vs. Windows paths (/usr/* vs. C:/Program Files/*).
  Opening HTML Reports: Uses start, open, or xdg-open based on the OS.

4. Testing Workflow:

  Compiles the application.
  Runs unit tests with CTest.
  Generates and filters a coverage report.
  Opens the report in the default browser.

5. Running the Script
  Ensure Python is installed (version 3.6+).
  Run it in your project directory (cadventory):
    python run_tests.py or python3 run_tests.py

## Roadmap

1) Implement automatic PDF report generation (1-page per library model).
2) Integrate metadata support into GUI for tagging primary models.
3) Graphically depict models and iconography during GUI browsing.
4) Implement support for creating & modifying collections manually.
5) Implement keyword search filtering for custom inventory reporting.


## Design

Here's an architecture diagram:

```
┌──────────────────────────────────────────────────────────────────────┐
│                               CADventory                             │
│                                                                      │
│    ┌──────────────────────────────────────────────────────────────┐  │
│    │                      User Interface (Qt)                     │  │
│    └─────────────┬───────────────────────┬─────────────────┬──────┘  │
│                  │                       │                 │         │
│    ┌─────────────▼───────────┐  ┌────────▼─────────┐  ┌────▼──────┐  │
│    │  Filesystem Processor   │  │ SQLite or JSON   │◄─│ Report    │  │
│    └─────────────┬───────────┘  │ Storage Manager  │  │ Generator │  │
│                  │              └──────────────────┘  └────┬──────┘  │
│                  │                                         │         │
│    ┌─────────────▼───────────┐                             │         │
│    │ Geometry/Image/Document │                             │         │
│    │ Handler                 │                             │         │
│    └─────────────┬───────────┘                             │         │
│                  │                                         │         │
│    ┌─────────────▼────────┐                                │         │
│    │     CAD Libraries    │◄───────────────────────────────┘         │
│    └──────────────────────┘                                          │
└──────────────────────────────────────────────────────────────────────┘
```


## License

MIT

