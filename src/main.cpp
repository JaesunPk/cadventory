#include "CADventory.h"

#include <QDir>
#include <QTimer>
#include <QPixmap>
#include <QString>
#include <QSplashScreen>

#include <stdlib.h>
#include <stdlib.h>
#include <iostream>
#include <filesystem>
#include <string>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

int APIENTRY
WinMain(HINSTANCE /*hInstance*/,
	HINSTANCE /*hPrevInstance*/,
	LPSTR /*lpszCmdLine*/,
	int /*nCmdShow*/)
{
    int argc = __argc;
    char **argv = __argv;
#else
int
main(int argc, char **argv)
{
#endif
  CADventory app(argc, argv);
  app.showSplash();

  QTimer::singleShot(250, [&app]() {
    std::string home = QDir::homePath().toStdString();
    const char *homestr = home.c_str();
    if (!homestr)
      homestr = ".";
    app.processEvents();
    std::string localDir = std::string(home);
    app.indexDirectory(localDir.c_str());
  });
  printf("Starting CADventory\n");

  return app.exec();
}
