#include "CADventory.h"

#include <iostream>

#include <QPixmap>
#include <QTimer>
#include <QString>
#include <QDir>
#include <QSettings>

#include "MainWindow.h"
#include "SplashDialog.h"
#include "FilesystemIndexer.h"


CADventory::CADventory(int &argc, char *argv[]) : QApplication (argc, argv), window(nullptr), splash(nullptr), loaded(false), gui(true)
{
  setOrganizationName("BRL-CAD");
  setOrganizationDomain("brlcad.org");
  setApplicationName("CADventory");
  setApplicationVersion("0.1.0");

  QString appName = QCoreApplication::applicationName();
  QString appVersion = QCoreApplication::applicationVersion();

  // ANSI escape codes for underlining and reset
  QString underlineStart = "\033[4m";
  QString underlineEnd = "\033[0m";

  // print underlined application name and version
  qInfo().noquote() << underlineStart + appName + " " + appVersion + underlineEnd;
  qInfo() << "Loading ... please wait.";

  // if any arg is specified, assume CLI-mode
  if (argc > 1) {
    this->gui = false;
    connect(this, &CADventory::indexingComplete, this, &QCoreApplication::quit);

    // could be separate setting, but let CLI-mode also wipe out all settings
    QSettings settings;
    settings.clear();
    settings.sync();
  }
}


CADventory::~CADventory()
{
  delete window;
  delete splash;
}


void CADventory::initMainWindow()
{
  window = new MainWindow();

  connect(this, &CADventory::indexingComplete, static_cast<MainWindow*>(window), &MainWindow::updateStatusLabel);

  window->show();
  QSplashScreen *splat = dynamic_cast<QSplashScreen*>(splash);
  if (splat) {
    splat->finish(window);
    delete splat;
  } else {
    QDialog *diag = static_cast<QDialog*>(splash);
    delete diag;
  }

  splash = nullptr;
  qInfo() << "Done loading.";
}


void CADventory::showSplash()
{
  if (!this->gui)
    return;

  /* first look rel to binary */
  QString relativePathToBinary = QCoreApplication::applicationDirPath() + "/../share/splash.png";
  /* alternatively look rel to cwd */
  QString fallbackPath = QDir::current().absoluteFilePath("../src/splash.png");

  QPixmap pixmap;
  if (QFile::exists(relativePathToBinary)) {
    pixmap.load(relativePathToBinary);
  } else if (QFile::exists(fallbackPath)) {
    pixmap.load(fallbackPath);
  }

  if (pixmap.isNull()) {
    // pixmap = QPixmap(512, 512);
    // pixmap.fill(Qt::black);
    splash = new SplashDialog();
  } else {
    splash = new QSplashScreen(pixmap);
    static_cast<QSplashScreen*>(splash)->showMessage("Loading... please wait.", Qt::AlignLeft, Qt::black);
  }
  splash->show();
  // ensure the splash is displayed immediately
  this->processEvents();
}


void CADventory::indexDirectory(const char *path)
{
  qInfo() << "Indexing...";
  FilesystemIndexer f = FilesystemIndexer(path);

  f.setProgressCallback([this](const std::string& msg) {
    static size_t counter = 0;
    static const int MAX_MSG = 80;

    /* NOTE: only displaying every 1000 directories */
    if (counter++ % 1000 == 0) {
      if (splash) {
        std::string message = msg;
        if (message.size() > MAX_MSG-3) {
          message.resize(MAX_MSG-3);
          message.append("...");
        }
        QSplashScreen* sc = dynamic_cast<QSplashScreen*>(splash);
        if (sc)
          sc->showMessage(QString::fromStdString(message), Qt::AlignLeft, Qt::white);
        QApplication::processEvents(); // keep UI responsive
      }
    }
  });

  f.indexDirectory(path);
  qInfo() << "... (found" << f.indexed() << "files) indexing done.";

  std::vector<std::string> gfilesuffixes{".g"};
  std::vector<std::string> imgfilesuffixes{".png", ".jpg", ".gif"};

  qInfo() << "Scanning...";
  std::vector<std::string> gfiles = f.findFilesWithSuffixes(gfilesuffixes);
  std::vector<std::string> imgfiles = f.findFilesWithSuffixes(imgfilesuffixes);
  qInfo() << "...scanning done.";

  qInfo() << "Found" << gfiles.size() << "geometry files";
  qInfo() << "Found" << imgfiles.size() << "image files";

  for (const auto& file : gfiles) {
    qInfo() << "Geometry: " + QString::fromStdString(file);
  }
#if 0
  for (const auto& file : imgfiles) {
    qInfo() << "Image: " + QString::fromStdString(file);
  }
#endif

  loaded = true;
  initMainWindow();

  // update the main window
  QString message = QString("Indexed " + QString::number(f.indexed()) + " files (" + QString::number(gfiles.size()) + " geometry, " + QString::number(imgfiles.size()) + " images)");

  emit indexingComplete(message.toUtf8().constData());

}


