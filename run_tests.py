import os
import platform
import subprocess
import sys
import shutil

def run_command(command, description=""):
    """Run a shell command with error handling."""
    print(f"Running: {description or command}")
    result = subprocess.run(command, shell=True)
    if result.returncode != 0:
        print(f"Error during: {description or command}")
        sys.exit(1)


def check_dependencies():
    """Check if required dependencies are installed."""
    dependencies = {
        "cmake": "CMake",
        "lcov": "LCOV (for code coverage)",
        "genhtml": "genhtml (for coverage report generation)"
    }
    for cmd, name in dependencies.items():
        if not shutil.which(cmd):
            print(f"Error: {name} is not installed. Please install it and retry.")
            sys.exit(1)


def get_db_root():
    """Prompt the user to input the BRLCAD_ROOT directory."""
    print("Specify the BRLCAD_ROOT directory: ", end="")
    brlcad_root = input().strip()
    if not brlcad_root:
        print("Error: You must specify a BRLCAD_ROOT path.")
        sys.exit(1)
    if not os.path.exists(brlcad_root):
        print(f"Error: Specified BRLCAD_ROOT path does not exist: {brlcad_root}")
        sys.exit(1)
    return brlcad_root


def main():
    # Check platform
    os_type = platform.system().lower()
    print(f"Detected OS: {os_type}")

    # Check dependencies
    check_dependencies()

    # Get BRLCAD_ROOT path
    brlcad_root = get_db_root()

    # Directories and filenames
    build_dir = "build"
    coverage_info = "coverage.info"
    coverage_filtered = "coverage_filtered.info"
    coverage_report_dir = "coverage_report"

    # Step 1: Create or navigate to the build directory
    if not os.path.exists(build_dir):
        os.mkdir(build_dir)
    os.chdir(build_dir)

    # Step 2: Run CMake to configure the project
    cmake_command = f"cmake -DCMAKE_BUILD_TYPE=Debug -DBRLCAD_ROOT={brlcad_root} .."
    run_command(cmake_command, "Configuring the project with CMake")

    # Step 3: Build the project
    make_command = "make" if os_type != "windows" else "mingw32-make"
    run_command(make_command, "Building the project")

    # Step 4: Run tests using CTest
    run_command("ctest --output-on-failure", "Running tests with CTest")

    # Step 5: Generate code coverage information
    run_command(f"lcov --capture --directory . --output-file {coverage_info}", "Generating code coverage report")

    # Step 6: Filter out unnecessary files (e.g., system files)
    remove_filter = "/usr/*" if os_type != "windows" else "C:/Program Files/*"
    run_command(f"lcov --remove {coverage_info} '{remove_filter}' --output-file {coverage_filtered}", "Filtering coverage data")

    # Step 7: Generate an HTML report
    run_command(f"genhtml {coverage_filtered} --output-directory {coverage_report_dir}", "Creating HTML coverage report")

    # Step 8: Open the coverage report in the default browser
    report_path = os.path.join(coverage_report_dir, "index.html")
    if os_type == "windows":
        run_command(f"start {report_path}", "Opening coverage report in browser")
    elif os_type == "darwin":  # macOS
        run_command(f"open {report_path}", "Opening coverage report in browser")
    else:  # Linux
        run_command(f"xdg-open {report_path}", "Opening coverage report in browser")

    print("All steps completed successfully. Coverage report is ready.")


if __name__ == "__main__":
    main()