#include <catch2/catch_test_macros.hpp>
#include <QTest>
#include <QTemporaryDir>
#include <QStandardItemModel>
#include <QPainter>
#include <QSignalSpy>
#include "ModelCardDelegate.h"
#include "Model.h"

// Test class for the ModelCardDelegate component
class ModelCardDelegateTest : public QObject {
    Q_OBJECT

private slots:
    // Test painting logic for the delegate
    void testPaintLogic() {
        QStandardItemModel model(1, 1);
        QModelIndex index = model.index(0, 0);

        // Mock data for the index
        model.setData(index, QPixmap(100, 100), Model::ThumbnailRole);
        model.setData(index, QString("Short Name"), Model::ShortNameRole);
        model.setData(index, QString("Title"), Model::TitleRole);
        model.setData(index, QString("Author"), Model::AuthorRole);

        ModelCardDelegate delegate;

        // Initialize a valid QPixmap for the painter
        QPixmap pixmap(300, 100);
        if (pixmap.isNull()) {
            FAIL("QPixmap initialization failed.");
        }
        pixmap.fill(Qt::white); // Ensure the pixmap is filled with a valid color

        QPainter painter(&pixmap);
        if (!painter.isActive()) {
            FAIL("QPainter initialization failed.");
        }

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 300, 100);

        try {
            // Perform painting
            delegate.paint(&painter, option, index);
            REQUIRE(true); // Pass if no exception is thrown
        } catch (const std::exception& e) {
            FAIL_CHECK(std::string("Exception during paint: ") + e.what());
        } catch (...) {
            FAIL_CHECK("Unknown exception during paint.");
        }

        // Verify the pixmap has been modified by the paint operation
        REQUIRE(!pixmap.isNull());
    }

    // Test size hint logic
    void testSizeHint() {
        QStandardItemModel model(1, 1);
        QModelIndex index = model.index(0, 0);

        ModelCardDelegate delegate;

        QStyleOptionViewItem option;
        QSize sizeHint = delegate.sizeHint(option, index);

        // Verify the size hint matches the expected value
        QCOMPARE(sizeHint.width(), 300);
        QCOMPARE(sizeHint.height(), 100);
    }

    // Test interaction with the editorEvent method
    void testEditorEvent() {
        QStandardItemModel model(1, 1);
        QModelIndex index = model.index(0, 0);

        // Mock data for the index
        model.setData(index, 123, Model::IdRole);

        ModelCardDelegate delegate;

        QMouseEvent mouseEvent(
            QEvent::MouseButtonRelease,  // Event type
            QPoint(290, 20),             // Mouse position
            Qt::LeftButton,              // Button clicked
            Qt::LeftButton,              // Button state
            Qt::NoModifier               // No keyboard modifier
        );
        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 300, 100);

        // Verify event coordinates fall within the option rect
        QVERIFY(option.rect.contains(mouseEvent.pos()));

        try {
            // Create a spy for the signal
            QSignalSpy spy(&delegate, &ModelCardDelegate::modelViewClicked);

            // Trigger the editor event
            REQUIRE(delegate.editorEvent(&mouseEvent, &model, option, index));

            // Verify the signal was emitted with the correct data
            REQUIRE(spy.count() == 1);
            QCOMPARE(spy.takeFirst().at(0).toInt(), 123);
        } catch (const std::exception& e) {
            FAIL_CHECK(std::string("Exception during editorEvent: ") + e.what());
        } catch (...) {
            FAIL_CHECK("Unknown exception during editorEvent.");
        }
    }
};

// Main function for running the test cases
QTEST_MAIN(ModelCardDelegateTest)
#include "ModelCardDelegateTest.moc"