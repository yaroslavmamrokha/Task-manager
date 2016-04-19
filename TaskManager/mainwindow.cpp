#include "mainwindow.h"
#include "ui_mainwindow.h"
wofstream log_file;
wofstream error_log("err_log.txt");
static int gl_sum;
static int rm_sum;
static int cm_sum;
static bool stop_cpu = false;
static bool go_cpu = false;
static MEMORYSTATUSEX mem_status;
static HANDLE hProcessSnap;
static PROCESSENTRY32 pe32;
static HANDLE hCPUProcessSnap;
static PROCESSENTRY32 CPUpe32;
static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
static HANDLE self;
static QString CPU;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableWidget,SIGNAL(customContextMenuRequested(const QPoint &)),this,SLOT(ProvideContextMenu(const QPoint &)));
    QAction *pTermProcess = new QAction("Terminate Process",ui->tableWidget);
    connect(pTermProcess, SIGNAL(triggered()),this,SLOT(termProc()));
    connect(ui->pushButton_5, SIGNAL(clicked()),this,SLOT(on_pushButton_2_clicked()));
    pContextMenu = new QMenu(this);
    pContextMenu->addAction(pTermProcess);
    pThr = new SystemThread;
    pThr->SetFunc(false);
    pThr2 = new SystemThread;
    pThr2->SetFunc(true);
    checker = new QTimer;
    connect(checker,SIGNAL(timeout()),this,SLOT(check()));
    ui->tableWidget->verticalHeader()->hide();
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->pushButton->setEnabled(false);
    pThr->start();
    pThr2->start();
    checker->start(970);
}

MainWindow::~MainWindow()
{
    delete pContextMenu;
    pContextMenu = NULL;
    delete ui;
}

void MainWindow::ProvideContextMenu(const QPoint &pos)
{
item = ui->tableWidget->itemAt(pos);
pContextMenu->exec(mapToGlobal(pos) );
}

void MainWindow::termProc()
{
    int id = ui->tableWidget->item(ui->tableWidget->currentRow(),1)->text().toInt();
    HANDLE curr_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, id);
    if (curr_proc == NULL){
       QMessageBox::critical(this,"Process error", "Failed to terminate process, handle zero");
       return;
}
    TerminateProcess(curr_proc,NULL);
    QMessageBox::information(this,"Sucessful termination", "Thread id: "+QString::number(id)+"\nSucessfully terminated");
}

void MainWindow::deleteRow()
{
int row = ui->tableWidget->row(item);
ui->tableWidget->removeRow(row);
}
//check if memory data and CPU data are filled
void MainWindow::check()
{
    HANDLE curr_mut = OpenMutex(NULL, NULL, L"cpu_load");
    bool chk = GO;
    bool cou_chk = go_cpu;
    ReleaseMutex(curr_mut);
    if(chk){
        printData();
        memory_data_list.clear();
        GO = false;
    }
    if(cou_chk){
        printCPU();
        go_cpu = false;
    }
}
//print memory data to screen
void MainWindow::printData()
{
    int lst_size = memory_data_list.size() - 4;
    list<QString>::iterator iter = memory_data_list.begin();
    {
      QString str = *(iter++);
      QStringList lst = str.split("#");
      ui->Sysl1->setText(lst[0]);
      ui->Sysl2->setText(lst[1]);
      ui->Sysl3->setText(lst[2]);
    }
    int wid_size = ui->tableWidget->rowCount();
    int dif = lst_size - wid_size;
    if(dif>0){
        for(int i = 0; i<dif; i++){
           ui->tableWidget->insertRow(0);
        }
    }else{
        dif*=-1;
        for(int i = 0; i<dif; i++){
           ui->tableWidget->removeRow(0);
        }
    }
    for(int i = 0; i< lst_size; i++,iter++){
        QString str = *iter;
        QStringList spl_lst = str.split("#");
        for(int j = 0; j < spl_lst.size(); j++){
        ui->tableWidget->setItem(i,j,new QTableWidgetItem(spl_lst[j]));
        }
    }
        QString str = *(iter++);
        QStringList spl_lst = str.split("#");
        ui->TRAM_3->setText(spl_lst[0]);
        ui->TPGd_3->setText(spl_lst[1]);
        str = *(iter++);
        spl_lst = str.split("#");
        ui->TRAM->setText(spl_lst[0]);
        ui->TRAM_2->setText(spl_lst[1]);
        ui->TPGd->setText(spl_lst[2]);
        ui->TPGd_2->setText(spl_lst[3]);
        str = *iter;
        spl_lst = str.split("#");
        ui->RAMUsage->setValue(spl_lst[0].toInt());
        ui->PMUsage->setValue(spl_lst[1].toInt());
}
//print CPU usage to screen
void MainWindow::printCPU()
{
    ui->CpuLabel->setText(CPU);
    ui->Cpu_Usage2->setText(CPU);
}
//Get enumaration of current running system processes
DWORD SystemThread::GetProcessesEnum(HANDLE& hSnap, LPPROCESSENTRY32W pe){
    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pe->dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hSnap, pe))
    {
        error_log << TEXT("Process32First") << endl;
        CloseHandle(hSnap);
    }
    return 0;
}

void SystemThread::SetFunc(bool j)
{
    func = j;
}
//memory information gaining thread
void SystemThread::MemoryThread()
{
    while (true){
        HANDLE curr_mut = OpenMutex(NULL, NULL, L"cpu_load");
        bool stop = stop_cpu;
        ReleaseMutex(curr_mut);
        if (stop){
            break;
        }
      mem_status.dwLength = sizeof(mem_status);


    ProcessesMemoryUsage();
    GlobalMemoryStatusEx(&mem_status);

   wstring used = to_wstring(rm_sum / 1024)+ L" MB#" + to_wstring(cm_sum / 1024) + L" MB";
   wstring avail = to_wstring(mem_status.ullTotalPhys / (1024 * 1024))+ L" MB#" + to_wstring(mem_status.ullAvailPhys  / (1024 * 1024))+ L" MB#" + to_wstring(mem_status.ullTotalPageFile / (1024 * 1024)) + L" MB#"+ to_wstring(mem_status.ullAvailPageFile / (1024 * 1024))+L" MB";
   wstring percent_use = to_wstring(mem_status.dwMemoryLoad)+L"#"+to_wstring(100 - ((mem_status.ullAvailPageFile * 100) / mem_status.ullTotalPageFile));
    memory_data_list.push_back(QString::fromStdWString(used));
    memory_data_list.push_back(QString::fromStdWString(avail));
    memory_data_list.push_back(QString::fromStdWString(percent_use));
    curr_mut = OpenMutex(NULL, NULL, L"cpu_load");
    GO = true;
    ReleaseMutex(curr_mut);
    msleep(1050);
    }
}
//CPU information gaining thread
void SystemThread::CPUThread()
{
    error_log.open(("err_log.txt"),ios_base::app);
    HQUERY cpuQuery;
      PDH_STATUS status;
      PDH_HCOUNTER cpuTotal;
      status = PdhOpenQuery(NULL, NULL, &cpuQuery);
          if(status != ERROR_SUCCESS){
          error_log << "Error Open: "<<hex<<status<<flush<<endl;
      }
      status = PdhAddEnglishCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
          if(status != ERROR_SUCCESS){
          error_log << "Error add: "<<hex<<status<<flush<<endl;
      }
      status = PdhCollectQueryData(cpuQuery);
      if (status != ERROR_SUCCESS){
          error_log << "Error collect: " << hex << status << flush << endl;
      }
      while (true){
          HANDLE curr_mut = OpenMutex(NULL, NULL, L"cpu_load");
          bool stop = stop_cpu;
          ReleaseMutex(curr_mut);
          if (stop){
              break;
          }
          PDH_FMT_COUNTERVALUE counterVal;
          PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_LONG, NULL, &counterVal);
          wchar_t CPU_info[100] = L"CPU usage: ";
          wcscat(CPU_info, to_wstring(counterVal.longValue).c_str());
          wcscat(CPU_info, L" %");

          status = PdhCollectQueryData(cpuQuery);
          if (status != ERROR_SUCCESS){
              error_log << "Error collect: " << hex << status << flush << endl;
          }
          curr_mut = OpenMutex(NULL, NULL, L"cpu_load");
          go_cpu = true;
          CPU = QString::fromStdWString(CPU_info);
          ReleaseMutex(curr_mut);
          Sleep(1050);
      }
    error_log.close();
}
//Get all system thread memory usage and fill it to global list
void SystemThread::ProcessesMemoryUsage()
{
    _PERFORMANCE_INFORMATION sys_PI;
    if (!GetPerformanceInfo(&sys_PI, sizeof(_PERFORMANCE_INFORMATION))){
        error_log << "Failed to get Performance info\n\n";
    }
    wstring sys_info =L"Thread count: " + to_wstring(sys_PI.ThreadCount) + L"#Process count: " + to_wstring(sys_PI.ProcessCount) + L"#System cache: "+ to_wstring((sys_PI.SystemCache*sys_PI.PageSize) / 1024) + L" KB";
    memory_data_list.push_back(QString::fromStdWString(sys_info));
    GetProcessesEnum(hProcessSnap, &pe32);
    rm_sum = 0;
    cm_sum = 0;
           do{
            HANDLE curr_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
            if (curr_proc == NULL)
                continue;
            PROCESS_MEMORY_COUNTERS_EX pmc;
            pmc.cb = sizeof(pmc);
            GetProcessMemoryInfo(curr_proc, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), pmc.cb);
            rm_sum = rm_sum + pmc.WorkingSetSize / 1024;
            cm_sum = cm_sum + pmc.PeakPagefileUsage / 1024;
            wstring proc_data = pe32.szExeFile;
            proc_data = proc_data+L"#" + to_wstring(pe32.th32ProcessID)+L"#"+ to_wstring(pmc.PrivateUsage / 1024)+L" KB#" + to_wstring(pmc.PeakPagefileUsage / 1024)+L" KB#" + to_wstring(pmc.WorkingSetSize / 1024)+L" KB#";
            log_file << "Process ID: " << pe32.th32ProcessID << " Name: " << pe32.szExeFile << endl;
            log_file << "System allocated virual memory for process (VM): " << pmc.PrivateUsage / 1024 << " KBytes." << endl;
            log_file << "system allocated page file memory (PM): " << pmc.PeakPagefileUsage / 1024 << " KBytes." << endl;
            log_file << "Physical memory usage(RAM): " << pmc.WorkingSetSize / 1024 << " KBytes." << endl << endl;
            memory_data_list.push_back(QString::fromStdWString(proc_data));
            CloseHandle(curr_proc);
        } while (Process32Next(hProcessSnap, &pe32));

}

void SystemThread::run()
{
    if(!func){
   std::thread thr1(&SystemThread::MemoryThread);
   thr1.join();
    }else{
        std::thread thr1(&SystemThread::CPUThread);
        thr1.join();
    }
    error_log.close();
    log_file.close();
}

void MainWindow::on_pushButton_clicked()
{
     termProc();
      ui->pushButton->setEnabled(false);
}

void MainWindow::on_pushButton_2_clicked()
{
    HANDLE curr_mut = OpenMutex(NULL, NULL, L"cpu_load");
    stop_cpu = true;
    ReleaseMutex(curr_mut);
    checker->stop();
    this->close();
}

void MainWindow::on_tableWidget_currentItemChanged(QTableWidgetItem *, QTableWidgetItem *)
{
    ui->pushButton->setEnabled(true);
}
