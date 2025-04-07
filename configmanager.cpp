#include "configmanager.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
ConfigManager::ConfigManager()
{

}

QString ConfigManager::getConfigFilePath() {
    // 获取应用程序的工作目录
    QDir dir(QApplication::applicationDirPath());
    return dir.filePath("config.json");
}

/**
 * @brief:初始化config.json
 * @param:无
 * @retval:bool true:初始化成功 false：初始化失败
 */
bool ConfigManager::initJson()
{
    // 获取文件路径
        QString filePath = getConfigFilePath();
        QFile file(filePath);

        // 如果文件不存在，则写入预设值
        if (!file.exists()) {

            // 创建预设的 JSON 对象
            QJsonObject presetJson;
            presetJson["VolThreshold"] = "20";
            presetJson["NormalMessFreq"] = "2";
            presetJson["LowPowMessFreq"] = "5";

            // 将 JSON 对象转换为 JSON 文档
            QJsonDocument jsonDoc(presetJson);

            // 以写入模式打开文件
            if (file.open(QIODevice::WriteOnly)) {
                // 写入 JSON 数据到文件
                file.write(jsonDoc.toJson());
                file.close();
                //写入成功
                emit initJsonResult("INIT:Success!JSON file created with preset values !");
                return true;
            } else {
                //写入失败
                emit initJsonResult("INIT:Failed to create JSON file!");
                return false;
            }
        }

        // 如果文件存在，则保持不变
        emit initJsonResult("INIT:JSON file already exists. No changes made.");
        return true;

}


/**
 * @brief：将数据保存在json文件中
 * @param：QString key：要保存的内容：json格式key-value中的key
 * @param：QString value：要保存的内容：json格式key-value中的value
 * @retval: bool true成功，false失败
*/
bool ConfigManager::saveToJson(QString key,QString value)
{
    // 读取现有 JSON 文件内容
    QFile file(getConfigFilePath());
    QJsonObject jsonObject;

    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = file.readAll();
        file.close();

        // 解析现有 JSON 数据
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        if (!jsonDoc.isNull() && jsonDoc.isObject()) {
            jsonObject = jsonDoc.object();
        }
    }

    // 添加新的 key-value 对
    jsonObject[key] = value;

    // 将 JSON 对象转换为 JSON 文档
    QJsonDocument jsonDoc(jsonObject);

    // 保存到文件
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonDoc.toJson());
        file.close();
        emit saveToJsonResult("设置并保存成功!");
        return true;
    } else {
        emit saveToJsonResult("设置并保存失败!");
        return false;
    }
}

/**
 * @brief：从 JSON 文件中读取指定 key 对应的value，并转换为浮点数
 * @param：QString key：要读取的 key
 * LowPowMessFreq：低功耗模式信息上报频率
 * NormalMessFreq：正常模式信息上报频率
 * VolThreshold：电压阈值
 * @retval: float key对应的value，类型为浮点数
 * 如果失败则返回负值
 * -1：Failed to open JSON file!
 * -2：Invalid JSON format!
 * -3：Key is not found in JSON file!
 * -4：Failed to convert value of key '%1' to float!
 */
float ConfigManager::readFromJson(QString key) {
    // 打开 JSON 文件
    QFile file(getConfigFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        emit readFromJsonFail("Failed to open JSON file!");
        return -1;
    }

    // 读取文件内容
    QByteArray jsonData = file.readAll();
    file.close();

    // 解析 JSON 数据
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        emit readFromJsonFail("Invalid JSON format!");
        return -2;

    }

    // 获取 JSON 对象
    QJsonObject jsonObject = jsonDoc.object();

    // 检查 key 是否存在
    if (!jsonObject.contains(key)) {
        emit readFromJsonFail(QString("Key '%1' not found in JSON file!").arg(key));
        return -3;
    }

    // 获取值并转换为浮点数
    QString value = jsonObject[key].toString();
    bool ok;
    float floatValue = value.toFloat(&ok);

    if (!ok) {
        emit readFromJsonFail(QString("Failed to convert value of key '%1' to float!").arg(key));
        return -4;
    }
    return floatValue;
}
