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
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Wldap32.lib")
using RtlGetVersionCopy = NTSTATUS (*)(PRTL_OSVERSIONINFOW);

static void EnableDebugPrivilege()
{
    HANDLE token = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        return;
    }
    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
        AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL);
    }
    CloseHandle(token);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableWidget_2->setColumnCount(3);
    ui->tableWidget_2->setColumnWidth(0, 200);
    ui->tableWidget_2->setColumnWidth(1, 80);
    QTreeWidget* tw = ui->treeWidget;

    tw->setColumnWidth(0, 180); // Process
    tw->setColumnWidth(1, 60);  // PID
    tw->setColumnWidth(2, 160); // SeImpersonatePrivilege?
    tw->setColumnWidth(3, 140); // SeDebugPrivilege?
    tw->setColumnWidth(4, 180); // Token-Owner
    tw->setColumnWidth(5, 110); // Integrity Levell
    tw->header()->setStretchLastSection(true);

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

    EnableDebugPrivilege();

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
        }else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL){
            if (arch != NULL){
                arch->setText("x32");
            }
        }else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64){
            if (arch != NULL){
                arch->setText("ARM64");
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
                if (DomainWorkGroup) DomainWorkGroup->setText("No Domain found!");
            } else {
                if (DomainWorkGroup) DomainWorkGroup->setText(domainName);
            }

            DsRoleFreeMemory(domainNamestruct);
        }
        else if (DomainWorkGroup) {
            DomainWorkGroup->setText("No Domain found!");
        }

        PDOMAIN_CONTROLLER_INFOW domainInfo = nullptr;

        DWORD res1 = DsGetDcNameW(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &domainInfo);

        bool haveDomainInfo = (res1 == ERROR_SUCCESS && domainInfo != nullptr);

        if (haveDomainInfo){
            wprintf(L"DC: %s\n", domainInfo->DomainControllerName);
            wprintf(L"Domain: %s\n", domainInfo->DomainName);
        }

        // LDAP nur versuchen, wenn wir überhaupt eine Domain haben
        if (haveDomainInfo){

            LDAP* connectLdap = ldap_initW(NULL, LDAP_PORT);

            if (!connectLdap){
                if (ui->tableWidget->item(0,4)) ui->tableWidget->item(0,4)->setText("LDAP init failed");
            }
            else{
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
                    if (ui->tableWidget->item(0,4)) ui->tableWidget->item(0,4)->setText("No Users found! Domain may be down");
                    ldap_unbind(connectLdap);
                } else {
                    qDebug() << "LDAP bind successful";
                    QString domain = QString::fromWCharArray(domainInfo->DomainName);
                    QString dc = QString::fromWCharArray(domainInfo->DomainControllerName);
                    qDebug() << domain;

                    // DN aus Domainnamen bauen, z.B. corp.local -> DC=corp,DC=local
                    QStringList parts = domain.split('.', Qt::SkipEmptyParts);
                    QStringList dcParts;
                    for (const QString& part : parts) {
                        dcParts << ("DC=" + part);
                    }
                    QString baseDn = dcParts.join(",");

                    PWCHAR userAttributes[] = {
                        const_cast<PWCHAR>(L"sAMAccountName"),
                        const_cast<PWCHAR>(L"userPrincipalName"),
                        const_cast<PWCHAR>(L"displayName"),
                        nullptr
                    };
                    LDAPMessage* searchResult = nullptr;
                    QString filter = "(&(objectCategory=person)(objectClass=user))";
                    std::wstring baseDnW = baseDn.toStdWString();
                    std::wstring filterW = filter.toStdWString();
                    ULONG result_query = ldap_search_sW(connectLdap, const_cast<PWSTR>(baseDnW.c_str()), LDAP_SCOPE_SUBTREE, const_cast<PWSTR>(filterW.c_str()), userAttributes, 0, &searchResult);

                    if (result_query != LDAP_SUCCESS){
                        qDebug() << "LDAP search failed:" << QString::fromWCharArray(ldap_err2stringW(result_query));
                        if (ui->tableWidget->item(0,4)) ui->tableWidget->item(0,4)->setText("LDAP search failed");
                    }
                    else{
                        int roww = 0;
                        for (LDAPMessage* user = ldap_first_entry(connectLdap, searchResult); user != nullptr; user = ldap_next_entry(connectLdap, user)){
                            PWCHAR attr = const_cast<PWCHAR>(L"sAMAccountName");
                            PWSTR* usernames = ldap_get_valuesW(connectLdap, user, attr);
                            if (usernames && usernames[0]){
                                QTableWidgetItem* userItem = ui->tableWidget->item(roww, 4);
                                if (!userItem){
                                    userItem = new QTableWidgetItem();
                                    ui->tableWidget->setItem(roww, 4, userItem);
                                }
                                userItem->setText(QString::fromWCharArray(usernames[0]));
                                ldap_value_freeW(usernames);
                            }
                            roww++;
                        }
                        if (roww == 0 && ui->tableWidget->item(0, 4)){
                            ui->tableWidget->item(0, 4)->setText("No Users found!");
                        }
                        if (searchResult) ldap_msgfree(searchResult);
                    }
                    ldap_unbind(connectLdap);
                }
            }
        }
        else if (ui->tableWidget->item(0,4)) {
            ui->tableWidget->item(0,4)->setText("No Domain found!");
        }

        if (domainInfo){
            NetApiBufferFree(domainInfo);
        }

        FreeLibrary(dllHandle);
    }

    HANDLE snapshotHandle = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
        );

    if (snapshotHandle == INVALID_HANDLE_VALUE){
        MessageBoxA(
            NULL,
            "Processes couldn't be listed. Try running the tool with administrator privileges.\n"
            "If the problem still remains, open an Issue on my GitHub.",
            "Warning",
            NULL
            );
        return;
    }

    PROCESSENTRY32W processEntryStruct = {};
    processEntryStruct.dwSize = sizeof(PROCESSENTRY32W);

    int currentRow = 0;

    BOOL ret = Process32FirstW(snapshotHandle, &processEntryStruct);

    if (!ret) {
        MessageBoxA(
            NULL,
            "Processes couldn't be listed. Try running the tool with administrator privileges.\n"
            "If the problem still remains, open an Issue on my GitHub.",
            "Warning",
            NULL
            );
    }
    else {

        do {

            DWORD pid = processEntryStruct.th32ProcessID;

            std::string pid_conv = std::to_string(pid);
            std::wstring processName_conv = processEntryStruct.szExeFile;

            QTreeWidgetItem* processItem = new QTreeWidgetItem(ui->tableWidget_2);
            processItem->setText(0, QString::fromStdWString(processName_conv));
            processItem->setText(1, QString::fromStdString(pid_conv));

            if (pid == 0){
                processItem->setText(2, "-");
            }
            else{
                HANDLE moduleHandle = CreateToolhelp32Snapshot(
                    TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
                    pid
                    );

                if (moduleHandle != INVALID_HANDLE_VALUE) {

                    MODULEENTRY32W modules = {};
                    modules.dwSize = sizeof(MODULEENTRY32W);

                    int dllCount = 0;

                    BOOL ret_mod1 = Module32FirstW(
                        moduleHandle,
                        &modules
                        );

                    if (ret_mod1) {

                        do {
                            QTreeWidgetItem* dllChild = new QTreeWidgetItem(processItem);
                            dllChild->setText(2, QString::fromWCharArray(modules.szModule));
                            dllCount++;

                        } while (Module32NextW(
                            moduleHandle,
                            &modules
                            ));
                    }

                    processItem->setText(2, QString("%1 DLL's (click to expand)").arg(dllCount));

                    CloseHandle(moduleHandle);
                }
                else{
                    DWORD err = GetLastError();
                    processItem->setText(2,
                                         err == ERROR_ACCESS_DENIED ? "Access denied (protected?)" : QString("Error %1").arg(err)
                                         );
                }
            }

        } while (Process32NextW(
            snapshotHandle,
            &processEntryStruct
            ));
    }

    CloseHandle(snapshotHandle);

    for (int i = 0; i < ui->tableWidget_2->columnCount(); i++){
        ui->tableWidget_2->resizeColumnToContents(i);
    }


}

MainWindow::~MainWindow()
{
    delete ui;
}