// Updated Test File: ProcessGFilesTests.cpp

#include <catch2/catch_test_macros.hpp>
#include "../ProcessGFiles.h"
#include "../Model.h"
#include <filesystem>
#include <memory>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <fstream>       // Added to use std::ifstream

// Base path for testing
const std::string TEST_LIBRARY_PATH = "./temp_test_library";

// Setup function to create necessary directories
void setupTestLibraryPath() {
    std::filesystem::create_directories(TEST_LIBRARY_PATH + "/.cadventory");
}

// Helper function to clean up test data (if needed)
void cleanupTestLibraryPath() {
    std::filesystem::remove_all(TEST_LIBRARY_PATH);
}

// Helper function to create a test ModelData object
ModelData createTestModelData(int id, const std::string& shortName, const std::string& filePath) {
    return ModelData{
        id,                 // id
        shortName,          // short_name
        filePath,           // primary_file
        "",                 // override_info
        "",                 // title
        {},                 // thumbnail (empty vector<char>)
        "Author Name",      // author (std::string)
        TEST_LIBRARY_PATH,  // file_path (library path)
        "Unknown Library",  // library_name
        false,              // is_selected
        false,              // is_processed
        false,              // is_included
        {}                  // tags
    };
}

// Test initialization and object creation
TEST_CASE("ProcessGFiles - Initialization", "[ProcessGFiles]") {
    setupTestLibraryPath();
    auto model = std::make_unique<Model>(TEST_LIBRARY_PATH, nullptr);
    ProcessGFiles processor(model.get());

    REQUIRE(model != nullptr);
    REQUIRE(&processor != nullptr);

    cleanupTestLibraryPath();
}

// Test processing a file (using `annual_gift_man.g`)
TEST_CASE("ProcessGFiles - File Processing with annual_gift_man.g", "[ProcessGFiles]") {
    setupTestLibraryPath();
    auto model = std::make_unique<Model>(TEST_LIBRARY_PATH, nullptr);
    ProcessGFiles processor(model.get());

    // Adjust the path to where your test .g files are located
    std::string testFilePath = "../src/tests/annual_gift_man.g";

    // Create a ModelData object with correct fields
    ModelData modelData = createTestModelData(1, "annual_gift_man", testFilePath);

    SECTION("Process annual_gift_man.g file") {
        if (std::filesystem::exists(modelData.primary_file)) {
            processor.processGFile(modelData);

            // Verify that the model is marked as processed
            REQUIRE(modelData.is_processed == true);

            // Verify thumbnail generation
            REQUIRE(!modelData.thumbnail.empty());

            // Verify that the title is extracted
            REQUIRE(!modelData.title.empty());
        } else {
            WARN("annual_gift_man.g file not found, skipping test");
        }
    }

    cleanupTestLibraryPath();
}

// Test generating a gist report and checking for .pdf output
TEST_CASE("ProcessGFiles - Generate Gist Report and Check PDF Output", "[ProcessGFiles]") {
    setupTestLibraryPath();
    auto model = std::make_unique<Model>(TEST_LIBRARY_PATH, nullptr);
    ProcessGFiles processor(model.get());

    std::string inputFilePath = "../src/tests/annual_gift_man.g";
    std::string outputFilePath = TEST_LIBRARY_PATH + "/annual_gist_report.pdf";  // Expecting a .pdf output
    std::string primaryObject = "all";
    std::string label = "Test Label";

    SECTION("Generate gist report and check for PDF output") {
        if (std::filesystem::exists(inputFilePath)) {
            auto [success, errorMessage, command] = processor.generateGistReport(
                inputFilePath, outputFilePath, primaryObject, label
                );

            // Verify that the gist report was generated successfully
            REQUIRE(success == true);
            REQUIRE(errorMessage.empty());

            // Check for output file existence
            if (std::filesystem::exists(outputFilePath)) {
                REQUIRE(std::filesystem::file_size(outputFilePath) > 0);

                // Optionally, verify that the output file is indeed a PDF
                std::ifstream file(outputFilePath, std::ios::binary);
                char buffer[5];
                file.read(buffer, 4);
                buffer[4] = '\0';
                std::string header(buffer);
                REQUIRE(header == "%PDF");
            } else {
                FAIL("Gist report PDF output file not generated");
            }
        } else {
            WARN("annual_gift_man.g file not found, skipping test");
        }
    }

    // Clean up the generated PDF file
    if (std::filesystem::exists(outputFilePath)) {
        std::filesystem::remove(outputFilePath);
    }

    cleanupTestLibraryPath();
}
