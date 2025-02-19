#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //设置应用名称和版本号
    a.setApplicationName("武汉兆辰");
    a.setApplicationVersion("0.3.1");

    Widget w;
    // 设置窗口标题，动态获取应用名称和版本号
    w.setWindowTitle(QString("%1 v%2")
                     .arg(a.applicationName())
                     .arg(a.applicationVersion()));
    w.show();
    return a.exec();
}
