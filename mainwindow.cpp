#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <lm.h>
#include <dsgetdc.h>
#include <winldap.h>
#include <winternl.h>
#include <iostream>
#include <ldap.h>
#include <DSRole.h>
#include <windows.h>
#include <lmapibuf.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Netapi32.lib")
using RtlGetVersionCopy = NTSTATUS (*)(PRTL_OSVERSIONINFOW);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


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

        DSROLE_PRIMARY_DOMAIN_INFO_BASIC domainNamestruct;

        DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE*)&domainNamestruct);

        std::wstring domainName = domainNamestruct.DomainNameDns;

        if (domainName == L""){
            DomainWorkGroup->setText("No Domain found!");
        }else{
            DomainWorkGroup->setText(QString::fromStdWString(domainName));
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
            qDebug() << "LDAP bind failed:"
                     << ldap_err2stringW(result);
        } else {
            qDebug() << "LDAP bind successful";
        }
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

        for (LDAPMessage* user = ldap_first_entry(connectLdap, searchResult); user != nullptr; user = ldap_next_entry(connectLdap, user)){
            //ldap_get_dnW
            //usernames = ldap_get_valuesW mit const_cast<PWCHAR>(L"sAMAccountName") als letztes
            //nach jedem mal ldap_value_freeW(usernames);
            //nach for loop -> ldap_msgfree(searchResult);
            //  -> ldap_unbind(ldap);
        }




    }


}

MainWindow::~MainWindow()
{
    delete ui;
}
