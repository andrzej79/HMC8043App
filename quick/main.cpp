#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
  QGuiApplication app(argc, argv);

  /* Same identity as the desktop UI, so both share the stored host address. */
  app.setApplicationName("HMC8043App");
  app.setOrganizationName("CS-Lab s.c.");
  app.setOrganizationDomain("cs-lab.eu");

  QQmlApplicationEngine engine;
  QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                   []() { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
  engine.loadFromModule("HMCMobile", "Main");

  return app.exec();
}
