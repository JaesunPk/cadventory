#ifndef MODELVIEW_H
#define MODELVIEW_H

#include <QDialog>

#include "Model.h"
#include "GeometryBrowserDialog.h"
#include "ui_modelview.h"

class Model;

class ModelView : public QDialog {
  Q_OBJECT

 public:
  explicit ModelView(int modelId, Model* model, QWidget* parent = nullptr);
  ~ModelView();

 private slots:
  void onAddTagClicked();
  void onRemoveTagClicked(QListWidgetItem* item);
  void onPropertyChanged(QListWidgetItem* item);
  void onOkClicked();
  void onGenerateTagsClicked();

 private:
  void loadPreviewImage();
  void populateProperties();
  void populateTags();
  void addTagItem(const QString& tagText);

  Ui::ModelView ui;
  int modelId;
  ModelData currModel;
  Model* model;
  QMap<QString, QString> properties;
  QStringList tags;
  GeometryBrowserDialog* geometryBrowser;
};

#endif  // modelview_H
