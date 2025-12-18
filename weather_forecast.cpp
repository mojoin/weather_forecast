#include "weather_forecast.h"
#include <QMessageBox>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

weather_forecast::weather_forecast(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::weather_forecastClass())
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/weather_forecast/icon/1_1.png"));
    geoManager = new QNetworkAccessManager(this);
    weatherManager = new QNetworkAccessManager(this);

    // 当 QNetworkAccessManager 完成一个网络请求时，会发出 finished(QNetworkReply*) 信号
    connect(geoManager, &QNetworkAccessManager::finished, this, &weather_forecast::onGeoReplyFinished);
    connect(weatherManager, &QNetworkAccessManager::finished, this, &weather_forecast::onWeatherReplyFinished);
    // 输入框回车时触发查询
    connect(ui->cityInput, &QLineEdit::returnPressed, this, &weather_forecast::on_searchButton_clicked);
}

weather_forecast::~weather_forecast()
{
    delete ui;
}

// geoManager 发送网络请求
void weather_forecast::on_searchButton_clicked() {
    QString city = ui->cityInput->text().trimmed();
    if (city.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入城市名！");
        return;
    }

    ui->currentWeatherLabel->setText("正在查询位置...");
    ui->forecastList->clear();

    // 获取经纬度 是通过open weather map API 获取地理数据
    // 在通过城市名获取地理数据,之后在用免费的天气api获取天气数据
    QString geoUrl = QString("http://api.openweathermap.org/geo/1.0/direct?q=%1&limit=1&appid=%2")
        .arg(city, apiKey);

    // 发送网络请求 获取经纬度 json数据
    geoManager->get(QNetworkRequest(QUrl(geoUrl)));
}

// geoManager 获取经纬度成功后，触发weatherManager获取天气数据
void weather_forecast::onGeoReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "错误", "位置查询失败: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.array();

    if (array.isEmpty()) {
        QMessageBox::warning(this, "错误", "未找到该城市！");
        reply->deleteLater();
        return;
    }

    QJsonObject obj = array.first().toObject();
    double lat = obj["lat"].toDouble();     // 纬度
    double lon = obj["lon"].toDouble();     // 经度
    cityName = obj["name"].toString() + ", " + obj["country"].toString();

    reply->deleteLater();

    QString weatherUrl = QString("https://api.open-meteo.com/v1/forecast?"
        "latitude=%1&longitude=%2"
        "&current=temperature_2m,weather_code"
        "&daily=temperature_2m_max,temperature_2m_min,weather_code"
        "&timezone=auto")
        .arg(lat).arg(lon);

    weatherManager->get(QNetworkRequest(QUrl(weatherUrl)));
    ui->currentWeatherLabel->setText(QString("正在加载 %1 的天气...").arg(cityName));
}

// weatherManager 获取天气成功后，触发ui更新
void weather_forecast::onWeatherReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "错误", "天气查询失败: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    qDebug() << data; 
    //ui->textEditdebug->setText(data);

    // 当前天气
    QJsonObject current = root["current"].toObject();
    double temp = current["temperature_2m"].toDouble();
    int code = current["weather_code"].toInt();
    ui->currentWeatherLabel->setText(cityName + QString("当前天气：%1°C，%2")
        .arg(temp, 0, 'f', 1)
        .arg(weatherDescription(code)));

    // 预报
    ui->forecastList->clear();
    QJsonObject daily = root["daily"].toObject();
    QJsonArray dates = daily["time"].toArray();
    QJsonArray maxT = daily["temperature_2m_max"].toArray();
    QJsonArray minT = daily["temperature_2m_min"].toArray();
    QJsonArray codes = daily["weather_code"].toArray();

    for (int i = 0; i < dates.size(); ++i) {
        QString date = dates[i].toString();
        double max = maxT[i].toDouble();
        double min = minT[i].toDouble();
        int c = codes[i].toInt();
        ui->forecastList->addItem(QString("%1: 最高 %2°C，最低 %3°C，%4")
            .arg(date).arg(max, 0, 'f', 1).arg(min, 0, 'f', 1).arg(weatherDescription(c)));
    }

    reply->deleteLater();
}

QString weather_forecast::weatherDescription(int code)
{
    switch (code) {
    case 0:  return "☀️ 晴朗";  // Clear weather
    case 1:
    case 2:  return "⛅ 部分多云";  // Partly cloudy
    case 3:  return "☁️ 阴天";  // Overcast
    case 4:  return "🌫️ 烟雾";  // Visibility reduced by smoke
    case 5:  return "🌫️ 霾";  // Haze
    case 6:  return "💨 浮尘";  // Widespread dust in suspension
    case 7:  return "💨 扬沙";  // Dust or sand raised by wind
    case 8:  return "🌪️ 尘卷风";  // Well developed dust whirl(s)
    case 9:  return "🌪️ 沙尘暴";  // Duststorm or sandstorm
    case 10: return "🌫️ 薄雾";  // Mist
    case 11:
    case 12: return "🌫️ 浅雾";  // Patches shallow fog / More or less continuous shallow fog
    case 13: return "⚡ 闪电";  // Lightning visible, no thunder heard
    case 14:
    case 15: return "🌧️ 远降水";  // Precipitation within sight, not reaching ground / reaching ground, distant
    case 16: return "🌧️ 近降水";  // Precipitation within sight, reaching ground, near
    case 17: return "⚡ 雷暴无降水";  // Thunderstorm, no precipitation
    case 18: return "🌪️ 飑";  // Squalls
    case 19: return "🌪️ 漏斗云";  // Funnel cloud(s)
    case 20: return "🌦️ 毛毛雨";  // Drizzle or snow grains not shower
    case 21: return "🌧️ 雨";  // Rain not freezing
    case 22: return "❄️ 雪";  // Snow
    case 23: return "🌨️ 雨夹雪";  // Rain and snow or ice pellets
    case 24: return "❄️ 冻雨";  // Freezing drizzle or freezing rain
    case 25: return "🌦️ 阵雨";  // Shower(s) of rain
    case 26: return "❄️ 阵雪";  // Shower(s) of snow, or of rain and snow
    case 27: return "⚪ 冰雹";  // Shower(s) of hail
    case 28: return "🌫️ 雾";  // Fog or ice fog
    case 29: return "⚡ 雷暴";  // Thunderstorm
    case 30: return "🌪️ 沙尘暴减弱";  // Slight or moderate duststorm, decreasing
    case 31: return "🌪️ 沙尘暴";  // Slight or moderate duststorm, no change
    case 32: return "🌪️ 沙尘暴增强";  // Slight or moderate duststorm, increasing
    case 33: return "🌪️ 强沙尘暴减弱";  // Severe duststorm, decreasing
    case 34: return "🌪️ 强沙尘暴";  // Severe duststorm, no change
    case 35: return "🌪️ 强沙尘暴增强";  // Severe duststorm, increasing
    case 36:
    case 38: return "❄️ 低吹雪";  // Slight or moderate blowing snow, low / high
    case 37:
    case 39: return "❄️ 强吹雪";  // Heavy drifting snow
    case 40: return "🌫️ 远雾";  // Fog at a distance
    case 41: return "🌫️ 片状雾";  // Fog in patches
    case 42:
    case 44:
    case 46: return "🌫️ 雾";  // Fog, sky visible
    case 43:
    case 45:
    case 47: return "🌫️ 浓雾";  // Fog, sky invisible
    case 48: return "❄️ 雾凇";  // Fog, depositing rime, sky visible
    case 49: return "❄️ 浓雾凇";  // Fog, depositing rime, sky invisible
    case 50:
    case 51: return "🌦️ 间歇性小毛毛雨";  // Drizzle, not freezing, intermittent slight / continuous
    case 52:
    case 53: return "🌦️ 持续性毛毛雨";  // Drizzle, not freezing, intermittent moderate / continuous
    case 54:
    case 55: return "🌧️ 间歇性大毛毛雨";  // Drizzle, not freezing, intermittent heavy / continuous
    case 56: return "❄️ 冻毛毛雨";  // Drizzle, freezing, slight
    case 57: return "❄️ 中到强冻毛毛雨";  // Drizzle, freezing, moderate or heavy
    case 58: return "🌦️ 毛毛雨和小雨";  // Drizzle and rain, slight
    case 59: return "🌧️ 毛毛雨和中到大雨";  // Drizzle and rain, moderate or heavy
    case 60:
    case 61: return "🌦️ 间歇性小雨";  // Rain, not freezing, intermittent slight / continuous
    case 62:
    case 63: return "🌧️ 间歇性中雨";  // Rain, not freezing, intermittent moderate / continuous
    case 64:
    case 65: return "🌧️ 间歇性大雨";  // Rain, not freezing, intermittent heavy / continuous
    case 66: return "❄️ 冻雨";  // Rain, freezing, slight
    case 67: return "❄️ 中到强冻雨";  // Rain, freezing, moderate or heavy
    case 68: return "🌨️ 雨夹雪";  // Rain or drizzle and snow, slight
    case 69: return "🌨️ 中到大雨夹雪";  // Rain or drizzle and snow, moderate or heavy
    case 70:
    case 71: return "❄️ 间歇性小雪";  // Snowflakes, intermittent slight / continuous
    case 72:
    case 73: return "❄️ 间歇性中雪";  // Snowflakes, intermittent moderate / continuous
    case 74:
    case 75: return "❄️ 间歇性大雪";  // Snowflakes, intermittent heavy / continuous
    case 76: return "❄️ 冰晶";  // Diamond dust
    case 77: return "❄️ 雪粒";  // Snow grains
    case 78: return "❄️ 星状雪晶";  // Isolated star-like snow crystals
    case 79: return "❄️ 冰粒";  // Ice pellets
    case 80: return "🌦️ 小阵雨";  // Rain shower(s), slight
    case 81: return "🌧️ 中到大阵雨";  // Rain shower(s), moderate or heavy
    case 82: return "🌧️ 强阵雨";  // Rain shower(s), violent
    case 83: return "🌨️ 小阵雨夹雪";  // Shower(s) of rain and snow mixed, slight
    case 84: return "🌨️ 中到大阵雨夹雪";  // Shower(s) of rain and snow mixed, moderate or heavy
    case 85: return "❄️ 小阵雪";  // Snow shower(s), slight
    case 86: return "❄️ 中到大阵雪";  // Snow shower(s), moderate or heavy
    case 87: return "❄️ 小阵雪粒或小冰雹";  // Shower(s) of snow pellets or small hail, slight
    case 88: return "❄️ 中到大阵雪粒或小冰雹";  // Shower(s) of snow pellets or small hail, moderate or heavy
    case 89: return "⚪ 小阵冰雹";  // Shower(s) of hail, slight
    case 90: return "⚪ 中到大阵冰雹";  // Shower(s) of hail, moderate or heavy
    case 91: return "⚡🌧️ 小雨，前一小时有雷暴";  // Slight rain, thunderstorm during preceding hour
    case 92: return "⚡🌧️ 中到大雨，前一小时有雷暴";  // Moderate or heavy rain, thunderstorm during preceding hour
    case 93: return "⚡🌨️ 小雪或雨夹雪，前一小时有雷暴";  // Slight snow, or rain and snow mixed or hail, thunderstorm during preceding hour
    case 94: return "⚡🌨️ 中到大雪或雨夹雪，前一小时有雷暴";  // Moderate or heavy snow, or rain and snow mixed or hail, thunderstorm during preceding hour
    case 95: return "⚡ 雷暴，无冰雹";  // Thunderstorm, slight or moderate, without hail
    case 96: return "⚡⚪ 雷暴伴有冰雹";  // Thunderstorm, slight or moderate, with hail
    case 97: return "⚡ 强雷暴，无冰雹";  // Thunderstorm, heavy, without hail
    case 98: return "⚡🌪️ 雷暴伴沙尘暴";  // Thunderstorm combined with duststorm or sandstorm
    case 99: return "⚡⚪ 强雷暴伴有冰雹";  // Thunderstorm, heavy, with hail
    default: return "❓ 未知天气代码";  // Unknown weather code
    }
}