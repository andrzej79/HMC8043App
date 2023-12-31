#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  a.setApplicationName( "HMC8043App" );
  a.setOrganizationName( "CS-Lab s.c." );
  a.setOrganizationDomain( "cs-lab.eu" );

  a.setStyle("fusion");
  QColor darkGray(53, 53, 53);
  QColor gray(128, 128, 128);
  QColor black(25, 25, 25);
  QColor blue(42, 130, 218);

  QPalette darkPalette;
  darkPalette.setColor(QPalette::Window, darkGray);
  darkPalette.setColor(QPalette::WindowText, Qt::white);
  darkPalette.setColor(QPalette::Base, black);
  darkPalette.setColor(QPalette::AlternateBase, darkGray);
  darkPalette.setColor(QPalette::ToolTipBase, blue);
  darkPalette.setColor(QPalette::ToolTipText, Qt::white);
  darkPalette.setColor(QPalette::Text, Qt::white);
  darkPalette.setColor(QPalette::Button, darkGray);
  darkPalette.setColor(QPalette::ButtonText, Qt::white);
  darkPalette.setColor(QPalette::Link, blue);
  darkPalette.setColor(QPalette::Highlight, blue);
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);

  darkPalette.setColor(QPalette::Active, QPalette::Button, gray.darker());
  darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
  darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
  darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
  darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

  qApp->setPalette(darkPalette);
  qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

  MainWindow w;
  w.show();
  return a.exec();
}
