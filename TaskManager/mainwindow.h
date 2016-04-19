#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include"gl_libs.h"

static bool GO{false};

static list<QString> memory_data_list;
static DWORD GetProcessesEnum(HANDLE& hSnap, LPPROCESSENTRY32W pe);

class SystemThread : public QThread{
private:
    bool func{false};
protected:
    static void  MemoryThread();
    static void  CPUThread();
    static void  ProcessesMemoryUsage();
    static DWORD GetProcessesEnum(HANDLE& hSnap, LPPROCESSENTRY32W pe);
public:
    void SetFunc(bool);
    void run();
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void ProvideContextMenu(const QPoint &pos);
    void termProc();
    void deleteRow();
    void check();
    void printData();
    void printCPU();
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_tableWidget_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous);

private:
    QTimer* checker;
    QTimer* cpu_checker;
    QMenu *pContextMenu;
    SystemThread* pThr;
    SystemThread* pThr2;
    QTableWidgetItem* item;
    Ui::MainWindow *ui;
};


#endif // MAINWINDOW_H
