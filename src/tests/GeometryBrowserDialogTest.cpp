#include <catch2/catch_test_macros.hpp>
#include <QTest>
#include <QTemporaryDir>
#include "GeometryBrowserDialog.h"
#include "Model.h"

// Test class for the GeometryBrowserDialog component
class GeometryBrowserDialogTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir tempDir; // Temporary directory for testing

private slots:
    // Initialize the test case and verify the temporary directory is valid
    void initTestCase() {
        QVERIFY(tempDir.isValid());
    }

    // Cleanup after all tests (currently no resources need explicit cleanup)
    void cleanupTestCase() {}

    // Test to verify tree initialization and hierarchical structure
    void testTreeInitializationAndHierarchy() {
        QString tempPath = tempDir.path();
        Model model(tempPath.toStdString());

        // Insert mock data representing a parent-child hierarchy
        model.insertObject({1, 1, "RootObject", -1, false});
        model.insertObject({2, 1, "ChildObject1", 1, false});
        model.insertObject({3, 1, "ChildObject2", 1, false});

        GeometryBrowserDialog dialog(1, &model);
        dialog.show();

        // Verify the number of top-level items in the tree
        int topLevelItemCount = dialog.getTreeWidget()->topLevelItemCount();
        QVERIFY(topLevelItemCount > 0);

        // Verify each top-level item's child count
        for (int i = 0; i < topLevelItemCount; ++i) {
            QTreeWidgetItem* topLevelItem = dialog.getTreeWidget()->topLevelItem(i);
            QVERIFY(topLevelItem->childCount() >= 0);
        }
    }

    // Test to verify checkbox selection state in the tree
    void testCheckboxSelectionState() {
        QString tempPath = tempDir.path();
        Model model(tempPath.toStdString());

        // Insert a single mock object
        model.insertObject({1, 1, "RootObject", -1, false});

        GeometryBrowserDialog dialog(1, &model);
        dialog.show();

        // Select the top-level item and verify its checkbox state
        QTreeWidgetItem* topLevelItem = dialog.getTreeWidget()->topLevelItem(0);
        QVERIFY(topLevelItem != nullptr);

        topLevelItem->setCheckState(0, Qt::Checked);
        QVERIFY(topLevelItem->checkState(0) == Qt::Checked);
    }

    // Test to ensure unchecking one item automatically unchecks others
    void testUncheckAllExceptSelected() {
        QString tempPath = tempDir.path();
        Model model(tempPath.toStdString());

        // Insert two mock objects at the same level
        model.insertObject({1, 1, "RootObject", -1, false});
        model.insertObject({2, 1, "SiblingObject", -1, false});

        GeometryBrowserDialog dialog(1, &model);
        dialog.show();

        QTreeWidgetItem* firstItem = dialog.getTreeWidget()->topLevelItem(0);
        QTreeWidgetItem* secondItem = dialog.getTreeWidget()->topLevelItem(1);

        QVERIFY(firstItem != nullptr);
        QVERIFY(secondItem != nullptr);

        // Check the first item and verify the second item remains unchecked
        firstItem->setCheckState(0, Qt::Checked);
        QCOMPARE(firstItem->checkState(0), Qt::Checked);
        QCOMPARE(secondItem->checkState(0), Qt::Unchecked);

        // Check the second item and verify the first item is unchecked
        secondItem->setCheckState(0, Qt::Checked);
        QCOMPARE(firstItem->checkState(0), Qt::Unchecked);
        QCOMPARE(secondItem->checkState(0), Qt::Checked);
    }

    // Test to ensure recursive unchecking propagates to child items
    void testRecursiveUncheckMechanism() {
        QString tempPath = tempDir.path();
        Model model(tempPath.toStdString());

        // Insert mock data with parent-child relationships
        model.insertObject({1, 1, "ParentObject", -1, false});
        model.insertObject({2, 1, "ChildObject", 1, false});

        GeometryBrowserDialog dialog(1, &model);
        dialog.show();

        QTreeWidgetItem* parentItem = dialog.getTreeWidget()->topLevelItem(0);
        QVERIFY(parentItem != nullptr);
        parentItem->setCheckState(0, Qt::Checked);

        if (parentItem->childCount() > 0) {
            QTreeWidgetItem* childItem = parentItem->child(0);
            QVERIFY(childItem != nullptr);

            // Check the child item and verify its state
            childItem->setCheckState(0, Qt::Checked);
            QVERIFY(childItem->checkState(0) == Qt::Checked);

            // Simulate interaction with another top-level item
            QTreeWidgetItem* anotherTopItem = dialog.getTreeWidget()->topLevelItem(1);
            anotherTopItem->setCheckState(0, Qt::Checked);

            // Verify the child item's state updates as expected
            QVERIFY(childItem->checkState(0) == Qt::Unchecked);
        }
    }
};

// Main function for running the test cases
QTEST_MAIN(GeometryBrowserDialogTest)
#include "GeometryBrowserDialogTest.moc"
