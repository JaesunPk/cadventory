#ifndef CADVENTORY_H
#define CADVENTORY_H

#include <QApplication>
#include <QMainWindow>
#include <QSplashScreen>
#include <QObject>
#include "ModelTagging.h"


class CADventory : public QApplication
{
  Q_OBJECT

public:
  CADventory(int &argc, char *argv[]);
  ~CADventory();

  void showSplash();

  void indexDirectory(const char *path);

  ModelTagging* getModelTagging() { return modelTagging; }

signals:
  void indexingComplete(const char *summary);

private:
  ModelTagging* modelTagging;
  void initMainWindow();

public:
  QMainWindow *window;
  QWidget *splash;
  bool loaded;
  bool gui;
};

#endif /* CADVENTORY_H */
