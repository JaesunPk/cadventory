#include "ModelView.h"

#include <qboxlayout.h>

#include <QFileDialog>
#include <iostream>

#include "Model.h"
#include "GeometryBrowserDialog.h"
#include "ui_modelview.h"
#include "CADventory.h"
#include "ModelTagging.h"

ModelView::ModelView(int modelId, Model* model, QWidget* parent)
    : QDialog(parent), modelId(modelId), model(model) {
  ui.setupUi(this);
  currModel = model->getModelById(modelId);

  geometryBrowser = new GeometryBrowserDialog(modelId, model, this);
  geometryBrowser->setWindowFlags(Qt::Widget);
  ui.geometryLayout->addWidget(geometryBrowser);

  ui.modelName->setText(QString::fromStdString(currModel.short_name));

  loadPreviewImage();
  populateProperties();
  populateTags();

  // Connect signals
  connect(ui.addTagButton, &QPushButton::clicked, this,
          &ModelView::onAddTagClicked);
  connect(ui.valuesList, &QListWidget::itemChanged, this,
          &ModelView::onPropertyChanged);
  connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
          this, &ModelView::onOkClicked);
  connect(ui.generateTagsButton, &QPushButton::clicked, this, &ModelView::onGenerateTagsClicked);

}

void ModelView::loadPreviewImage() {
  QPixmap thumbnail;
  thumbnail.loadFromData(
      reinterpret_cast<const uchar*>(currModel.thumbnail.data()),
      currModel.thumbnail.size());
  thumbnail = thumbnail.scaled(ui.previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  ui.previewLabel->setPixmap(thumbnail);
}

void ModelView::populateProperties() {
  ui.keysList->clear();
  ui.valuesList->clear();
  int i = 0;

  for (const auto& [key, value] : model->getPropertiesForModel(modelId)) {
    std::vector<std::string> editableProperties = {"short_name", "author"};

    std::cout << "Key: " << key << ":: Value: " << value << "  i" << i++
              << std::endl;
    QListWidgetItem* keyItem =
        new QListWidgetItem(QString::fromStdString(key), ui.keysList);
    QString displayValue =
        value.empty() ? "(unknown)" : QString::fromStdString(value);
    QListWidgetItem* valueItem =
        new QListWidgetItem(displayValue, ui.valuesList);
    keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
    if (std::find(editableProperties.begin(), editableProperties.end(), key) != editableProperties.end()) {
      valueItem->setFlags(valueItem->flags() | Qt::ItemIsEditable);
      valueItem->setForeground(Qt::blue);
    }
  }
}

void ModelView::onPropertyChanged(QListWidgetItem* item) {
  int row = ui.valuesList->row(item);  // Get the row of the changed item
  QString key = ui.keysList->item(row)->text();  // Get the corresponding key
  QString value = item->text();                  // Get the new value

  // Update the properties map and the model
  properties[key] = value;
  model->setPropertyForModel(modelId, key.toStdString(), value.toStdString());
}

void ModelView::populateTags() {
  ui.tagsList->clear();
  std::vector<std::string> gottenTags = model->getTagsForModel(modelId);
  for (const std::string& tag : gottenTags) {
    addTagItem(QString::fromStdString(tag));
  }
}

void ModelView::addTagItem(const QString& tagText) {
  QListWidgetItem* item = new QListWidgetItem(ui.tagsList);
  QWidget* widget = new QWidget();
  QPushButton* removeButton = new QPushButton("x");
  removeButton->setFixedWidth(20);
  QLabel* tagLabel = new QLabel(tagText);
  QHBoxLayout* layout = new QHBoxLayout(widget);

  layout->addWidget(removeButton);
  layout->addWidget(tagLabel);
  layout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(layout);

  item->setSizeHint(widget->sizeHint());
  ui.tagsList->setItemWidget(item, widget);

  // Connect remove button
  connect(removeButton, &QPushButton::clicked, this,
          [this, item]() { onRemoveTagClicked(item); });
}

void ModelView::onAddTagClicked() {
  QString newTag = ui.newTagLine->text();
  if (!newTag.isEmpty()) {
    currModel.tags.push_back(newTag.toStdString());
    addTagItem(newTag);
    ui.newTagLine->clear();
  }
}

void ModelView::onRemoveTagClicked(QListWidgetItem* item) {
  int row = ui.tagsList->row(item);
  ui.tagsList->takeItem(row);
  delete item;
}

void ModelView::onOkClicked() {
  std::cout << "onOkClicked" << std::endl;
  model->updateModel(modelId, currModel);
  std::cout << "updated model" << std::endl;

  // Update currModel properties
  for (int i = 0; i < ui.valuesList->count(); ++i) {
    QListWidgetItem* keyItem = ui.keysList->item(i);
    QListWidgetItem* valueItem = ui.valuesList->item(i);
    QString key = keyItem->text();
    QString value = valueItem->text();

    model->setPropertyForModel(modelId, key.toStdString(), value.toStdString());
  }

  // Update tags
  model->removeAllTagsFromModel(modelId);
  for (int i = 0; i < ui.tagsList->count(); ++i) {
    QListWidgetItem* item = ui.tagsList->item(i);
    QWidget* widget = ui.tagsList->itemWidget(item);
    QLabel* tagLabel = widget->findChild<QLabel*>();
    QString tagText = tagLabel->text();
    model->addTagToModel(modelId, tagText.toStdString());
  }
}

void ModelView::onGenerateTagsClicked() {
    // generate tags
	CADventory* app = qobject_cast<CADventory*>(QCoreApplication::instance());
	ModelTagging* modelTagging = app->getModelTagging();
	//send filepath of model to generate tags
	std::vector<std::string> tags = modelTagging->generateTags(currModel.file_path);

	// convert tags to QString
    QStringList dummyTags;

	for (const std::string& tag : tags) {
		dummyTags.append(QString::fromStdString(tag));
	}

    for (const QString& tag : dummyTags) {
        // Avoid duplicates
        bool alreadyExists = false;
        for (int i = 0; i < ui.tagsList->count(); ++i) {
            QWidget* widget = ui.tagsList->itemWidget(ui.tagsList->item(i));
            QLabel* label = widget->findChild<QLabel*>();
            if (label && label->text() == tag) {
                alreadyExists = true;
                break;
            }
        }
        if (!alreadyExists) {
            currModel.tags.push_back(tag.toStdString());
            addTagItem(tag);
        }
    }
}


ModelView::~ModelView() { qDebug() << "ModelView destructor called"; }