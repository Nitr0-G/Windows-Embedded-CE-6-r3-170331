//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft shared
// source or premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license agreement,
// you are not authorized to use this source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the SOURCE.RTF on your install media or the root of your tools installation.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#include "storeincludes.hpp"

MountTable_t* g_pMountTable;

static HANDLE hRootFindAPI;

#ifdef UNDER_CE

const PFNVOID RootEnumAPIMethods[NUM_FIND_APIS] = {
    (PFNVOID)ROOTFS_FindClose,
    (PFNVOID)0,
    (PFNVOID)ROOTFS_FindNextFileW
};


const ULONGLONG RootEnumAPISigs[NUM_FIND_APIS] = {
    FNSIG1(DW),                 // FindClose
    FNSIG0(),                   // unused
    FNSIG2(DW,IO_PDW)           // FindNextFile
};

#endif // UNDER_CE

LRESULT InitializeVirtualRoot ()
{    
    g_pMountTable = new MountTable_t ();
    if (!g_pMountTable) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

#ifdef UNDER_CE
    // Initialize the virtual root file system by registering the 
    // root enumeration handle-based API set.
    hRootFindAPI = CreateAPISet (const_cast<CHAR*> ("RTFS"), NUM_FIND_APIS, RootEnumAPIMethods, RootEnumAPISigs);
    RegisterAPISet (hRootFindAPI, HT_FIND | REGISTER_APISET_TYPE);

    // Reserve the network name root.
    HINSTANCE hFileSys = LoadLibrary (L"filesys.dll");
    if (hFileSys) {
        
        WORD StringID = AFS_ROOTNUM_NETWORK + 1;
        HRSRC hResInfo = FindResource(hFileSys, MAKEINTRESOURCE((StringID >> 4) + 1), RT_STRING);

        if (hResInfo) {
            HANDLE hStringSeg = LoadResource(hFileSys, hResInfo);
            if (hStringSeg) {
                const WCHAR* pResString = (LPWSTR)LockResource(hStringSeg);
                size_t ResChars;
                if (pResString) {
                    StringID &= 0x0F;
                    while (1) {
                        ResChars = *pResString++;
                        if (StringID-- == 0) {
                            break;
                        }
                        pResString += ResChars;
                    }
                    DEBUGCHK(ResChars > 0);
                    DEBUGCHK(!pResString[ResChars-1]);
                    g_pMountTable->ReserveVolumeName (pResString, AFS_ROOTNUM_NETWORK, 
                        (HANDLE)GetCurrentProcessId ());
                }
            }
        }
        FreeLibrary (hFileSys);
    }
#endif

    return ERROR_SUCCESS;
}


// determine if a path is, or is a sub-dir of, the root directory
BOOL IsPathInSystemDir(LPCWSTR lpszPath)
{
    BOOL fRet = FALSE;

    while ((*lpszPath == L'\\') || (*lpszPath == L'/')) {
        lpszPath++;
    }

    if (wcslen(lpszPath) > SYSDIRLEN) {
        if ((wcsnicmp(lpszPath, SYSDIRNAME, SYSDIRLEN) == 0) &&
                ((*(lpszPath+SYSDIRLEN) == L'\\') ||
                (*(lpszPath+SYSDIRLEN) == L'/')) ||
                (*lpszPath == L'\0')) {
            // matches the system directory
            fRet = TRUE;
        }
    }
    return fRet;
}

// determine if a path is in the root directory, but not a sub-dir
BOOL IsPathInRootDir(LPCWSTR lpszPath)
{
    // skip all preceding slashes
    while ((*lpszPath == L'\\') || (*lpszPath == L'/')) {
        lpszPath++;
    }

    // make sure there are no more slashes in the string
    while ((L'\0' != *lpszPath) && (*lpszPath != L'\\') && (*lpszPath != L'/')) {
        lpszPath++;
    }

    // if this were not canonicalized, this would fail for paths ending in \

    // if we reached the end of the string, then this is not a sub-dir
    return (L'\0' == *lpszPath);
}

BOOL IsConsoleName (const WCHAR* pName)
{
    // If a file name is named "reg:" or "con" we assume it is a console
    // device. This is for legacy resons with relfsd.
    if ((0 == wcsicmp (pName, L"\\reg:")) || 
        (0 == wcsicmp (pName, L"\\con"))) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsPsuedoDeviceName (const WCHAR* pName)
{
    // If a file name ends in a colon, assume it is a pseudo device.
    // This is typically something like L"\\Storage Card\\VOL:"
    size_t NameChars;
    if (FAILED (StringCchLengthW (pName, MAX_PATH, &NameChars))) {
        return FALSE;
    }
    return pName[NameChars-1] == L':';
}

BOOL IsLegacyDeviceName (const WCHAR* pName)
{   
    if (*pName == L'\\') {
        pName++;
    }
    
    // Five char names where the 5th char is ':' are
    // considered legacy device names and are passed to device
    // manager for processing.
    size_t NameChars;
    if (FAILED (StringCchLengthW (pName, MAX_PATH, &NameChars))) {
        return FALSE;
    }    
    return (NameChars == 5) && (pName[4] == L':');
}

BOOL IsSecondaryStreamName (const WCHAR* pName)
{
    // TODO: What exactly does a stream name look like?
    return FALSE;
}

BOOL FileExistsInROM (const WCHAR* pName)
{
    return (INVALID_FILE_ATTRIBUTES != ROM_GetFileAttributesW (pName));
}

// Search all ROM volumes for a file and return attributes of the first one found.
DWORD ROM_GetFileAttributesW (const WCHAR* pFileName)
{
    DWORD Attributes = INVALID_FILE_ATTRIBUTES;
    ROMVolumeListNode* pROMNode = g_pMountTable->GetROMVolumeList ();

    // Check all available ROM volumes.
    for (pROMNode = g_pMountTable->GetROMVolumeList ();
         pROMNode && (INVALID_FILE_ATTRIBUTES == Attributes);
         pROMNode = pROMNode->pNext) {
            
        HANDLE hVolume = pROMNode->hVolume;

        // TODO: VolumeAccessCheck for permission to the file.

        if (hVolume) {
            __try {
                Attributes = AFS_GetFileAttributesW (hVolume, pFileName);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                SetLastError (ERROR_EXCEPTION_IN_SERVICE);
            }
        }
    }

    return Attributes;
}

HANDLE ROM_CreateFileW (HANDLE hProcess, const WCHAR* pPathName, 
    DWORD Access, DWORD ShareMode, SECURITY_ATTRIBUTES* pSecurityAttributes, 
    DWORD Create, DWORD FlagsAndAttributes, HANDLE hTemplate)
{
    if (IsPsuedoDeviceName (pPathName)) {
        // Disallow opening \Windows\VOL:
        SetLastError (ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }

    HANDLE h = INVALID_HANDLE_VALUE;
    ROMVolumeListNode* pROMNode;

    // Check all available ROM volumes.
    for (pROMNode = g_pMountTable->GetROMVolumeList ();
         pROMNode && (INVALID_HANDLE_VALUE == h);
         pROMNode = pROMNode->pNext) {

        HANDLE hVolume = pROMNode->hVolume;

        // TODO: VolumeAccessCheck for permission to the file.

        if (hVolume) {
            __try {
                h = AFS_CreateFileW (hVolume, hProcess, pPathName, Access,
                    ShareMode, pSecurityAttributes, Create, FlagsAndAttributes,
                    hTemplate, NULL, 0);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                SetLastError (ERROR_EXCEPTION_IN_SERVICE);
            }
        }
    }

    return h;
}

static BOOL IsROMFileShadowed (const WCHAR* pFileName, ROMVolumeListNode* pNextROM)
{
    BOOL fShadowed = FALSE;

    WCHAR PathName[MAX_PATH];

    if (FAILED (StringCchCopyW (PathName, MAX_PATH, L"\\")) ||
        FAILED (StringCchCatW (PathName, MAX_PATH, SYSDIRNAME)) ||
        FAILED (StringCchCatW (PathName, MAX_PATH, L"\\")) ||
        FAILED (StringCchCatW (PathName, MAX_PATH, pFileName))) {
        // Unable to build the full path?
        DEBUGCHK (0);
        return FALSE;
    }    
    
    // First, check to see if the file exists on the root file system.
    HANDLE hVolume;
    int RootIndex = g_pMountTable->GetRootVolumeIndex ();    
    LRESULT lResult = g_pMountTable->GetVolumeFromIndex (RootIndex, &hVolume);
    if (ERROR_SUCCESS == lResult) {
        __try {
            if (INVALID_FILE_ATTRIBUTES != AFS_GetFileAttributesW (hVolume, PathName)) {
                fShadowed = TRUE;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError (ERROR_EXCEPTION_IN_SERVICE);
        }
    }

    // If the file did not exist on the root file system, check every ROM 
    // volume prior to pCurrentROM in the list.
    ROMVolumeListNode* pCurrentROM = g_pMountTable->GetROMVolumeList ();
    while (!fShadowed && pCurrentROM && pCurrentROM != pNextROM) {

        hVolume = pCurrentROM->hVolume;
        if (hVolume) {
            __try {
                if (INVALID_FILE_ATTRIBUTES != AFS_GetFileAttributesW (hVolume, PathName)) {
                    fShadowed = TRUE;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                SetLastError (ERROR_EXCEPTION_IN_SERVICE);
            }
        }
        pCurrentROM = pCurrentROM->pNext;
    }

    return fShadowed;
        
}

static BOOL FindNextOnROM (RootFileEnumHandle *pEnum, WIN32_FIND_DATA* pFindData)
{
    // Expect that the root fs shadows ROM, so root should always be enumerated
    // prior to enumerating ROM.
    DEBUGCHK (!(pEnum->EnumFlags & EnumRoot)); 
    DEBUGCHK (pEnum->EnumFlags & EnumROMs);
    
    // Always use a local copy of the find data structure because we internally
    // use the cFileName field and we don't want it to be asynchronously 
    // modified by the caller process.
    WIN32_FIND_DATA FindDataLocal;

    if (!pEnum->pNextROM) {
        pEnum->pNextROM = g_pMountTable->GetROMVolumeList ();
    }
    
    while (pEnum->pNextROM) {

        if (INVALID_HANDLE_VALUE == pEnum->hInternalSearchContext) {

            WCHAR SearchSpec[MAX_PATH];
            if (FAILED (StringCchCopyW (SearchSpec, MAX_PATH, pEnum->SearchSpec))) {
                DEBUGCHK (0);
                SetLastError (ERROR_FILENAME_EXCED_RANGE);
                break;
            }

            HANDLE hProcess = reinterpret_cast<HANDLE> (GetCurrentProcessId());
            
            HANDLE hVolume = pEnum->pNextROM->hVolume;
            if (!hVolume) {
                pEnum->pNextROM = pEnum->pNextROM->pNext;
                continue;
            }

            // Invoke the internal volume-based FindFirstFileW function. This will
            // return a kernel search handle owned by this process which can be passed
            // to FindClose and FindNextFileW internally when searching for more files.
            __try {
                pEnum->hInternalSearchContext = AFS_FindFirstFileW (hVolume, hProcess,
                    SearchSpec, &FindDataLocal, sizeof (WIN32_FIND_DATAW));
            } __except (EXCEPTION_EXECUTE_HANDLER) {
            }

            if (INVALID_HANDLE_VALUE == pEnum->hInternalSearchContext) {
                pEnum->pNextROM = pEnum->pNextROM->pNext;
                continue;
            }

            if (IsROMFileShadowed (FindDataLocal.cFileName, pEnum->pNextROM)) {
                // Found a file, but it is shadowed so we'll skip it.
                continue;
            }

            // Found a ROM file; copy local find data to caller's buffer.
            VERIFY (CeSafeCopyMemory (pFindData, &FindDataLocal, sizeof (WIN32_FIND_DATA)));
            return TRUE;
                        
        } else {

            // Perform FindNextFile on our internal search handle. This will trap to
            // the appropriate ROM file system we're currently searching.
            if (!FindNextFileW (pEnum->hInternalSearchContext, &FindDataLocal)) {
                // Finished enumerating this ROM file system. Continue with the next
                // ROM file system, if any exists, and close the internal find handle.
                VERIFY (FindClose (pEnum->hInternalSearchContext));
                pEnum->hInternalSearchContext = INVALID_HANDLE_VALUE;
                pEnum->pNextROM = pEnum->pNextROM->pNext;
                continue;
            }

            if (IsROMFileShadowed (FindDataLocal.cFileName, pEnum->pNextROM)) {
                // Found a file, but it is shadowed so we'll skip it.
                continue;
            }

            // Found a ROM file; copy local find data to caller's buffer.
            VERIFY (CeSafeCopyMemory (pFindData, &FindDataLocal, sizeof (WIN32_FIND_DATA)));
            return TRUE;
        }
        
    } 
    
    SetLastError (ERROR_NO_MORE_FILES);
    return FALSE;
}

static BOOL FindNextOnRootFS (RootFileEnumHandle *pEnum, WIN32_FIND_DATA* pFindData)
{
    DEBUGCHK (pEnum->EnumFlags & EnumRoot);

    if (INVALID_HANDLE_VALUE == pEnum->hInternalSearchContext) {

        WCHAR SearchSpec[MAX_PATH];
        if (FAILED (StringCchCopyW (SearchSpec, MAX_PATH, pEnum->SearchSpec))) {
            DEBUGCHK (0);
            SetLastError (ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }

        HANDLE hProcess = reinterpret_cast<HANDLE> (GetCurrentProcessId());
        
        HANDLE hVolume;
        int RootIndex = g_pMountTable->GetRootVolumeIndex ();
        LRESULT lResult = g_pMountTable->GetVolumeFromIndex (RootIndex, &hVolume);
        if (ERROR_SUCCESS != lResult) {
            return FALSE;
        }

        // Invoke the internal volume-based FindFirstFileW function. This will
        // return a kernel search handle owned by this process which can be passed
        // to FindClose and FindNextFileW internally when searching for more files.
        WIN32_FIND_DATAW FindData;
        
        __try {
            pEnum->hInternalSearchContext = AFS_FindFirstFileW (hVolume, hProcess,
                SearchSpec, &FindData, sizeof (WIN32_FIND_DATAW));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        
        if (INVALID_HANDLE_VALUE == pEnum->hInternalSearchContext) {
            return FALSE;
        }        

        // TODO: If in root dir, does this match a mount point? If so, skip.

        return CeSafeCopyMemory (pFindData, &FindData, sizeof (WIN32_FIND_DATAW));
                    
    } else {

        // Perform FindNextFile on our internal search handle. This will trap to
        // the appropriate ROM file system we're currently searching.
        if (!FindNextFileW (pEnum->hInternalSearchContext, pFindData)) {
            // Finished enumerating the root file system.
            VERIFY (FindClose (pEnum->hInternalSearchContext));
            pEnum->hInternalSearchContext = INVALID_HANDLE_VALUE;
            return FALSE;
        }
        
        // TODO: If in root dir, does this match a mount point? If so, skip.

        return TRUE;
    }
}

static BOOL FindNextMountDir (RootFileEnumHandle *pEnum, WIN32_FIND_DATA* pFindData)
{
    BOOL fDone = FALSE;

    int MaxIndex = g_pMountTable->GetHighMountIndex ();
    
    for (   pEnum->MountIndex = pEnum->MountIndex + 1; 
            pEnum->MountIndex <= MaxIndex; 
            pEnum->MountIndex ++) {

        HANDLE hVolume;        
        DWORD MountFlags;
        LRESULT lResult = g_pMountTable->GetVolumeFromIndex (pEnum->MountIndex, &hVolume, &MountFlags);
        if (ERROR_SUCCESS != lResult) {
            continue;
        }

        if (AFS_FLAG_HIDDEN & MountFlags) {
            // Skip hidden volumes.
            continue;
        }

        if ((AFS_FLAG_ROOTFS & MountFlags) &&
            (0 != pEnum->SearchSpecChars)) {
            // Skip the root volume unless explicitly enumerating it (no wildcard).
            continue;
        }
        
        // Get the path of the folder where this volume is mounted.
        WCHAR MountFolder[MAX_PATH];
        size_t MountFolderChars;
        lResult = g_pMountTable->GetMountName (pEnum->MountIndex, MountFolder, MAX_PATH);
        if (ERROR_SUCCESS != lResult ||
            FAILED (StringCchLengthW (MountFolder, MAX_PATH, &MountFolderChars))) {
            DEBUGCHK (0);
            continue;
        }

        // Check for a match.
        if (!MatchesWildcardMask (pEnum->SearchSpecChars, pEnum->SearchSpec, 
            MountFolderChars, MountFolder)) {
            // Does not match the current wildcard.
            continue;
        }

        static const SYSTEMTIME st = {1998,1,0,1,12,0,0,0};  
        FILETIME ft;
        SystemTimeToFileTime (&st, &ft);

        pFindData->ftCreationTime = ft;
        pFindData->ftLastAccessTime = ft;
        pFindData->ftLastWriteTime = ft;
        pFindData->nFileSizeHigh = 0;
        pFindData->nFileSizeLow = 0;
        pFindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

        if (!(AFS_FLAG_PERMANENT & MountFlags)) {
            pFindData->dwFileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
        }
            
        if (AFS_FLAG_SYSTEM & MountFlags) {
            pFindData->dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
        }

        VERIFY (SUCCEEDED (StringCchCopy (pFindData->cFileName, MAX_PATH, MountFolder)));

#ifdef UNDER_CE
        pFindData->dwOID = (CEOID)((SYSTEM_MNTVOLUME << 28) | pEnum->MountIndex);
#endif // UNDER_CE

        return TRUE;
    }

    pEnum->MountIndex = INVALID_MOUNT_INDEX;

    SetLastError (ERROR_NO_MORE_FILES);
    return FALSE;
}

BOOL ROOTFS_FindNextFileW (RootFileEnumHandle *pEnum, WIN32_FIND_DATA* pFindData)
{
    if (EnumMounts & pEnum->EnumFlags) {
        // Search the mount points in the virtual root directory.
        if (FindNextMountDir (pEnum, pFindData)) {
            return TRUE;
        }
        pEnum->EnumFlags &= ~EnumMounts;
        // Fall through and try the next enum type.
    }

    if (EnumRoot & pEnum->EnumFlags) {
        // Search for files and directories in this path on the root 
        // file system volume.
        if (FindNextOnRootFS (pEnum, pFindData)) {
            return TRUE;
        }
        pEnum->EnumFlags &= ~EnumRoot;
        // Fall through and try the next enum type.
    }

    if (EnumROMs & pEnum->EnumFlags) {
        // Search for files in ROM file systems in the virtual system
        // directory.
        if (FindNextOnROM (pEnum, pFindData)) {
            return TRUE;
        }
        pEnum->EnumFlags &= ~EnumROMs;
        // Enumeration is complete.
    } 

    DEBUGCHK (pEnum->EnumFlags == 0);
    SetLastError (ERROR_NO_MORE_FILES);
    return FALSE;
}

HANDLE ROOTFS_FindFirstFileW(HANDLE hProcess, const WCHAR* pSearchSpec, WIN32_FIND_DATAW* pFindData,
    DWORD SizeOfFindData)
{   
    // We'll only ever get here if the path is in the root directory or windows 
    // directory which are handled slightly differently from normal file system 
    // enumeration. In these "merged" directories, multiple file systems must be 
    // enumerated to give the appearance of a single file system:
    //
    //     Root: Contains merged contents of the root file system and all mount
    //           point folders.
    //
    //     System: Contains merged contents of \windows off the root file system 
    //             and the contents of \windows on all external ROM file systems.
    //
    DEBUGCHK (IsPathInRootDir (pSearchSpec) || IsPathInSystemDir (pSearchSpec));

#ifdef UNDER_CE
    if (sizeof (WIN32_FIND_DATAW) != SizeOfFindData) {
        DEBUGCHK (0); // FindFirstFileW_Trap macro was called directly w/out proper size.
        SetLastError (ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
#endif // UNDER_CE

    // Skip the 1st slash
    if (L'\\' == *pSearchSpec) {
        pSearchSpec ++;
    }
    
    size_t SearchSpecChars;
    if (FAILED (StringCchLengthW (pSearchSpec, MAX_PATH, &SearchSpecChars))) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    DWORD EnumSize = sizeof (RootFileEnumHandle) + sizeof (WCHAR) * SearchSpecChars;
    RootFileEnumHandle* pEnum = reinterpret_cast<RootFileEnumHandle*> (new BYTE[EnumSize]);
    if (!pEnum) {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    if (FAILED (StringCchCopyNW (pEnum->SearchSpec, SearchSpecChars + 1, 
        pSearchSpec, SearchSpecChars))) {
        delete[] reinterpret_cast<BYTE*> (pEnum);
        SetLastError (ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    // Initialize the enumeration handle structure.
    pEnum->SearchSpecChars = SearchSpecChars;
    pEnum->hInternalSearchContext = INVALID_HANDLE_VALUE;
    pEnum->MountIndex = INVALID_MOUNT_INDEX;
    pEnum->pNextROM = NULL;

    // Always enumerate the root file system in root and \windows.
    pEnum->EnumFlags = EnumRoot;

    if (IsPathInRootDir (pEnum->SearchSpec)) {
        pEnum->EnumFlags |= EnumMounts;
    }

    if (IsPathInSystemDir (pEnum->SearchSpec)) {
        DEBUGCHK (!(pEnum->EnumFlags & EnumMounts));
        pEnum->EnumFlags |= EnumROMs;
    }

    // Internally just invoke ROOTFS_FindNextFile to perform the first
    // iteration of the enumeration.
    if (!ROOTFS_FindNextFileW (pEnum, pFindData)) {
        delete[] reinterpret_cast<BYTE*> (pEnum);
        SetLastError (ERROR_NO_MORE_FILES);
        return INVALID_HANDLE_VALUE;
    }

    HANDLE h = CreateAPIHandle (hRootFindAPI, pEnum);
    if ( !h ) {
        delete[] reinterpret_cast<BYTE*> (pEnum);
        return INVALID_HANDLE_VALUE;
    }

    return h;
}

BOOL ROOTFS_FindClose(RootFileEnumHandle *pEnum)
{
    if (INVALID_HANDLE_VALUE != pEnum->hInternalSearchContext) {
        VERIFY (FindClose (pEnum->hInternalSearchContext));
    }
    delete[] reinterpret_cast<BYTE*> (pEnum);
    return TRUE;
}

