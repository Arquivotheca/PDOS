/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  ntdll - implementation of Windows based on PDOS/386              */
/*                                                                   */
/*********************************************************************/

#include <windows.h>

#include <pos.h>


/* auto-genned dummy functions */

void WINAPI NtAccessCheck(void)
{
    size_t len = 29;
    PosWriteFile(1, "NtAccessCheck unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtAdjustPrivilegesToken(void)
{
    size_t len = 39;
    PosWriteFile(1, "NtAdjustPrivilegesToken unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtAllocateLocallyUniqueId(void)
{
    size_t len = 41;
    PosWriteFile(1, "NtAllocateLocallyUniqueId unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCancelTimer(void)
{
    size_t len = 29;
    PosWriteFile(1, "NtCancelTimer unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtClose(void)
{
    size_t len = 23;
    PosWriteFile(1, "NtClose unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateDirectoryObject(void)
{
    size_t len = 39;
    PosWriteFile(1, "NtCreateDirectoryObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateEvent(void)
{
    size_t len = 29;
    PosWriteFile(1, "NtCreateEvent unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateFile(void)
{
    size_t len = 28;
    PosWriteFile(1, "NtCreateFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateKey(void)
{
    size_t len = 27;
    PosWriteFile(1, "NtCreateKey unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateMailslotFile(void)
{
    size_t len = 36;
    PosWriteFile(1, "NtCreateMailslotFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateMutant(void)
{
    size_t len = 30;
    PosWriteFile(1, "NtCreateMutant unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateSection(void)
{
    size_t len = 31;
    PosWriteFile(1, "NtCreateSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateSemaphore(void)
{
    size_t len = 33;
    PosWriteFile(1, "NtCreateSemaphore unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateTimer(void)
{
    size_t len = 29;
    PosWriteFile(1, "NtCreateTimer unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtCreateToken(void)
{
    size_t len = 29;
    PosWriteFile(1, "NtCreateToken unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtDuplicateToken(void)
{
    size_t len = 32;
    PosWriteFile(1, "NtDuplicateToken unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtFlushBuffersFile(void)
{
    size_t len = 34;
    PosWriteFile(1, "NtFlushBuffersFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtFsControlFile(void)
{
    size_t len = 31;
    PosWriteFile(1, "NtFsControlFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtLoadKey(void)
{
    size_t len = 25;
    PosWriteFile(1, "NtLoadKey unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtLockFile(void)
{
    size_t len = 26;
    PosWriteFile(1, "NtLockFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtLockVirtualMemory(void)
{
    size_t len = 35;
    PosWriteFile(1, "NtLockVirtualMemory unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtMapViewOfSection(void)
{
    size_t len = 34;
    PosWriteFile(1, "NtMapViewOfSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtNotifyChangeDirectoryFile(void)
{
    size_t len = 43;
    PosWriteFile(1, "NtNotifyChangeDirectoryFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenDirectoryObject(void)
{
    size_t len = 37;
    PosWriteFile(1, "NtOpenDirectoryObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenEvent(void)
{
    size_t len = 27;
    PosWriteFile(1, "NtOpenEvent unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenFile(void)
{
    size_t len = 26;
    PosWriteFile(1, "NtOpenFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenKey(void)
{
    size_t len = 25;
    PosWriteFile(1, "NtOpenKey unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenMutant(void)
{
    size_t len = 28;
    PosWriteFile(1, "NtOpenMutant unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenProcessToken(void)
{
    size_t len = 34;
    PosWriteFile(1, "NtOpenProcessToken unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenSection(void)
{
    size_t len = 29;
    PosWriteFile(1, "NtOpenSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenSemaphore(void)
{
    size_t len = 31;
    PosWriteFile(1, "NtOpenSemaphore unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenSymbolicLinkObject(void)
{
    size_t len = 40;
    PosWriteFile(1, "NtOpenSymbolicLinkObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtOpenThreadToken(void)
{
    size_t len = 33;
    PosWriteFile(1, "NtOpenThreadToken unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtPrivilegeCheck(void)
{
    size_t len = 32;
    PosWriteFile(1, "NtPrivilegeCheck unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryAttributesFile(void)
{
    size_t len = 37;
    PosWriteFile(1, "NtQueryAttributesFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryDirectoryFile(void)
{
    size_t len = 36;
    PosWriteFile(1, "NtQueryDirectoryFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryDirectoryObject(void)
{
    size_t len = 38;
    PosWriteFile(1, "NtQueryDirectoryObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryEaFile(void)
{
    size_t len = 29;
    PosWriteFile(1, "NtQueryEaFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryEvent(void)
{
    size_t len = 28;
    PosWriteFile(1, "NtQueryEvent unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryFullAttributesFile(void)
{
    size_t len = 41;
    PosWriteFile(1, "NtQueryFullAttributesFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryInformationFile(void)
{
    size_t len = 38;
    PosWriteFile(1, "NtQueryInformationFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryInformationProcess(void)
{
    size_t len = 41;
    PosWriteFile(1, "NtQueryInformationProcess unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryInformationThread(void)
{
    size_t len = 40;
    PosWriteFile(1, "NtQueryInformationThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryInformationToken(void)
{
    size_t len = 39;
    PosWriteFile(1, "NtQueryInformationToken unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryObject(void)
{
    size_t len = 29;
    PosWriteFile(1, "NtQueryObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQuerySecurityObject(void)
{
    size_t len = 37;
    PosWriteFile(1, "NtQuerySecurityObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQuerySemaphore(void)
{
    size_t len = 32;
    PosWriteFile(1, "NtQuerySemaphore unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQuerySymbolicLinkObject(void)
{
    size_t len = 41;
    PosWriteFile(1, "NtQuerySymbolicLinkObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQuerySystemInformation(void)
{
    size_t len = 40;
    PosWriteFile(1, "NtQuerySystemInformation unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQuerySystemTime(void)
{
    size_t len = 33;
    PosWriteFile(1, "NtQuerySystemTime unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryTimer(void)
{
    size_t len = 28;
    PosWriteFile(1, "NtQueryTimer unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryTimerResolution(void)
{
    size_t len = 38;
    PosWriteFile(1, "NtQueryTimerResolution unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryValueKey(void)
{
    size_t len = 31;
    PosWriteFile(1, "NtQueryValueKey unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryVirtualMemory(void)
{
    size_t len = 36;
    PosWriteFile(1, "NtQueryVirtualMemory unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtQueryVolumeInformationFile(void)
{
    size_t len = 44;
    PosWriteFile(1, "NtQueryVolumeInformationFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtReadFile(void)
{
    size_t len = 26;
    PosWriteFile(1, "NtReadFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtSetEaFile(void)
{
    size_t len = 27;
    PosWriteFile(1, "NtSetEaFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtSetEvent(void)
{
    size_t len = 26;
    PosWriteFile(1, "NtSetEvent unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtSetInformationFile(void)
{
    size_t len = 36;
    PosWriteFile(1, "NtSetInformationFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtSetInformationThread(void)
{
    size_t len = 38;
    PosWriteFile(1, "NtSetInformationThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtSetInformationToken(void)
{
    size_t len = 37;
    PosWriteFile(1, "NtSetInformationToken unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtSetSecurityObject(void)
{
    size_t len = 35;
    PosWriteFile(1, "NtSetSecurityObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtSetTimer(void)
{
    size_t len = 26;
    PosWriteFile(1, "NtSetTimer unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtSetTimerResolution(void)
{
    size_t len = 36;
    PosWriteFile(1, "NtSetTimerResolution unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtSetValueKey(void)
{
    size_t len = 29;
    PosWriteFile(1, "NtSetValueKey unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtUnlockFile(void)
{
    size_t len = 28;
    PosWriteFile(1, "NtUnlockFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtUnlockVirtualMemory(void)
{
    size_t len = 37;
    PosWriteFile(1, "NtUnlockVirtualMemory unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtUnmapViewOfSection(void)
{
    size_t len = 36;
    PosWriteFile(1, "NtUnmapViewOfSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI NtWriteFile(void)
{
    size_t len = 27;
    PosWriteFile(1, "NtWriteFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlAbsoluteToSelfRelativeSD(void)
{
    size_t len = 43;
    PosWriteFile(1, "RtlAbsoluteToSelfRelativeSD unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlAcquirePebLock(void)
{
    size_t len = 33;
    PosWriteFile(1, "RtlAcquirePebLock unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlAddAccessAllowedAce(void)
{
    size_t len = 38;
    PosWriteFile(1, "RtlAddAccessAllowedAce unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlAddAccessAllowedAceEx(void)
{
    size_t len = 40;
    PosWriteFile(1, "RtlAddAccessAllowedAceEx unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlAddAccessDeniedAceEx(void)
{
    size_t len = 39;
    PosWriteFile(1, "RtlAddAccessDeniedAceEx unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlAddAce(void)
{
    size_t len = 25;
    PosWriteFile(1, "RtlAddAce unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlAllocateHeap(void)
{
    size_t len = 31;
    PosWriteFile(1, "RtlAllocateHeap unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlAppendUnicodeStringToString(void)
{
    size_t len = 46;
    PosWriteFile(1, "RtlAppendUnicodeStringToString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlAppendUnicodeToString(void)
{
    size_t len = 40;
    PosWriteFile(1, "RtlAppendUnicodeToString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlCheckRegistryKey(void)
{
    size_t len = 35;
    PosWriteFile(1, "RtlCheckRegistryKey unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlCompareUnicodeString(void)
{
    size_t len = 39;
    PosWriteFile(1, "RtlCompareUnicodeString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlConvertSidToUnicodeString(void)
{
    size_t len = 44;
    PosWriteFile(1, "RtlConvertSidToUnicodeString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlConvertToAutoInheritSecurityObject(void)
{
    size_t len = 53;
    PosWriteFile(1, "RtlConvertToAutoInheritSecurityObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlCopySid(void)
{
    size_t len = 26;
    PosWriteFile(1, "RtlCopySid unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlCopyUnicodeString(void)
{
    size_t len = 36;
    PosWriteFile(1, "RtlCopyUnicodeString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlCreateAcl(void)
{
    size_t len = 28;
    PosWriteFile(1, "RtlCreateAcl unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlCreateQueryDebugBuffer(void)
{
    size_t len = 41;
    PosWriteFile(1, "RtlCreateQueryDebugBuffer unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlCreateSecurityDescriptor(void)
{
    size_t len = 43;
    PosWriteFile(1, "RtlCreateSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlCreateUnicodeStringFromAsciiz(void)
{
    size_t len = 48;
    PosWriteFile(1, "RtlCreateUnicodeStringFromAsciiz unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlDeleteSecurityObject(void)
{
    size_t len = 39;
    PosWriteFile(1, "RtlDeleteSecurityObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlDestroyQueryDebugBuffer(void)
{
    size_t len = 42;
    PosWriteFile(1, "RtlDestroyQueryDebugBuffer unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlDowncaseUnicodeString(void)
{
    size_t len = 40;
    PosWriteFile(1, "RtlDowncaseUnicodeString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlEnterCriticalSection(void)
{
    size_t len = 39;
    PosWriteFile(1, "RtlEnterCriticalSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlEqualPrefixSid(void)
{
    size_t len = 33;
    PosWriteFile(1, "RtlEqualPrefixSid unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlEqualSid(void)
{
    size_t len = 27;
    PosWriteFile(1, "RtlEqualSid unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlEqualUnicodeString(void)
{
    size_t len = 37;
    PosWriteFile(1, "RtlEqualUnicodeString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlFirstFreeAce(void)
{
    size_t len = 31;
    PosWriteFile(1, "RtlFirstFreeAce unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlFreeHeap(void)
{
    size_t len = 27;
    PosWriteFile(1, "RtlFreeHeap unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlFreeUnicodeString(void)
{
    size_t len = 36;
    PosWriteFile(1, "RtlFreeUnicodeString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlGetAce(void)
{
    size_t len = 25;
    PosWriteFile(1, "RtlGetAce unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlGetControlSecurityDescriptor(void)
{
    size_t len = 47;
    PosWriteFile(1, "RtlGetControlSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlGetDaclSecurityDescriptor(void)
{
    size_t len = 44;
    PosWriteFile(1, "RtlGetDaclSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlGetGroupSecurityDescriptor(void)
{
    size_t len = 45;
    PosWriteFile(1, "RtlGetGroupSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlGetOwnerSecurityDescriptor(void)
{
    size_t len = 45;
    PosWriteFile(1, "RtlGetOwnerSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlIdentifierAuthoritySid(void)
{
    size_t len = 41;
    PosWriteFile(1, "RtlIdentifierAuthoritySid unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlInitUnicodeString(void)
{
    size_t len = 36;
    PosWriteFile(1, "RtlInitUnicodeString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlInitializeSid(void)
{
    size_t len = 32;
    PosWriteFile(1, "RtlInitializeSid unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlLeaveCriticalSection(void)
{
    size_t len = 39;
    PosWriteFile(1, "RtlLeaveCriticalSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlLengthSecurityDescriptor(void)
{
    size_t len = 43;
    PosWriteFile(1, "RtlLengthSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlLengthSid(void)
{
    size_t len = 28;
    PosWriteFile(1, "RtlLengthSid unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlNtStatusToDosError(void)
{
    size_t len = 37;
    PosWriteFile(1, "RtlNtStatusToDosError unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlPrefixUnicodeString(void)
{
    size_t len = 38;
    PosWriteFile(1, "RtlPrefixUnicodeString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlQueryProcessDebugInformation(void)
{
    size_t len = 47;
    PosWriteFile(1, "RtlQueryProcessDebugInformation unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlQueryRegistryValues(void)
{
    size_t len = 38;
    PosWriteFile(1, "RtlQueryRegistryValues unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlReleasePebLock(void)
{
    size_t len = 33;
    PosWriteFile(1, "RtlReleasePebLock unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlSetControlSecurityDescriptor(void)
{
    size_t len = 47;
    PosWriteFile(1, "RtlSetControlSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlSetCurrentDirectory_U(void)
{
    size_t len = 40;
    PosWriteFile(1, "RtlSetCurrentDirectory_U unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlSetDaclSecurityDescriptor(void)
{
    size_t len = 44;
    PosWriteFile(1, "RtlSetDaclSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlSetGroupSecurityDescriptor(void)
{
    size_t len = 45;
    PosWriteFile(1, "RtlSetGroupSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlSetOwnerSecurityDescriptor(void)
{
    size_t len = 45;
    PosWriteFile(1, "RtlSetOwnerSecurityDescriptor unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlSubAuthorityCountSid(void)
{
    size_t len = 39;
    PosWriteFile(1, "RtlSubAuthorityCountSid unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlSubAuthoritySid(void)
{
    size_t len = 34;
    PosWriteFile(1, "RtlSubAuthoritySid unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlUnicodeStringToAnsiString(void)
{
    size_t len = 44;
    PosWriteFile(1, "RtlUnicodeStringToAnsiString unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlUpcaseUnicodeChar(void)
{
    size_t len = 36;
    PosWriteFile(1, "RtlUpcaseUnicodeChar unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlUpcaseUnicodeString(void)
{
    size_t len = 38;
    PosWriteFile(1, "RtlUpcaseUnicodeString unimplemented\r\n", len, &len);
    for (;;) ;
}

/* end of auto-genned functions */
