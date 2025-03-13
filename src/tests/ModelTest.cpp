// Catch2 is used for writing and running unit tests
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include "Model.h"
#include <filesystem>
#include <memory>

// Helper function to create a temporary test directory
std::string setupTestDirectory() {
    auto tempDir = std::filesystem::temp_directory_path() / "cadventory_test";
    if (!std::filesystem::exists(tempDir)) {
        std::filesystem::create_directories(tempDir);
    }
    return tempDir.string();
}

// Helper function to clean up the temporary test directory after tests
void cleanupTestDirectory(const std::string& path) {
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
}

// Tests for initializing the Model and performing basic CRUD operations
TEST_CASE("Model Initialization and CRUD Operations", "[Model]") {
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);  // Ensure a clean environment
    std::filesystem::create_directories(testDir);
    Model model(testDir);

    // Test if the database and its supporting directories are created successfully
    SECTION("Database Initialization") {
        REQUIRE(std::filesystem::exists(model.getHiddenDirectoryPath()));
        REQUIRE(std::filesystem::exists(testDir + "/.cadventory/metadata.db"));
    }

    // Test inserting, retrieving, and updating a model
    SECTION("Insert, Retrieve, and Update Model") {
        ModelData newModel = {0, "TestModel", "./path/to/file", "{}", "Test Title", {}, "Author", "/file/path", "Library", true, false, false, {}};
        REQUIRE(model.insertModel(newModel) == true);

        auto fetchedModel = model.getModelByFilePath(newModel.file_path);
        REQUIRE(fetchedModel.short_name == "TestModel");

        fetchedModel.short_name = "UpdatedModel";
        REQUIRE(model.updateModel(fetchedModel.id, fetchedModel) == true);

        auto updatedModel = model.getModelById(fetchedModel.id);
        REQUIRE(updatedModel.short_name == "UpdatedModel");
    }

    // Test deleting a model and verifying its deletion
    SECTION("Delete Model and Verify Deletion") {
        ModelData delModel = {0, "DeleteModel", "./delete/path", "{}", "Delete Title", {}, "Author", "/delete/file/path", "Library", false, false, false, {}};
        REQUIRE(model.insertModel(delModel) == true);

        auto fetchedModel = model.getModelByFilePath(delModel.file_path);
        REQUIRE(model.deleteModel(fetchedModel.id) == true);
        REQUIRE_FALSE(model.modelExists(fetchedModel.id));
    }
}

// Tests for verifying data roles and utility functions
TEST_CASE("Model Data Roles and Utility Functions", "[Model]") {
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);
    Model model(testDir);

    ModelData testModel = {0, "RoleTest", "./path/to/file", "{}", "Role Title", {}, "Author", "/file/path", "Library", true, false, false, {}};
    REQUIRE(model.insertModel(testModel) == true);

    QModelIndex index = model.index(0, 0);
    REQUIRE(index.isValid());

    // Verify role names mapping
    SECTION("Role Mapping") {
        auto roles = model.roleNames();
        REQUIRE(roles[Model::IdRole] == "id");
        REQUIRE(roles[Model::ShortNameRole] == "short_name");
    }

    // Test the data retrieval for specific roles
    SECTION("Verify Data for Roles") {
        REQUIRE(model.data(index, Model::ShortNameRole).toString().toStdString() == "RoleTest");
        REQUIRE(model.data(index, Model::TitleRole).toString().toStdString() == "Role Title");
    }
}

// Tests for managing objects associated with models
TEST_CASE("Object Management and Transactions", "[Model]") {
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);
    Model model(testDir);

    ModelData testModel = {0, "ObjectTest", "./path/to/file", "{}", "Object Title", {}, "Author", "/file/path", "Library", false, false, false, {}};
    REQUIRE(model.insertModel(testModel) == true);

    auto fetchedModel = model.getModelByFilePath(testModel.file_path);

    // Test inserting and retrieving objects
    SECTION("Insert and Retrieve Objects") {
        ObjectData obj1 = {0, fetchedModel.id, "Object1", -1, false};
        REQUIRE(model.insertObject(obj1) != -1);

        ObjectData obj2 = {0, fetchedModel.id, "Object2", 0, true};
        REQUIRE(model.insertObject(obj2) != -1);

        auto objects = model.getObjectsForModel(fetchedModel.id);
        REQUIRE(objects.size() == 2);
        REQUIRE(objects[0].name == "Object1");
    }

    // Test updating and deleting objects
    SECTION("Update and Delete Objects") {
        ObjectData obj = {0, fetchedModel.id, "ToUpdate", -1, false};
        int objId = model.insertObject(obj);
        REQUIRE(objId != -1);

        obj.name = "UpdatedObject";
        REQUIRE(model.updateObject(obj) == true);

        // Fetch the object again to validate the update
        auto updatedObj = model.getObjectById(objId);
        REQUIRE(updatedObj.object_id == objId);  // Ensure correct object is fetched
        REQUIRE(updatedObj.name == "ToUpdate");  // Validate the updated name

        // Delete objects for the model and verify
        REQUIRE(model.deleteObjectsForModel(fetchedModel.id) == true);
        REQUIRE(model.getObjectsForModel(fetchedModel.id).empty());
    }

    // Test transaction handling for object updates
    SECTION("Transaction Handling") {
        model.beginTransaction();
        REQUIRE(model.updateObjectSelection(0, true) == true);
        model.commitTransaction();

        auto selectedObjects = model.getSelectedObjectsForModel(fetchedModel.id);
        REQUIRE(selectedObjects.empty());
    }
}


// Test cases for advanced model features like handling tags
TEST_CASE("Advanced Model Features", "[Model]") {
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir); // Ensure clean environment
    std::filesystem::create_directories(testDir);

    Model model(testDir);

    // Verify adding and retrieving tags for a model
    SECTION("Handle Tags") {
        ModelData tagModel = {0, "TagModel", "./path", "{}", "Tag Title", {}, "Author", "/file/path", "Library", false, false, false, {}};
        REQUIRE(model.insertModel(tagModel) == true);

        auto modelId = model.getModelByFilePath(tagModel.file_path).id;
        REQUIRE(model.addTagToModel(modelId, "Tag1") == true);
        REQUIRE(model.addTagToModel(modelId, "Tag2") == true);

        auto tags = model.getTagsForModel(modelId);
        REQUIRE(tags.size() == 2); // Ensure two tags are added
    }
}

// Test cases for hashing functionality in the Model class
TEST_CASE("Model: Hashing Functionality", "[Model]") {
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Create dummy files for hashing tests
    std::string validFilePath = testDir + "/valid_file.txt";
    std::ofstream validFile(validFilePath);
    validFile << "Test content for hashing.";
    validFile.close();

    std::string emptyFilePath = testDir + "/empty_file.txt";
    std::ofstream emptyFile(emptyFilePath);
    emptyFile.close();

    Model model(testDir);

    // Test hashing a valid file
    SECTION("Hashing a Valid File") {
        int hashValue = model.hashModel(validFilePath);
        REQUIRE(hashValue != 0); // Ensure a valid hash is produced
    }

    // Test hashing an empty file
    SECTION("Hashing an Empty File") {
        int hashValue = model.hashModel(emptyFilePath);
        REQUIRE(hashValue != 0); // Hashing should still produce a valid value
    }

    // Test hashing a nonexistent file
    SECTION("Hashing a Nonexistent File") {
        std::string invalidFilePath = testDir + "/nonexistent.txt";
        int hashValue = model.hashModel(invalidFilePath);
        REQUIRE(hashValue == 0); // Nonexistent file should return a hash of 0
    }

    cleanupTestDirectory(testDir);
}

// Test cases for printing model details to output
TEST_CASE("Model: Print Functionality", "[Model]") {
    ModelData testModel = {1, "TestModel", "./primary/file", "{}", "Title", {}, "Author", "/path", "Library", true, false, false, {}};
    Model model("");

    // Ensure the printModel function runs without crashing
    SECTION("Print Model") {
        REQUIRE_NOTHROW(model.printModel(testModel));
    }
}

// Test cases for refreshing data and checking roles
TEST_CASE("Model: Refresh Data and Roles", "[Model]") {
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    Model model(testDir);

    // Verify that refreshing model data does not throw errors
    SECTION("Refresh Model Data") {
        REQUIRE_NOTHROW(model.refreshModelData());
    }

    // Ensure role names map to expected values
    SECTION("Role Names") {
        auto roles = model.roleNames();
        REQUIRE(roles[Model::IdRole] == "id");
        REQUIRE(roles[Model::ShortNameRole] == "short_name");
    }

    cleanupTestDirectory(testDir);
}

// Test cases for setting data and checking item flags
TEST_CASE("Model: Set Data and Flags", "[Model]") {
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    Model model(testDir);
    ModelData testModel = {0, "SelectableModel", "./file", "{}", "Title", {}, "Author", "/path", "Library", false, false, false, {}};
    REQUIRE(model.insertModel(testModel));

    QModelIndex index = model.index(0, 0);
    REQUIRE(index.isValid()); // Ensure the index is valid

    // Verify setting data for the IsSelectedRole role
    SECTION("Set Data for IsSelectedRole") {
        REQUIRE(model.setData(index, true, Model::IsSelectedRole) == true);
        REQUIRE(model.data(index, Model::IsSelectedRole).toBool() == true);
    }

    // Verify setting data for the IsIncludedRole role
    SECTION("Set Data for IsIncludedRole") {
        REQUIRE(model.setData(index, true, Model::IsIncludedRole) == true);
        REQUIRE(model.data(index, Model::IsIncludedRole).toBool() == true);
    }

    // Test invalid index handling
    SECTION("Invalid Index Handling") {
        QModelIndex invalidIndex;
        REQUIRE(model.setData(invalidIndex, true, Model::IsSelectedRole) == false);
    }

    // Verify item flags for valid and invalid indices
    SECTION("Item Flags") {
        auto flags = model.flags(index);
        REQUIRE(flags == (Qt::ItemIsEnabled | Qt::ItemIsSelectable));

        QModelIndex invalidIndex;
        REQUIRE(model.flags(invalidIndex) == Qt::NoItemFlags);
    }

    cleanupTestDirectory(testDir);
}

// Test case for retrieving models marked as "selected"
TEST_CASE("Model: Get Selected Models", "[Model]") {
    // Setup and cleanup temporary directory for the test
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Initialize model object with the test directory
    Model model(testDir);

    // Insert sample models into the database
    ModelData model1 = {0, "Model1", "./file1", "{}", "Title1", {}, "Author1", "/path1", "Library1", true, false, false, {}};
    ModelData model2 = {0, "Model2", "./file2", "{}", "Title2", {}, "Author2", "/path2", "Library2", false, false, false, {}};
    REQUIRE(model.insertModel(model1)); // Ensure model1 is inserted successfully
    REQUIRE(model.insertModel(model2)); // Ensure model2 is inserted successfully

    // Verify that only the "selected" model is retrieved
    SECTION("Retrieve Only Selected Models") {
        auto selectedModels = model.getSelectedModels();
        REQUIRE(selectedModels.size() == 1); // Only one model should be selected
        REQUIRE(selectedModels[0].short_name == "Model1"); // Verify the selected model
    }

    // Clean up after test execution
    cleanupTestDirectory(testDir);
}

// Test case for updating the parent ID of an object
TEST_CASE("Model: Update Object Parent ID", "[Model]") {
    // Setup and cleanup temporary directory for the test
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Initialize model object with the test directory
    Model model(testDir);

    // Insert a sample object into the database
    ObjectData object = {0, 1, "Object", -1, false}; // Object with no parent initially
    int objectId = model.insertObject(object);
    REQUIRE(objectId != -1); // Ensure the object is inserted successfully

    // Verify that the parent ID can be updated successfully
    SECTION("Update Parent ID Successfully") {
        REQUIRE(model.updateObjectParentId(objectId, 100) == true); // Update parent ID to 100

        auto updatedObject = model.getObjectById(objectId);
        REQUIRE(updatedObject.parent_object_id == 100); // Confirm the parent ID update
    }

    // Clean up after test execution
    cleanupTestDirectory(testDir);
}

// Test case for deleting and recreating database tables
TEST_CASE("Model: Delete Tables", "[Model]") {
    // Setup and cleanup temporary directory for the test
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Initialize model object with the test directory
    Model model(testDir);

    // Verify that tables can be deleted and recreated without errors
    SECTION("Successfully Delete and Recreate Tables") {
        REQUIRE(model.deleteTables() == true); // Tables deleted successfully
        REQUIRE_NOTHROW(model.resetDatabase()); // Tables recreated without exceptions
    }

    // Clean up after test execution
    cleanupTestDirectory(testDir);
}

// Test case for retrieving all tags in the database
TEST_CASE("Model: Get All Tags", "[Model]") {
    // Setup and cleanup temporary directory for the test
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Initialize model object with the test directory
    Model model(testDir);

    // Insert sample tags into the database
    REQUIRE(model.addTagToModel(1, "Tag1") == true); // Add "Tag1" to model with ID 1
    REQUIRE(model.addTagToModel(2, "Tag2") == true); // Add "Tag2" to model with ID 2

    // Verify that all tags are retrieved correctly
    SECTION("Retrieve All Tags") {
        auto tags = model.getAllTags();
        REQUIRE(tags.size() == 2); // Ensure two tags are retrieved
        REQUIRE(std::find(tags.begin(), tags.end(), "Tag1") != tags.end()); // Check for "Tag1"
        REQUIRE(std::find(tags.begin(), tags.end(), "Tag2") != tags.end()); // Check for "Tag2"
    }

    // Clean up after test execution
    cleanupTestDirectory(testDir);
}

// Test case for removing tags from a model
TEST_CASE("Model: Remove Tags from Model", "[Model]") {
    // Setup and cleanup temporary directory for the test
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Initialize model object with the test directory
    Model model(testDir);

    // Insert a sample model and add tags to it
    ModelData modelData = {0, "ModelWithTags", "./file", "{}", "Title", {}, "Author", "/path", "Library", false, false, false, {}};
    REQUIRE(model.insertModel(modelData)); // Insert model

    auto modelId = model.getModelByFilePath(modelData.file_path).id;
    REQUIRE(model.addTagToModel(modelId, "Tag1") == true); // Add "Tag1"
    REQUIRE(model.addTagToModel(modelId, "Tag2") == true); // Add "Tag2"

    // Verify that a specific tag can be removed
    SECTION("Remove a Specific Tag") {
        REQUIRE(model.removeTagFromModel(modelId, "Tag1") == true); // Remove "Tag1"

        auto tags = model.getTagsForModel(modelId);
        REQUIRE(tags.size() == 1); // Only one tag should remain
        REQUIRE(tags[0] == "Tag2"); // Verify remaining tag
    }

    // Verify that all tags can be removed
    SECTION("Remove All Tags") {
        REQUIRE(model.removeAllTagsFromModel(modelId) == true); // Remove all tags

        auto tags = model.getTagsForModel(modelId);
        REQUIRE(tags.empty()); // Ensure no tags remain
    }

    // Clean up after test execution
    cleanupTestDirectory(testDir);
}

// Test case for getting and setting model properties
TEST_CASE("Model: Get and Set Properties", "[Model]") {
    // Setup and cleanup temporary directory for the test
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Initialize model object with the test directory
    Model model(testDir);

    // Insert a sample model into the database
    ModelData modelData = {0, "PropertyModel", "./file", "{}", "Title", {}, "Author", "/path", "Library", false, false, false, {}};
    REQUIRE(model.insertModel(modelData)); // Ensure model is inserted successfully

    auto modelId = model.getModelByFilePath(modelData.file_path).id;

    // Verify that properties can be retrieved correctly
    SECTION("Get Properties for Model") {
        auto properties = model.getPropertiesForModel(modelId);
        REQUIRE(properties["short_name"] == "PropertyModel");
        REQUIRE(properties["title"] == "Title");
        REQUIRE(properties["author"] == "Author");
    }

    // Verify that a property can be updated successfully
    SECTION("Set a Property for Model") {
        REQUIRE(model.setPropertyForModel(modelId, "title", "New Title") == true); // Update the "title" property

        auto updatedProperties = model.getPropertiesForModel(modelId);
        REQUIRE(updatedProperties["title"] == "New Title"); // Confirm updated property
    }

    // Verify that attempting to set an invalid property fails
    SECTION("Fail to Set Invalid Property") {
        REQUIRE(model.setPropertyForModel(modelId, "invalid_property", "value") == false); // Invalid property
    }

    // Clean up after test execution
    cleanupTestDirectory(testDir);
}

// Test case for retrieving models marked as "included"
TEST_CASE("Model: Get Included Models", "[Model]") {
    // Setup and cleanup temporary directory for the test
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Initialize model object with the test directory
    Model model(testDir);

    // Insert sample models into the database
    ModelData model1 = {0, "IncludedModel1", "./file1", "{}", "Title1", {}, "Author1", "/path1", "Library1", false, false, true};
    ModelData model2 = {0, "ExcludedModel", "./file2", "{}", "Title2", {}, "Author2", "/path2", "Library2", false, false, false};
    ModelData model3 = {0, "IncludedModel2", "./file3", "{}", "Title3", {}, "Author3", "/path3", "Library3", false, false, true};
    REQUIRE(model.insertModel(model1));
    REQUIRE(model.insertModel(model2));
    REQUIRE(model.insertModel(model3));

    // Verify that only "included" models are retrieved
    SECTION("Retrieve Included Models") {
        auto includedModels = model.getIncludedModels();
        REQUIRE(includedModels.size() == 2); // Only two models are marked as "included"

        std::vector<std::string> includedNames;
        for (const auto& modelData : includedModels) {
            includedNames.push_back(modelData.short_name);
        }

        REQUIRE(std::find(includedNames.begin(), includedNames.end(), "IncludedModel1") != includedNames.end());
        REQUIRE(std::find(includedNames.begin(), includedNames.end(), "IncludedModel2") != includedNames.end());
        REQUIRE(std::find(includedNames.begin(), includedNames.end(), "ExcludedModel") == includedNames.end());
    }

    // Clean up after test execution
    cleanupTestDirectory(testDir);
}

// Test case for checking if a file is included in the model database
TEST_CASE("Model: Is File Included", "[Model]") {
    // Setup and cleanup temporary directory for the test
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Initialize model object with the test directory
    Model model(testDir);

    // Insert a sample model into the database
    ModelData modelData = {0, "IncludedFileModel", "./file_path", "{}", "Title", {}, "Author", "/path", "Library", false, false, true};
    REQUIRE(model.insertModel(modelData));

    // Verify that a file is correctly identified as "included"
    SECTION("Check Included File") {
        REQUIRE(model.isFileIncluded("/path") == true); // File is included
    }

    // Verify that a non-existent file is not identified as "included"
    SECTION("Check Non-Included File") {
        REQUIRE(model.isFileIncluded("/nonexistent_path") == false); // File is not included
    }

    // Clean up after test execution
    cleanupTestDirectory(testDir);
}

// Test case for retrieving models that are included but not processed
TEST_CASE("Model: Get Included Not Processed Models", "[Model]") {
    // Setup and cleanup temporary directory for the test
    std::string testDir = setupTestDirectory();
    cleanupTestDirectory(testDir);
    std::filesystem::create_directories(testDir);

    // Initialize model object with the test directory
    Model model(testDir);

    // Insert sample models into the database
    ModelData model1 = {0, "IncludedNotProcessed1", "./file1", "{}", "Title1", {}, "Author1", "/path1", "Library1", false, false, true};
    ModelData model2 = {0, "IncludedProcessed", "./file2", "{}", "Title2", {}, "Author2", "/path2", "Library2", false, true, true};
    ModelData model3 = {0, "ExcludedNotProcessed", "./file3", "{}", "Title3", {}, "Author3", "/path3", "Library3", false, false, false};
    REQUIRE(model.insertModel(model1));
    REQUIRE(model.insertModel(model2));
    REQUIRE(model.insertModel(model3));

    // Verify that only models marked as "included" and "not processed" are retrieved
    SECTION("Retrieve Included Not Processed Models") {
        auto notProcessedModels = model.getIncludedNotProcessedModels();
        REQUIRE(notProcessedModels.size() == 1); // Only one model matches the criteria
        REQUIRE(notProcessedModels[0].short_name == "IncludedNotProcessed1"); // Verify the model
    }

    // Clean up after test execution
    cleanupTestDirectory(testDir);
}