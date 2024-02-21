#include "CADventory.h"

#include <QPixmap>
#include <QTimer>

#include "MainWindow.h"
#include "FilesystemIndexer.h"


CADventory::CADventory(int &argc, char *argv[]) : QApplication (argc, argv), window(nullptr), splash(nullptr), loaded(false)
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

}


CADventory::~CADventory()
{
  delete window;
  delete splash;
}


void CADventory::initMainWindow()
{
  window = new MainWindow();

  window->show();
  splash->finish(window);
  delete splash;
  splash = nullptr;

  qInfo() << "Done loading.";
}


void CADventory::showSplash()
{
  QPixmap pixmap("/Users/morrison/Desktop/RSEG127/cadventory/splash.png");
  if (pixmap.isNull()) {
    pixmap = QPixmap(512, 512);
    pixmap.fill(Qt::black);
  }
  splash = new QSplashScreen(pixmap);
  splash->showMessage("Loading... please wait.", Qt::AlignLeft, Qt::black);
  splash->show();
  // ensure the splash is displayed immediately
  this->processEvents();
}


void CADventory::indexDirectory(const char *path)
{
  qInfo() << "Indexing...";
  FilesystemIndexer f = FilesystemIndexer();

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
        splash->showMessage(QString::fromStdString(message), Qt::AlignLeft, Qt::white);
        QApplication::processEvents(); // keep UI responsive
      }
    }
  });

  size_t cnt = f.indexDirectory(path);
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
    qInfo() << "Geometry: " << file;
  }
#if 0
  for (const auto& file : imgfiles) {
    qInfo() << "Image: " << file;
  }
#endif

  loaded = true;
  // keep it visible for a minimum time
  QTimer::singleShot(500, this, &CADventory::initMainWindow);
}


