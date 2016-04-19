#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    remove("log.txt");
    remove("err_log.txt");

    CreateMutex(NULL, NULL, L"cpu_load");
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("Task Manager (Mamrokha)");
    Sleep(800);
    w.show();

    return a.exec();
}
