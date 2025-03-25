
/* let catch provide main() */
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include "FilesystemIndexer.h"
#include <filesystem>
#include <fstream>


class FilesystemIndexerFixture {
public:
  std::filesystem::path testDir;
  FilesystemIndexer indexer;

  FilesystemIndexerFixture() {
    /* set up a little hierarchy */
    testDir = std::filesystem::temp_directory_path() / "FilesystemIndexerTest";
    std::filesystem::create_directories(testDir);
    std::ofstream(testDir / "test1.txt");
    std::ofstream(testDir / "test2.cpp");
    std::filesystem::create_directory(testDir / "subdir");
    std::ofstream(testDir / "subdir" / "test3.cpp");
    std::ofstream(testDir / "subdir" / "test4.h");
  }

  ~FilesystemIndexerFixture() {
    /* delete our test hierarchy */
    std::filesystem::remove_all(testDir);
  }
};


TEST_CASE_METHOD(FilesystemIndexerFixture, "IndexesCorrectNumberOfFiles", "[FilesystemIndexer]") {
  size_t filesIndexed = indexer.indexDirectory(testDir.string(), 2);
  REQUIRE(filesIndexed == 4);
}


TEST_CASE_METHOD(FilesystemIndexerFixture, "RespectsDepthParameter", "[FilesystemIndexer]") {
  size_t shallowIndex = indexer.indexDirectory(testDir.string(), 1);
  REQUIRE(shallowIndex == 2);
}


TEST_CASE_METHOD(FilesystemIndexerFixture, "FindsFilesWithGivenSuffixes", "[FilesystemIndexer]") {
  indexer.indexDirectory(testDir.string(), 2);
  auto cppFiles = indexer.findFilesWithSuffixes({".cpp"});
  REQUIRE(cppFiles.size() == 2);

  auto headerFiles = indexer.findFilesWithSuffixes({".h"});
  REQUIRE(headerFiles.size() == 1);
}


TEST_CASE_METHOD(FilesystemIndexerFixture, "HandlesNonExistentDirectoryGracefully", "[FilesystemIndexer]") {
  size_t filesIndexed = indexer.indexDirectory(testDir.string() + "/nonexistent", 2);
  REQUIRE(filesIndexed == 0);
}


TEST_CASE_METHOD(FilesystemIndexerFixture, "Depth -1 Traverses the Full Hierarchy", "[FilesystemIndexer]") {
  // -1 should index all files
  size_t filesIndexed = indexer.indexDirectory(testDir.string(), -1);
  REQUIRE(filesIndexed == 4);
}


TEST_CASE_METHOD(FilesystemIndexerFixture, "Depth 0 Does Not Traverse", "[FilesystemIndexer]") {
  // 0 should not index any files
  size_t filesIndexed = indexer.indexDirectory(testDir.string(), 0);
  REQUIRE(filesIndexed == 0);
}
