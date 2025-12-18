#pragma once

#include <QtWidgets/QMainWindow>
#include <QNetworkAccessManager>    // 网络管理器
#include <QNetworkReply>
#include "ui_weather_forecast.h"


QT_BEGIN_NAMESPACE
namespace Ui { class weather_forecastClass; };
QT_END_NAMESPACE

class weather_forecast : public QMainWindow
{
    Q_OBJECT

public:
    weather_forecast(QWidget *parent = nullptr);
    ~weather_forecast();

private slots:
    void on_searchButton_clicked();
    void onGeoReplyFinished(QNetworkReply* reply);
    void onWeatherReplyFinished(QNetworkReply* reply);


private:
    Ui::weather_forecastClass *ui;
    QNetworkAccessManager* geoManager;
    QNetworkAccessManager* weatherManager;
    QString apiKey = "ba51ed96460fb987ceb3529056a2fcb5";

    QString weatherDescription(int code);  // WMO天气代码转描述
    QString cityName;                      // 城市名
};

