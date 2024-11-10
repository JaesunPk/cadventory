#ifndef LIBRARYWINDOW_H
#define LIBRARYWINDOW_H

#include <QWidget>
#include <QListView>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QThread>

#include "ui_librarywindow.h"
#include "Library.h"
#include "Model.h"
#include "ModelCardDelegate.h"
#include "ModelFilterProxyModel.h"
#include "IndexingWorker.h"
#include "FileSystemModelWithCheckboxes.h"
#include "FileSystemFilterProxyModel.h"


class MainWindow;

class LibraryWindow : public QWidget {
    Q_OBJECT

public:
    explicit LibraryWindow(QWidget* parent = nullptr);
    ~LibraryWindow();

    void loadFromLibrary(Library* _library);
    void reloadLibrary();
    void setMainWindow(MainWindow* mainWindow);
    MainWindow* mainWindow;
    QAction *reload;


private slots:
    void onSearchTextChanged(const QString& text);
    void onSearchFieldChanged(const QString& field);
    void onAvailableModelClicked(const QModelIndex& index);
    void onSelectedModelClicked(const QModelIndex& index);
    void onGenerateReportButtonClicked();
    void on_backButton_clicked();

    void onSettingsClicked(int modelId);
    void onGeometryBrowserClicked(int modelId);

    void startIndexing();
    void onModelProcessed(int modelId);
    void onProgressUpdated(const QString& currentObject, int percentage);

    // filesystem view slots
    void onFileSystemItemClicked(const QModelIndex& index);
    void onInclusionChanged(const QModelIndex& index, bool included);
    void onIncludeCheckBoxStateChanged(int state);
    void onReindexButtonClicked();
    void onIndexingComplete();
    void onDirectoryLoaded(const QString& path);




private:
    void setupModelsAndViews();
    void setupConnections();
    void includeAllModels();


    Ui::LibraryWindow ui;
    Library* library;


    Model* model;

    ModelFilterProxyModel* availableModelsProxyModel;
    ModelFilterProxyModel* selectedModelsProxyModel;

    ModelCardDelegate* modelCardDelegate;

    QList<int> selectedModelIds;

    // Indexing worker and thread
    QThread* indexingThread;
    IndexingWorker* indexingWorker;

    QFileSystemModel* fileSystemModel;
    FileSystemFilterProxyModel* fileSystemProxyModel;


};

#endif // LIBRARYWINDOW_H
