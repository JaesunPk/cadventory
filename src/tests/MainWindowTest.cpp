#include <QtTest/QtTest>
#include <QApplication>
#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QTemporaryDir>
#include <QDebug>
#include <filesystem>

#include "../MainWindow.h"
#include "../LibraryWindow.h"
#include "../Library.h"
#include "../Model.h"
#include "../ModelFilterProxyModel.h"
#include "../ModelCardDelegate.h"
#include "../IndexingWorker.h"
#include "../FilesystemIndexer.h"
#include "../ProcessGFiles.h"
#include "../GeometryBrowserDialog.h"

// const std::string TEST_LIBRARY_PATH = "temp_test_library";  // Base path for testing

// void setupTestLibraryPath() {
//     std::filesystem::create_directories(TEST_LIBRARY_PATH + "/.cadventory");
// }

class MainWindowTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();       // Set up any common resources before tests
    void cleanupTestCase();    // Clean up resources after tests

    void testInitialization();
    void testAddLibrary();
    void testClearLibraries();
    void testSaveAndLoadState();
    void testAddLibraryButtonClick();
    void testStatusLabelUpdate();

private:
    MainWindow *mainWindow;
};

void MainWindowTest::initTestCase() {
    //setupTestLibraryPath();  // Ensure test directory structure is set up

    QSettings settings;
    settings.clear();
    settings.sync();
    mainWindow = new MainWindow();
}

void MainWindowTest::cleanupTestCase() {
    delete mainWindow;
    //std::filesystem::remove_all(TEST_LIBRARY_PATH);  // Clean up test directory after tests
}

void MainWindowTest::testInitialization() {
    QCOMPARE(mainWindow->getLibraries().size(), size_t(0));
}

void MainWindowTest::testAddLibrary() {
    mainWindow->addLibrary("Test Library", BRLCAD_BUILD);
    QCOMPARE(mainWindow->getLibraries().size(), size_t(1));
    QCOMPARE(QString(mainWindow->getLibraries()[0]->name()), QString("Test Library"));
}

void MainWindowTest::testClearLibraries() {
    mainWindow->addLibrary("Library 1", BRLCAD_BUILD);
    //mainWindow->addLibrary("Library 2", "/path/to/library2");
    mainWindow->clearLibraries();
    QCOMPARE(mainWindow->getLibraries().size(), size_t(0));
}

void MainWindowTest::testSaveAndLoadState() {
    mainWindow->clearLibraries();
    mainWindow->addLibrary("Persistent Library", BRLCAD_BUILD);
    mainWindow->publicSaveState();

    mainWindow->clearLibraries();  // Clear and then load
    QCOMPARE(mainWindow->getLibraries().size(), size_t(0));

    mainWindow->publicLoadState();
    QCOMPARE(mainWindow->getLibraries().size(), size_t(1));
    QCOMPARE(QString(mainWindow->getLibraries()[0]->name()), QString("Persistent Library"));
}

void MainWindowTest::testAddLibraryButtonClick() {
    mainWindow->clearLibraries();
    //QMetaObject::invokeMethod(mainWindow, "on_addLibraryButton_clicked");
    QCOMPARE(mainWindow->getLibraries().size(), size_t(0));
}

void MainWindowTest::testStatusLabelUpdate() {
    mainWindow->updateStatusLabel("Status Updated");
    QCOMPARE(mainWindow->getStatusLabel(), QString("Status Updated"));
}

QTEST_MAIN(MainWindowTest)
#include "MainWindowTest.moc"
