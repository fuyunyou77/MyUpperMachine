#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    ConfigManager();
    virtual ~ConfigManager() {}

    QString getConfigFilePath(); // 声明 getConfigFilePath 函数
    bool initJson();//初始化config.json
    bool saveToJson(QString key,QString value);//将数据保存到json文件中
    float readFromJson(QString key);//从json中读取数据
signals:
    void initJsonResult(const QString &result);
    void saveToJsonResult(const QString &result);
    void readFromJsonFail(const QString &result);
//    void readFromJsonSuccess(float result);
};


#endif // CONFIGMANAGER_H
