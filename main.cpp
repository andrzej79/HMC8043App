#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  a.setApplicationName( "HMC8043App" );
  a.setOrganizationName( "CS-Lab s.c." );
  a.setOrganizationDomain( "cs-lab.eu" );

  MainWindow w;
  w.show();
  return a.exec();
}
