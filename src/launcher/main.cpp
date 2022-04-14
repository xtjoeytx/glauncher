//////////////////////////////////////////////////////////////////////////////
//
//  Test DetourCreateProcessWithDll function (withdll.cpp).
//
//  Microsoft Research Detours Package
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
#include <stdio.h>
#include <windows.h>
#include <detours.h>
#pragma warning(push)
#if _MSC_VER > 1400
#pragma warning(disable:6102 6103) // /analyze warnings
#endif
#include <strsafe.h>
#pragma warning(pop)

#include "IConfig.h"

//////////////////////////////////////////////////////////////////////////////
//
//  This code verifies that the named DLL has been configured correctly
//  to be imported into the target process.  DLLs must export a function with
//  ordinal #1 so that the import table touch-up magic works.
//
struct ExportContext
{
    BOOL    fHasOrdinal1;
    ULONG   nExports;
};

static BOOL CALLBACK ExportCallback(_In_opt_ PVOID pContext,
                                    _In_ ULONG nOrdinal,
                                    _In_opt_ LPCSTR pszSymbol,
                                    _In_opt_ PVOID pbTarget)
{
    (void)pContext;
    (void)pbTarget;
    (void)pszSymbol;

    ExportContext *pec = (ExportContext *)pContext;

    if (nOrdinal == 1) {
        pec->fHasOrdinal1 = TRUE;
    }
    pec->nExports++;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////// main.
//
int CDECL main(int, char **)
{
    LPCSTR rpszDllsRaw[256];
    LPCSTR rpszDllsOut[256];
    DWORD nDlls = 0;

    for (DWORD n = 0; n < ARRAYSIZE(rpszDllsRaw); n++) {
        rpszDllsRaw[n] = NULL;
        rpszDllsOut[n] = NULL;
    }

    //int arg = 1;

    rpszDllsRaw[nDlls++] = HELPER_DLL_FILE;
    
    /////////////////////////////////////////////////////////// Validate DLLs.
    //
    for (DWORD n = 0; n < nDlls; n++) {
        CHAR szDllPath[1024];
        PCHAR pszFilePart = NULL;

        if (!GetFullPathNameA(rpszDllsRaw[n], ARRAYSIZE(szDllPath), szDllPath, &pszFilePart)) {
            printf("withdll.exe: Error: %s is not a valid path name..\n",
                   rpszDllsRaw[n]);
            return 9002;
        }

        DWORD c = (DWORD)strlen(szDllPath) + 1;
        PCHAR psz = new CHAR [c];
        StringCchCopyA(psz, c, szDllPath);
        rpszDllsOut[n] = psz;

        HMODULE hDll = LoadLibraryExA(rpszDllsOut[n], NULL, DONT_RESOLVE_DLL_REFERENCES);
        if (hDll == NULL) {
            printf("withdll.exe: Error: %s failed to load (error %d).\n",
                   rpszDllsOut[n],
                   GetLastError());
            return 9003;
        }

        ExportContext ec;
        ec.fHasOrdinal1 = FALSE;
        ec.nExports = 0;
        DetourEnumerateExports(hDll, &ec, ExportCallback);
        FreeLibrary(hDll);

        if (!ec.fHasOrdinal1) {
            printf("withdll.exe: Error: %s does not export ordinal #1.\n",
                   rpszDllsOut[n]);
            printf("             See help entry DetourCreateProcessWithDllEx in Detours.chm.\n");
            return 9004;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    CHAR szCommand[2048];
    CHAR szExe[1024] = TARGET_EXE_FILE;
    CHAR szFullExe[1024] = "\0";
    PCHAR pszFileExe = NULL;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    szCommand[0] = L'\0';

    DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

    SetLastError(0);
    SearchPathA(NULL, szExe, ".exe", ARRAYSIZE(szFullExe), szFullExe, &pszFileExe);
    if (!DetourCreateProcessWithDllsA(szFullExe[0] ? szFullExe : NULL, szCommand,
                                     NULL, NULL, TRUE, dwFlags, NULL, NULL,
                                     &si, &pi, nDlls, rpszDllsOut, NULL)) {
        DWORD dwError = GetLastError();
        printf("withdll.exe: DetourCreateProcessWithDllEx failed: %d\n", dwError);
        if (dwError == ERROR_INVALID_HANDLE) {
#if DETOURS_64BIT
            printf("withdll.exe: Can't detour a 32-bit target process from a 64-bit parent process.\n");
#else
            printf("withdll.exe: Can't detour a 64-bit target process from a 32-bit parent process.\n");
#endif
        }
        ExitProcess(9009);
    }

    ResumeThread(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD dwResult = 0;
    if (!GetExitCodeProcess(pi.hProcess, &dwResult)) {
        printf("withdll.exe: GetExitCodeProcess failed: %d\n", GetLastError());
        return 9010;
    }

    for (DWORD n = 0; n < nDlls; n++) {
        if (rpszDllsOut[n] != NULL) {
            delete[] rpszDllsOut[n];
            rpszDllsOut[n] = NULL;
        }
    }

    return dwResult;
}
//
///////////////////////////////////////////////////////////////// End of File.
