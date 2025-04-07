#include "LibraryWindow.h"
#include "IndexingWorker.h"
#include "ProcessGFiles.h"
#include "MainWindow.h"
#include "GeometryBrowserDialog.h"
#include "ModelView.h"
// #include "AdvancedOptionsDialog.h"
#include "ReportGenerationWindow.h"
#include "ReportGeneratorWorker.h"
#include "FileSystemFilterProxyModel.h"

#include <QThread>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListView>
#include <QPushButton>
#include <QComboBox>
#include <QMenuBar>
#include <QLineEdit>
#include <QLabel>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QDebug>

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <set>

#include "CADventory.h"
#include "ModelTagging.h"

namespace fs = std::filesystem;

LibraryWindow::LibraryWindow(QWidget* parent)
    : QWidget(parent),
    library(nullptr),
    mainWindow(nullptr),
    model(nullptr),
    availableModelsProxyModel(new ModelFilterProxyModel(this)),
    selectedModelsProxyModel(new ModelFilterProxyModel(this)),
    modelCardDelegate(new ModelCardDelegate(this)),
    indexingThread(nullptr),
    indexingWorker(nullptr)
{
    ui.setupUi(this);
}

LibraryWindow::~LibraryWindow() {
    qDebug() << "LibraryWindow destructor called";

    // Ensure the indexing thread is stopped if it wasn't already
    if (indexingThread && indexingThread->isRunning()) {
        qDebug() << "Waiting for indexingThread to finish in destructor";
        indexingWorker->stop();
        indexingThread->requestInterruption();
        indexingThread->quit();
        indexingThread->wait();
        qDebug() << "indexingThread finished in destructor";
    }

    // Delete indexingWorker and indexingThread if they exist
    if (indexingWorker) {
        delete indexingWorker;
        indexingWorker = nullptr;
        qDebug() << "indexingWorker deleted in destructor";
    }

    if (indexingThread) {
        delete indexingThread;
        indexingThread = nullptr;
        qDebug() << "indexingThread deleted in destructor";
    }
}

void LibraryWindow::loadFromLibrary(Library* _library) {
    library = _library;
    ui.currentLibrary->setText(library->name());

    // Load models from the library
    model = library->model;

    // Set the source model for proxy models
    availableModelsProxyModel->setSourceModel(model);
    selectedModelsProxyModel->setSourceModel(model);

    // Set initial filters
    availableModelsProxyModel->setFilterRole(Model::IsSelectedRole);
    availableModelsProxyModel->setFilterFixedString("0"); // Show unselected models

    selectedModelsProxyModel->setFilterRole(Model::IsSelectedRole);
    selectedModelsProxyModel->setFilterFixedString("1"); // Show selected models

    // Now that library is set, set up models and views
    setupModelsAndViews();

    // Set up connections
    setupConnections();

    // Start indexing to process any already included but unprocessed models
    startIndexing();


}

void LibraryWindow::startIndexing() {
    if (indexingThread && indexingThread->isRunning()) {
        if (indexingWorker) {
            indexingWorker->requestReindex();
        }
        return;
    }

    // Create the indexing worker and thread
    indexingThread = new QThread(this);
    indexingWorker = new IndexingWorker(library);

    indexingWorker->moveToThread(indexingThread);

    // Connect signals and slots
    connect(indexingThread, &QThread::started, indexingWorker, &IndexingWorker::process);
    connect(indexingWorker, &IndexingWorker::modelProcessed, this, &LibraryWindow::onModelProcessed);
    connect(indexingWorker, &IndexingWorker::progressUpdated, this, &LibraryWindow::onProgressUpdated);
    connect(indexingWorker, &IndexingWorker::finished, this, &LibraryWindow::onIndexingComplete);
    connect(indexingWorker, &IndexingWorker::finished, indexingThread, &QThread::quit);
    connect(indexingThread, &QThread::finished, indexingWorker, &QObject::deleteLater);
    connect(indexingThread, &QThread::finished, indexingThread, &QObject::deleteLater);

    // Start the indexing thread
    indexingThread->start();
}

void LibraryWindow::processNextFile() {
    if (currentFileIndex >= static_cast<int>(filesToTag.size())) {
        ui.statusLabel->setText("Tagging complete!");
        ui.generateAllTagsButton->setEnabled(true);

        ui.generateAllTagsButton->show();
        ui.generateAllTagsButton->setEnabled(true);

        ui.pauseButton->hide();
        ui.cancelButton->hide();
        return;
    }

    std::string filepath = filesToTag[currentFileIndex];
    currentFileIndex++;

    qDebug() << "Processing file:" << QString::fromStdString(filepath);

    CADventory* app = qobject_cast<CADventory*>(QCoreApplication::instance());
    ModelTagging* modelTagging = app->getModelTagging();
    modelTagging->generateTags(filepath);
}

void LibraryWindow::onTagsGeneratedFromBatch(const std::vector<std::string>& tags) {
    int fileIndex = currentFileIndex - 1;
    if (fileIndex < 0 || fileIndex >= (int)filesToTag.size()) {
        return;
    }
    std::string filepath = filesToTag[fileIndex];

    ModelData data = model->getModelByFilePath(filepath);
    int modelId = data.id;
    if (modelId != -1) {
        std::vector<std::string> existingTags = model->getTagsForModel(modelId);
        std::set<std::string> existing(existingTags.begin(), existingTags.end());
        for (const auto& tag : tags) {
            if (existing.find(tag) == existing.end()) {
                model->addTagToModel(modelId, tag);
            }
        }
    }

    int progress = ui.progressBar->value() + 1;
    ui.progressBar->setValue(progress);
    ui.statusLabel->setText(QString("Processed %1/%2")
        .arg(progress)
        .arg(ui.progressBar->maximum()));

    QTimer::singleShot(200, this, &LibraryWindow::processNextFile);
}

void LibraryWindow::onPauseTagGenerationClicked() {

}

void LibraryWindow::onCancelTagGenerationClicked() {

} 

void LibraryWindow::onGenerateAllTagsClicked() {
    // generates all tags for all models
    CADventory* app = qobject_cast<CADventory*>(QCoreApplication::instance());
    ModelTagging* modelTagging = app->getModelTagging();

    // dependency check
    if (!modelTagging->checkOllamaAvailability()) {
        QMessageBox::critical(this, "Missing Dependency", "Ollama is not installed or not available in PATH.");
        return;
    }

    if (!modelTagging->checkModelAvailability("llama3")) {
        QMessageBox::critical(this, "Missing Model", "The 'llama3' model is not available.\nRun: `ollama pull llama3`.");
        return;
    }

    // get all the paths
    std::vector<std::string> relativePaths = library->getModels();

	if (relativePaths.empty()) {
		QMessageBox::information(this, "No Models", "No models found in the library.");
		return;
	}

    ui.generateAllTagsButton->setEnabled(false);
    ui.generateAllTagsButton->hide();

    ui.pauseButton->show();
    ui.cancelButton->show();

    // debug print
	qDebug() << "Generating tags for the following models:";
	for (const std::string& rel : relativePaths) {
		qDebug() << QString::fromStdString(rel);
	}

    ui.progressBar->setMaximum(static_cast<int>(relativePaths.size()));
    ui.progressBar->setValue(0);
    ui.statusLabel->setText("Generating tags...");
    ui.generateAllTagsButton->setEnabled(false);

    filesToTag.clear();
    for (const auto& rel : relativePaths) {
        filesToTag.push_back(library->fullPath + "/" + rel);
    }
    currentFileIndex = 0;

    static bool connectedOnce = false;
    if (!connectedOnce) {
        connect(modelTagging, &ModelTagging::tagsGenerated,
            this, &LibraryWindow::onTagsGeneratedFromBatch);
        connectedOnce = true;
    }

    processNextFile();
}

void LibraryWindow::setMainWindow(MainWindow* mainWindow) {
    this->mainWindow = mainWindow;
    reload = new QAction(tr("&Reload"), this);

    this->mainWindow->editMenu->addAction(reload);
    connect(reload, &QAction::triggered, this, &LibraryWindow::reloadLibrary);
}

void LibraryWindow::setupModelsAndViews() {
    // Configure available models view
    ui.availableModelsView->setModel(availableModelsProxyModel);
    ui.availableModelsView->setItemDelegate(modelCardDelegate);
    ui.availableModelsView->setViewMode(QListView::ListMode);
    ui.availableModelsView->setFlow(QListView::TopToBottom);
    ui.availableModelsView->setWrapping(false);
    ui.availableModelsView->setResizeMode(QListView::Adjust);
    ui.availableModelsView->setSpacing(0);
    ui.availableModelsView->setUniformItemSizes(true);
    ui.availableModelsView->setSelectionMode(QAbstractItemView::NoSelection);
    ui.availableModelsView->setSelectionBehavior(QAbstractItemView::SelectRows);

    QSize itemSize = modelCardDelegate->sizeHint(QStyleOptionViewItem(), QModelIndex());
    ui.availableModelsView->setGridSize(QSize(0, itemSize.height()));

    // Configure selected models view
    ui.selectedModelsView->setModel(selectedModelsProxyModel);
    ui.selectedModelsView->setItemDelegate(modelCardDelegate);
    ui.selectedModelsView->setViewMode(QListView::ListMode);
    ui.selectedModelsView->setFlow(QListView::TopToBottom);
    ui.selectedModelsView->setWrapping(false);
    ui.selectedModelsView->setResizeMode(QListView::Adjust);
    ui.selectedModelsView->setSpacing(0);
    ui.selectedModelsView->setUniformItemSizes(true);
    ui.selectedModelsView->setSelectionMode(QAbstractItemView::NoSelection);
    ui.selectedModelsView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Set grid size for selected models view
    ui.selectedModelsView->setGridSize(QSize(1, itemSize.height()));

    // Setup file system model with checkboxes
    QString libraryPath = QString::fromStdString(library->fullPath);
    qDebug() << "Library Path in setupModelsAndViews:" << libraryPath;

    // Create the FileSystemModelWithCheckboxes
    fileSystemModel = new FileSystemModelWithCheckboxes(model, libraryPath, this);

    // Create and set up the proxy model to filter .g files
    fileSystemProxyModel = new FileSystemFilterProxyModel(libraryPath, this);
    fileSystemProxyModel->setSourceModel(fileSystemModel);
    fileSystemProxyModel->setRecursiveFilteringEnabled(true);

    // Set the model to the tree view
    ui.fileSystemTreeView->setModel(fileSystemProxyModel);

    // Set the root index of the view to the mapped root index
    QModelIndex rootIndex = fileSystemModel->index(libraryPath);
    QModelIndex proxyRootIndex = fileSystemProxyModel->mapFromSource(rootIndex);
    ui.fileSystemTreeView->setRootIndex(proxyRootIndex);
    qDebug() << "Set root index of fileSystemTreeView to proxyRootIndex.";

    // Hide columns other than the name
    for (int i = 1; i < fileSystemModel->columnCount(); ++i) {
        ui.fileSystemTreeView->hideColumn(i);
    }

    // Set uniform row heights for consistent appearance
    ui.fileSystemTreeView->setUniformRowHeights(true);

    // Set icon size (if desired)
    ui.fileSystemTreeView->setIconSize(QSize(24, 24));

    // Expand all nodes currently loaded
    ui.fileSystemTreeView->expandAll();

    // Connect signals
    connect(fileSystemModel, &QFileSystemModel::directoryLoaded,
            this, &LibraryWindow::onDirectoryLoaded);
    connect(fileSystemModel, &FileSystemModelWithCheckboxes::inclusionChanged,
            this, &LibraryWindow::onInclusionChanged);
}

void LibraryWindow::setupConnections() {

    ui.pauseButton->hide();
    ui.cancelButton->hide();

    // Connect search input
    connect(ui.searchLineEdit, &QLineEdit::textChanged,
            this, &LibraryWindow::onSearchTextChanged);
    connect(ui.searchFieldComboBox, &QComboBox::currentTextChanged,
            this, &LibraryWindow::onSearchFieldChanged);

    // Connect clicks on available models
    connect(ui.availableModelsView, &QListView::clicked,
            this, &LibraryWindow::onAvailableModelClicked);

    // Connect clicks on selected models
    connect(ui.selectedModelsView, &QListView::clicked,
            this, &LibraryWindow::onSelectedModelClicked);

    // Connect Generate Report button
    connect(ui.generateReportButton, &QPushButton::clicked,
            this, &LibraryWindow::onGenerateReportButtonClicked);

    // Connect geometry browser clicked signal
    connect(modelCardDelegate, &ModelCardDelegate::geometryBrowserClicked,
            this, &LibraryWindow::onGeometryBrowserClicked);

    connect(modelCardDelegate, &ModelCardDelegate::modelViewClicked,
            this, &LibraryWindow::onModelViewClicked);

	// Connect generate all tags button
	connect(ui.generateAllTagsButton, &QPushButton::clicked,
		this, &LibraryWindow::onGenerateAllTagsClicked);

	// Connect pause/cancel tag generation buttons
    connect(ui.pauseButton, &QPushButton::clicked,
        this, &LibraryWindow::onPauseTagGenerationClicked);
    connect(ui.cancelButton, &QPushButton::clicked,
        this, &LibraryWindow::onCancelTagGenerationClicked);

}

void LibraryWindow::onSearchTextChanged(const QString& text) {
    int role = ui.searchFieldComboBox->currentData().toInt();
    availableModelsProxyModel->setFilterRole(role);
    availableModelsProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    availableModelsProxyModel->setFilterFixedString(text);
}

void LibraryWindow::onSearchFieldChanged(const QString& field) {
    Q_UNUSED(field);
    // Update filter role based on selected field
    int role = ui.searchFieldComboBox->currentData().toInt();
    availableModelsProxyModel->setFilterRole(role);
    // Re-apply filter
    availableModelsProxyModel->invalidate();
}



void LibraryWindow::onAvailableModelClicked(const QModelIndex& index) {
    // Toggle selection state
    QModelIndex sourceIndex = availableModelsProxyModel->mapToSource(index);
    bool isSelected = model->data(sourceIndex, Model::IsSelectedRole).toBool();
    model->setData(sourceIndex, !isSelected, Model::IsSelectedRole);

    // Update filters
    availableModelsProxyModel->invalidate();
    selectedModelsProxyModel->invalidate();
}

void LibraryWindow::onSelectedModelClicked(const QModelIndex& index) {
    // Toggle selection state
    QModelIndex sourceIndex = selectedModelsProxyModel->mapToSource(index);
    bool isSelected = model->data(sourceIndex, Model::IsSelectedRole).toBool();
    model->setData(sourceIndex, !isSelected, Model::IsSelectedRole);

    // Update filters
    availableModelsProxyModel->invalidate();
    selectedModelsProxyModel->invalidate();
}

void LibraryWindow::onGenerateReportButtonClicked() {
    if (model->getSelectedModels().empty()) {
        QMessageBox::information(this, "Report",
                                 "No models selected for the report.");
        return;
    }

    ReportGenerationWindow* window =
        new ReportGenerationWindow(nullptr, model, library);
    window->show();
}

void LibraryWindow::onSettingsClicked(int modelId) {
    // Handle settings button click
    qDebug() << "Settings button clicked for model ID:" << modelId;
    // Implement settings dialog or other actions here
}

void LibraryWindow::onModelProcessed(int modelId) {
    Q_UNUSED(modelId);
    model->refreshModelData();
    availableModelsProxyModel->invalidate();
    selectedModelsProxyModel->invalidate();
}

void LibraryWindow::on_backButton_clicked() {
    qDebug() << "Back button clicked";

    if (indexingWorker) {
        qDebug() << "Requesting indexingWorker to stop";
        indexingWorker->stop();
        qDebug() << "indexingWorker->stop() called";
    } else {
        qDebug() << "indexingWorker is null or already deleted";
    }

    // Hide the LibraryWindow
    this->hide();
    qDebug() << "LibraryWindow hidden";

    // Show the MainWindow
    if (mainWindow) {
        this->mainWindow->editMenu->removeAction(reload);
        disconnect(reload, nullptr, nullptr, nullptr);
        this->mainWindow->returnCentralWidget();
        qDebug() << "MainWindow shown";
    } else {
        qDebug() << "mainWindow is null";
    }
}

void LibraryWindow::reloadLibrary() {
    std::string path = library->fullPath; //+ "/.cadventory/metadata.db";
    fs::path filePath(path);

    qDebug() << QString::fromStdString(filePath.string());

    // Check if the file exists
    if (fs::exists(filePath)) {

        qDebug() << "reloadLibrary is called" ;
        try {

                // Reload the library
                model->resetDatabase();
                model->refreshModelData();
                availableModelsProxyModel->invalidate();
                selectedModelsProxyModel->invalidate();
                fileSystemModel->refresh(); // Custom method to refresh the model
                this->loadFromLibrary(library);

        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    } else {
        std::cout << "'.cadventory' does not exist." << std::endl;
}

}
void LibraryWindow::onModelViewClicked(int modelId) {
    qDebug() << "Model view clicked for model ID:" << modelId;
    ModelView* modelView = new ModelView(modelId, model, this);
    modelView->exec();

}

void LibraryWindow::onGeometryBrowserClicked(int modelId) {
    qDebug() << "Geometry browser clicked for model ID:" << modelId;

    GeometryBrowserDialog* dialog = new GeometryBrowserDialog(modelId, model, this);
    dialog->exec();
}

void LibraryWindow::onProgressUpdated(const QString& currentObject, int percentage) {
    ui.progressBar->setValue(percentage);

    if (percentage >= 100) {
        ui.statusLabel->setText("Processing complete");
        ui.progressBar->setVisible(false);
    } else {
        ui.statusLabel->setText(QString("Processing: %1").arg(currentObject));
        ui.progressBar->setVisible(true);
    }
}

void LibraryWindow::onInclusionChanged(const QModelIndex& index, bool /*included*/) {
    Q_UNUSED(index);
    availableModelsProxyModel->invalidate();
    selectedModelsProxyModel->invalidate();

    startIndexing();
    model->refreshModelData();
}


void LibraryWindow::onIndexingComplete() {
    qDebug() << "Indexing complete";
    indexingThread = nullptr;
    indexingWorker = nullptr;

    // Refresh model data
    model->refreshModelData();
    availableModelsProxyModel->invalidate();
    selectedModelsProxyModel->invalidate();

    // Update filesystem view checkboxes
    fileSystemModel->dataChanged(fileSystemModel->index(0, 0),
                                 fileSystemModel->index(fileSystemModel->rowCount() - 1, 0),
                                 {Qt::CheckStateRole});
}

void LibraryWindow::onDirectoryLoaded(const QString& /*path*/) {
    fileSystemProxyModel->invalidate();
    ui.fileSystemTreeView->expandAll();
}

