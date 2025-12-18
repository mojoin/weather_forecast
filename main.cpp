#include "weather_forecast.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    weather_forecast window;
    window.show();
    return app.exec();
}
