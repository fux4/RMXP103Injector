// #pragma once

#include <windows.h>
#include <TlHelp32.h>
//#include <time.h>
//
// 提升进程访问权限
bool enableDebugPriv()
{
    HANDLE  hToken;
    LUID    sedebugnameValue;
    TOKEN_PRIVILEGES tkp;
    if  ( !OpenProcessToken(  GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)
        )
    {
        return false;
    }
    if( !LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue) )
    {
        CloseHandle(hToken);
        return false;
    }
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = sedebugnameValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if( !AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL) )
    {
        CloseHandle(hToken);
        return false;
    }
    return true;
}

// 根据进程名称得到进程ID,如果有多个运行实例的话，返回第一个枚举到的进程的ID
DWORD processNameToId(LPCTSTR lpszProcessName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if( !Process32First(hSnapshot, &pe) )
    {
        MessageBox( NULL,
                    "The frist entry of the process list has not been copyied to the buffer",
                    "Notice",
                    MB_ICONINFORMATION | MB_OK
                    );
        return 0;
    }
    while( Process32Next(hSnapshot, &pe) )
    {
        if( !strcmp(lpszProcessName, pe.szExeFile) )
        {
            return pe.th32ProcessID;
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    // 定义线程体的大小
    const DWORD dwThreadSize = 5 * 1024;
    DWORD dwWriteBytes;
    // 提升进程访问权限
    enableDebugPriv();
    // 等待输入进程名称，注意大小写匹配
    auto szExeName = "RPGXP.exe";
    DWORD dwProcessId = processNameToId(szExeName);
    if( dwProcessId == 0 )
    {
        MessageBox( NULL,
                    "先得打开RM !",
                    "提示",
                    MB_ICONINFORMATION | MB_OK
                    );
        return -1;
    }

    // 根据进程ID得到进程句柄
    HANDLE hTargetProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if( !hTargetProcess )
    {
        MessageBox( NULL,
                    "Open target process failed !",
                    "Notice",
                    MB_ICONINFORMATION | MB_OK
                    );
        return 0;
    }

    // 在宿主进程中为线程体开辟一块存储区域
    // 在这里需要注意MEM_COMMIT内存非配类型以及PAGE_EXECUTE_READWRITE内存保护类型
    // 其具体含义请参考MSDN中关于VirtualAllocEx函数的说明。
    void* pRemoteThread = VirtualAllocEx(   hTargetProcess,
                                            0,
                                            dwThreadSize,
                                            MEM_COMMIT , PAGE_EXECUTE_READWRITE);
    if( !pRemoteThread )
    {
        MessageBox( NULL,
                    "Alloc memory in target process failed !",
                    "notice",
                    MB_ICONINFORMATION | MB_OK
                    );
        return 0;
    }
    // 设置需要注入的DLL名称
    char szDll[256];
	memset(szDll, 0, 256);
	HMODULE myPID = GetModuleHandle("RPGMakerXP-Injector.exe");
	DWORD len = GetModuleFileNameA(myPID, szDll, MAX_PATH);
    szDll[len - 23] = 'R';
	szDll[len - 22] = 'M';
	szDll[len - 21] = 'I';
	szDll[len - 20] = 'n';
	szDll[len - 19] = 'j';
	szDll[len - 18] = 'e';
	szDll[len - 17] = 'c';
	szDll[len - 16] = 't';
	szDll[len - 15] = '.';
	szDll[len - 14] = 'd';
	szDll[len - 13] = 'l';
	szDll[len - 12] = 'l';
	szDll[len - 11] = 0;
    // 拷贝注入DLL内容到宿主空间
    if( !WriteProcessMemory(    hTargetProcess,
                                pRemoteThread,
                                (LPVOID)szDll,
                                dwThreadSize,
                                0) )
    {
        MessageBox( NULL,
                    "Write data to target process failed !",
                    "Notice",
                    MB_ICONINFORMATION | MB_OK
                    );
        return 0;
    }

    LPVOID pFunc = LoadLibraryA;
    //在宿主进程中创建线程
    HANDLE hRemoteThread = CreateRemoteThread(  hTargetProcess,
                                                NULL,
                                                0,
                                                (LPTHREAD_START_ROUTINE)pFunc,
                                                pRemoteThread,
                                                0,
                                                &dwWriteBytes);
     if( !hRemoteThread )
     {
         MessageBox(    NULL,
                        "Create remote thread failed !",
                        "Notice",
                        MB_ICONINFORMATION | MB_OK
                        );
         return 0;
     }
     // 等待LoadLibraryA加载完毕
     WaitForSingleObject(hRemoteThread, INFINITE );
     VirtualFreeEx(hTargetProcess, pRemoteThread, dwThreadSize, MEM_COMMIT);
     CloseHandle( hRemoteThread );
     CloseHandle( hTargetProcess );
     return 0;
}