#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <lm.h>
#include <TlHelp32.h>
#include <dsgetdc.h>
#include <winldap.h>
#include <winternl.h>
#include <iostream>
#include <chrono>
#include <DSRole.h>
#include <lmapibuf.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <thread>
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Netapi32.lib")
using RtlGetVersionCopy = NTSTATUS (*)(PRTL_OSVERSIONINFOW);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QTimer* timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, [this]()
            {
                quint64 uptimeMs = GetTickCount64();
                quint64 totalSeconds = uptimeMs / 1000;

                quint64 hours = totalSeconds / 3600;
                quint64 minutes = (totalSeconds % 3600) / 60;
                quint64 seconds = totalSeconds % 60;

                QString uptime = QString("%1:%2:%3")
                                     .arg(hours, 2, 10, QChar('0'))
                                     .arg(minutes, 2, 10, QChar('0'))
                                     .arg(seconds, 2, 10, QChar('0'));

                QTableWidgetItem* item = ui->tableWidget->item(0, 5);

                if (!item) {
                    item = new QTableWidgetItem();
                    ui->tableWidget->setItem(0, 5, item);
                }

                item->setText(uptime);
            });

    timer->start(1000);

    int testnum;
    QTableWidgetItem * o = ui->tableWidget->item(0,0);
    QTableWidgetItem * oVersion = ui->tableWidget->item(0, 1);
    if(o){
        char compName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(compName);
        int ret = GetComputerNameA(compName, &size);
        if (ret != 0){
            QString stringToInput;
            std::string temp = compName;
            stringToInput = QString::fromStdString(temp);
            o->setText(stringToInput);
        }
        else{
            o->setText("Lookup failed");
        }
    }
    HMODULE dllHandle = LoadLibraryA("Ntdll.dll");
    if (!dllHandle){
        char buff[5096 ];
        sprintf(buff, "LoadLibrary function failed with error code -> %lu", GetLastError());
        MessageBoxA(NULL, buff, "Warning", MB_OK);
        exit(0);
    }
    else{

        RtlGetVersionCopy RtlGetVersion = reinterpret_cast<RtlGetVersionCopy>(GetProcAddress(dllHandle, "RtlGetVersion"));

        if (!RtlGetVersion){
            char buff[5096 ];
            sprintf(buff, "RtlGetVersion was not found. Error code -> %lu", GetLastError());
            MessageBoxA(NULL, buff, "Warning", MB_OK);
            exit(0);
        }

        RTL_OSVERSIONINFOW buildInfo = {0};
        NTSTATUS status = RtlGetVersion(&buildInfo);

        if (!NT_SUCCESS(status)){
            char buff[5096 ];
            sprintf(buff, "RtlGetVersion failed. Error code -> %lu", GetLastError());
            MessageBoxA(NULL, buff, "Warning", MB_OK);
            exit(0);
        }

        if (buildInfo.dwMajorVersion == 10 && buildInfo.dwBuildNumber >= 22000){
            std::string versionToCpy = "Windows 11 Build " + std::to_string(buildInfo.dwBuildNumber);
            oVersion->setText(QString::fromStdString(versionToCpy));
        }else if (buildInfo.dwMajorVersion == 10){
            std::string versionToCpy = "Windows 10 Build " + std::to_string(buildInfo.dwBuildNumber);
            oVersion->setText(QString::fromStdString(versionToCpy));
        }

        //hier als nächsteas architecture
        SYSTEM_INFO sysInfo = {0};
        GetNativeSystemInfo(&sysInfo);
        QTableWidgetItem* arch = ui->tableWidget->item(0, 2);
        if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64){
            if (arch != NULL){
                arch->setText("x64");
            }
        }else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64){
            if (arch != NULL){
                arch->setText("x32");
            }
        }

        QTableWidgetItem* DomainWorkGroup = ui->tableWidget->item(0,3);

        PDSROLE_PRIMARY_DOMAIN_INFO_BASIC domainNamestruct = nullptr;

        DWORD res = DsRoleGetPrimaryDomainInformation(
            nullptr,
            DsRolePrimaryDomainInfoBasic,
            reinterpret_cast<PBYTE*>(&domainNamestruct)
            );

        if (res == ERROR_SUCCESS && domainNamestruct)
        {
            QString domainName =
                QString::fromWCharArray(domainNamestruct->DomainNameDns);

            if (domainName.isEmpty()) {
                DomainWorkGroup->setText("No Domain found!");
            } else {
                DomainWorkGroup->setText(domainName);
            }

            DsRoleFreeMemory(domainNamestruct);
        }




        PDOMAIN_CONTROLLER_INFOW domainInfo = {0};

        DWORD res1 = DsGetDcNameW(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &domainInfo);

        if (res1 == ERROR_SUCCESS){
            wprintf(L"DC: %s\n", domainInfo->DomainControllerName);
            wprintf(L"Domain: %s\n", domainInfo->DomainName);


        }

        LDAP* connectLdap = ldap_initW(NULL, LDAP_PORT);
        ULONG version = LDAP_VERSION3;
        ldap_set_optionW(connectLdap, LDAP_OPT_PROTOCOL_VERSION, &version);
        ULONG result = ldap_bind_sW(
            connectLdap,
            nullptr,
            nullptr,
            LDAP_AUTH_NEGOTIATE
            );
        if (result != LDAP_SUCCESS) {
            qDebug() << "LDAP bind result:" << result;
            qDebug() << "LDAP error:"
                     << QString::fromWCharArray(ldap_err2stringW(result));
            ui->tableWidget->item(0,4)->setText("No Users found! Domain may be down");
        } else {
            qDebug() << "LDAP bind successful";
            QString domain = QString::fromStdWString(domainInfo->DomainName);
            QString dc = QString::fromStdWString(domainInfo->DomainControllerName);
            qDebug() << domain;
            PWCHAR userAttributes[] = {
                const_cast<PWCHAR>(L"sAMAccountName"),
                const_cast<PWCHAR>(L"userPrincipalName"),
                const_cast<PWCHAR>(L"displayName"),
                nullptr
            };
            LDAPMessage* searchResult = nullptr;
            QString filter = "(&(objectCategory=person)(objectClass=user))";
            std::wstring domainW = domain.toStdWString();
            std::wstring filterW = filter.toStdWString();
            ULONG result_query = ldap_search_sW(connectLdap, const_cast<PWSTR>(domainW.c_str()), LDAP_SCOPE_SUBTREE, const_cast<PWSTR>(filterW.c_str()), userAttributes, 0, &searchResult);

            int roww = 0;
            for (LDAPMessage* user = ldap_first_entry(connectLdap, searchResult); user != nullptr; user = ldap_next_entry(connectLdap, user)){
                PWCHAR attr = const_cast<PWCHAR>(L"sAMAccountName");
                PWSTR* usernames = ldap_get_valuesW(connectLdap, user, attr);
                if (usernames && usernames[0]){
                    ui->tableWidget->item(roww, 4)->setText(QString::fromWCharArray(usernames[0]));
                    ldap_value_freeW(usernames);
                }
                roww++;
            }
            if (ui->tableWidget->item(0, 4) == nullptr || ui->tableWidget->item(0, 4)->text().isEmpty()){
                ui->tableWidget->item(0, 4)->setText("No Users found!");
            }
            ldap_msgfree(searchResult);
            ldap_unbind(connectLdap);

        }
        }

        HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        PROCESSENTRY32  processEntryStruct = {};
        processEntryStruct.dwSize = sizeof(PROCESSENTRY32);
        int currentRow = 0;
        BOOL ret = Process32First(snapshotHandle, &processEntryStruct);

        if (!ret){
            MessageBoxA(NULL, "Processes couldn't be listed. Try running the tool with administrator privileges.\nIf the problem still remains, open an Issue on my GitHub." ,"Warning", NULL);
        }
        else{

                DWORD pid = processEntryStruct.th32ProcessID;
                std::string pid_conv = std::to_string(pid);

                std::wstring processName_conv = processEntryStruct.szExeFile;
                ui->tableWidget_2->setItem(currentRow, 0, new QTableWidgetItem(QString::fromStdWString(processName_conv)));
                ui->tableWidget_2->setItem(currentRow, 1, new QTableWidgetItem(QString::fromStdString(pid_conv)));
                CloseHandle(snapshotHandle); //das morgen raus! War nur testweise
                //hier dann direkt auch noch die module auflisten mit seperatem CreateToolhelp32Snapshot. Dann in den Process32Next loop und dort das selbe machen! (Name anzeigen, PID anzeigen und Module)

        }







}

MainWindow::~MainWindow()
{
    delete ui;
}
