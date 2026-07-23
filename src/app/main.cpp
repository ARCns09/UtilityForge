#include "app/ApplicationBootstrap.h"

#include <QApplication>

int main(int argumentCount, char *argumentValues[])
{
    QApplication application(argumentCount, argumentValues);
    const utilityforge::app::ApplicationBootstrap bootstrap;
    return bootstrap.run(application);
}
