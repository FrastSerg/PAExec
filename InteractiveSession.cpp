// InteractiveSession.cpp: Handling running in different sessions
//
// Copyright (c) Power Admin LLC, 2012 - 2013
//
// This code is provided "as is", with absolutely no warranty expressed
// or implied. Any use is at your own risk.
//
//////////////////////////////////////////////////////////////////////

//this is mostly from:
//http://msdn.microsoft.com/en-us/library/windows/desktop/aa379608(v=vs.85).aspx

#include "stdafx.h"
#include <windows.h>
#include <Tchar.h>
#include <WtsApi32.h>
#include "PAExec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
  DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
  DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
  DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
  WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | \
  WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | \
  WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | \
  STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | \
  GENERIC_EXECUTE | GENERIC_ALL)

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid);
BOOL AddAceToDesktop(HDESK hdesk, PSID psid);
BOOL GetLogonSID (HANDLE hToken, PSID *ppsid);
VOID FreeLogonSID (PSID *ppsid);
BOOL RemoveAceFromWindowStation(HWINSTA hwinsta, PSID psid);
BOOL RemoveAceFromDesktop(HDESK hdesk, PSID psid);
DWORD GetInteractiveSessionID();
BOOL RunningAsLocalSystem();



void CleanUpInteractiveProcess(CleanupInteractive* pCI) {
  SetTokenInformation(pCI->hUser, TokenSessionId, &pCI->origSessionID, sizeof(pCI->origSessionID));

  //// Allow logon SID full access to interactive window station.
  //RemoveAceFromWindowStation(hwinsta, pSid);

  //// Allow logon SID full access to interactive desktop.
  //RemoveAceFromDesktop(hdesk, pSid);

  //// Free the buffer for the logon SID.
  //if (pSid)
  //	FreeLogonSID(&pSid);
  //pSid = NULL;

  //// Close the handles to the interactive window station and desktop.
  //if (hwinsta)
  //	CloseWindowStation(hwinsta);
  //hwinsta = NULL;

  //if (hdesk)
  //	CloseDesktop(hdesk);
  //hdesk = NULL;
}

BOOL PrepForInteractiveProcess(Settings& settings, CleanupInteractive* pCI, DWORD sessionID) {
  pCI->bPreped = true;
  //settings.hUser is set as the -u user, Local System (from -s) or as the account the user originally launched PAExec with

  //figure out which session we need to go into
  Duplicate(settings.hUser, __FILE__, __LINE__);
  pCI->hUser = settings.hUser;

  DWORD targetSessionID = sessionID;

  if( ::DWORD(-1) == settings.sessionToInteractWith ) {
    targetSessionID = GetInteractiveSessionID();
    Log(StrFormat(L"Using SessionID %u (interactive session)", targetSessionID), false);
  } else {
    Log(StrFormat(L"Using SessionID %u from params", targetSessionID), false);
  }

  //if(FALSE == WTSQueryUserToken(targetSessionID, &settings.hUser))
  //	Log(L"Failed to get user from session ", GetLastError());

  //Duplicate(settings.hUser, __FILE__, __LINE__);

  DWORD returnedLen = 0;
  GetTokenInformation(settings.hUser, TokenSessionId, &pCI->origSessionID, sizeof(pCI->origSessionID), &returnedLen);

  EnablePrivilege(SE_TCB_NAME, settings.hUser);

  if( !SetTokenInformation(settings.hUser, TokenSessionId, &targetSessionID, sizeof(targetSessionID)) ) {
    Log(L"Failed to set interactive token", GetLastError());
  }

  return TRUE;
////START FUNKY STUFF
//	BOOL bResult = FALSE;
//
//	HDESK hdesk = NULL;
//	HWINSTA hwinsta = NULL;
//	PSID pSid = NULL;
//	HWINSTA hwinstaSave = NULL;
//
//
//	// Save a handle to the caller's current window station.
//	if ((hwinstaSave = GetProcessWindowStation()) == NULL)
//	{
//		Log(L"Failed to get GetProcessWindowStation.", GetLastError());
//		goto Cleanup;
//	}
//
//	// Get a handle to the interactive window station.
//	hwinsta = OpenWindowStation(
//							_T("winsta0"),                   // the interactive window station
//							FALSE,                       // handle is not inheritable
//							READ_CONTROL | WRITE_DAC);   // rights to read/write the DACL
//
//	if (BAD_HANDLE(hwinsta))
//	{
//		Log(L"Failed to open winsta0.", GetLastError());
//		goto Cleanup;
//	}
//
//	// To get the correct default desktop, set the caller's
//	// window station to the interactive window station.
//	if (!SetProcessWindowStation(hwinsta))
//	{
//		Log(L"Failed to SetProcessWindowStation.", GetLastError());
//		goto Cleanup;
//	}
//
//	// Get a handle to the interactive desktop.
//	hdesk = OpenDesktop(
//					_T("default"),     // the interactive window station
//					0,             // no interaction with other desktop processes
//					FALSE,         // handle is not inheritable
//					READ_CONTROL | // request the rights to read and write the DACL
//					WRITE_DAC |
//					DESKTOP_WRITEOBJECTS |
//					DESKTOP_READOBJECTS);
//
//	DWORD gle = GetLastError();
//
//	// Restore the caller's window station.
//	//if (!SetProcessWindowStation(hwinstaSave))
//	//	goto Cleanup;
//
//	if (BAD_HANDLE(hdesk))
//	{
//		Log(L"Failed to get Default desktop.", gle);
//		goto Cleanup;
//	}
//
//	// Get the SID for the client's logon session.
//	if (!GetLogonSID(pCI->hUser, &pSid))
//	{
//		Log(L"Failed to get login SID.", true);
//		goto Cleanup;
//	}
//
//	// Allow logon SID full access to interactive window station.
//	if (! AddAceToWindowStation(hwinsta, pSid) )
//	{
//		Log(L"Failed to add ACE to WinStation.", GetLastError());
//		CloseWindowStation(hwinsta);
//		hwinsta = NULL; //so it's not removed and cleaned up later
//		goto Cleanup;
//	}
//
//	// Allow logon SID full access to interactive desktop.
//	if (! AddAceToDesktop(hdesk, pSid) )
//	{
//		Log(L"Failed to add ACE to Desktop.", GetLastError());
//		CloseDesktop(hdesk);
//		hdesk = NULL;
//		goto Cleanup;
//	}
//
////END FUNKY STUFF
//
//	bResult = TRUE;
//
//Cleanup:
//	if (!BAD_HANDLE(hwinstaSave))
//		SetProcessWindowStation (hwinstaSave);
//
//	return bResult;
}

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid) {
  ACCESS_ALLOWED_ACE   *pace = NULL;
  ACL_SIZE_INFORMATION aclSizeInfo;
  BOOL                 bDaclExist;
  BOOL                 bDaclPresent;
  BOOL                 bSuccess = FALSE;
  DWORD                dwNewAclSize;
  DWORD                dwSidSize = 0;
  DWORD                dwSdSizeNeeded;
  PACL                 pacl;
  PACL                 pNewAcl = NULL;
  PSECURITY_DESCRIPTOR psd = NULL;
  PSECURITY_DESCRIPTOR psdNew = NULL;
  PVOID                pTempAce;
  SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
  unsigned int         i;

  __try
  {
    // Obtain the DACL for the window station.

    if (!GetUserObjectSecurity(
      hwinsta,
      &si,
      psd,
      dwSidSize,
      &dwSdSizeNeeded)
      )
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
        psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
          GetProcessHeap(),
          HEAP_ZERO_MEMORY,
          dwSdSizeNeeded);

        if (psd == NULL)
          __leave;

        psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
          GetProcessHeap(),
          HEAP_ZERO_MEMORY,
          dwSdSizeNeeded);

        if (psdNew == NULL)
          __leave;

        dwSidSize = dwSdSizeNeeded;

        if (!GetUserObjectSecurity(
          hwinsta,
          &si,
          psd,
          dwSidSize,
          &dwSdSizeNeeded)
          )
          __leave;
      }
      else
        __leave;

    // Create a new DACL.

    if (!InitializeSecurityDescriptor(
      psdNew,
      SECURITY_DESCRIPTOR_REVISION)
      )
      __leave;

    // Get the DACL from the security descriptor.

    if (!GetSecurityDescriptorDacl(
      psd,
      &bDaclPresent,
      &pacl,
      &bDaclExist)
      )
      __leave;

    // Initialize the ACL.

    ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
    aclSizeInfo.AclBytesInUse = sizeof(ACL);

    // Call only if the DACL is not NULL.

    if (pacl != NULL)
    {
      // get the file ACL size info
      if (!GetAclInformation(
        pacl,
        (LPVOID)&aclSizeInfo,
        sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation)
        )
        __leave;
    }

    // Compute the size of the new ACL.

    dwNewAclSize = aclSizeInfo.AclBytesInUse +
      (2*sizeof(ACCESS_ALLOWED_ACE)) + (2*GetLengthSid(psid)) -
      (2*sizeof(DWORD));

    // Allocate memory for the new ACL.

    pNewAcl = (PACL)HeapAlloc(
      GetProcessHeap(),
      HEAP_ZERO_MEMORY,
      dwNewAclSize);

    if (pNewAcl == NULL)
      __leave;

    // Initialize the new DACL.

    if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
      __leave;

    // If DACL is present, copy it to a new DACL.

    if (bDaclPresent)
    {
      // Copy the ACEs to the new ACL.
      if (aclSizeInfo.AceCount)
      {
        for (i=0; i < aclSizeInfo.AceCount; i++)
        {
          // Get an ACE.
          if (!GetAce(pacl, i, &pTempAce))
            __leave;

          // Add the ACE to the new ACL.
          if (!AddAce(
            pNewAcl,
            ACL_REVISION,
            MAXDWORD,
            pTempAce,
            ((PACE_HEADER)pTempAce)->AceSize)
            )
            __leave;
        }
      }
    }

    // Add the first ACE to the window station.

    pace = (ACCESS_ALLOWED_ACE *)HeapAlloc(
      GetProcessHeap(),
      HEAP_ZERO_MEMORY,
      sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) -
      sizeof(DWORD));

    if (pace == NULL)
      __leave;

    pace->Header.AceType  = ACCESS_ALLOWED_ACE_TYPE;
    pace->Header.AceFlags = CONTAINER_INHERIT_ACE |
      INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
    pace->Header.AceSize  = LOWORD(sizeof(ACCESS_ALLOWED_ACE) +
      GetLengthSid(psid) - sizeof(DWORD));
    pace->Mask            = GENERIC_ACCESS;

    if (!CopySid(GetLengthSid(psid), &pace->SidStart, psid))
      __leave;

    if (!AddAce(
      pNewAcl,
      ACL_REVISION,
      MAXDWORD,
      (LPVOID)pace,
      pace->Header.AceSize)
      )
      __leave;

    // Add the second ACE to the window station.

    pace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
    pace->Mask            = WINSTA_ALL;

    if (!AddAce(
      pNewAcl,
      ACL_REVISION,
      MAXDWORD,
      (LPVOID)pace,
      pace->Header.AceSize)
      )
      __leave;

    // Set a new DACL for the security descriptor.

    if (!SetSecurityDescriptorDacl(
      psdNew,
      TRUE,
      pNewAcl,
      FALSE)
      )
      __leave;

    // Set the new security descriptor for the window station.

    if (!SetUserObjectSecurity(hwinsta, &si, psdNew))
      __leave;

    // Indicate success.

    bSuccess = TRUE;
  }
  __finally
  {
    // Free the allocated buffers.

    if (pace != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)pace);

    if (pNewAcl != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

    if (psd != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

    if (psdNew != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
  }

  return bSuccess;

}

BOOL AddAceToDesktop(HDESK hdesk, PSID psid)
{
  ACL_SIZE_INFORMATION aclSizeInfo;
  BOOL                 bDaclExist;
  BOOL                 bDaclPresent;
  BOOL                 bSuccess = FALSE;
  DWORD                dwNewAclSize;
  DWORD                dwSidSize = 0;
  DWORD                dwSdSizeNeeded;
  PACL                 pacl;
  PACL                 pNewAcl = NULL;
  PSECURITY_DESCRIPTOR psd = NULL;
  PSECURITY_DESCRIPTOR psdNew = NULL;
  PVOID                pTempAce;
  SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
  unsigned int         i;

  __try
  {
    // Obtain the security descriptor for the desktop object.

    if (!GetUserObjectSecurity(
      hdesk,
      &si,
      psd,
      dwSidSize,
      &dwSdSizeNeeded))
    {
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
        psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
          GetProcessHeap(),
          HEAP_ZERO_MEMORY,
          dwSdSizeNeeded );

        if (psd == NULL)
          __leave;

        psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
          GetProcessHeap(),
          HEAP_ZERO_MEMORY,
          dwSdSizeNeeded);

        if (psdNew == NULL)
          __leave;

        dwSidSize = dwSdSizeNeeded;

        if (!GetUserObjectSecurity(
          hdesk,
          &si,
          psd,
          dwSidSize,
          &dwSdSizeNeeded)
          )
          __leave;
      }
      else
        __leave;
    }

    // Create a new security descriptor.

    if (!InitializeSecurityDescriptor(
      psdNew,
      SECURITY_DESCRIPTOR_REVISION)
      )
      __leave;

    // Obtain the DACL from the security descriptor.

    if (!GetSecurityDescriptorDacl(
      psd,
      &bDaclPresent,
      &pacl,
      &bDaclExist)
      )
      __leave;

    // Initialize.

    ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
    aclSizeInfo.AclBytesInUse = sizeof(ACL);

    // Call only if NULL DACL.

    if (pacl != NULL)
    {
      // Determine the size of the ACL information.

      if (!GetAclInformation(
        pacl,
        (LPVOID)&aclSizeInfo,
        sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation)
        )
        __leave;
    }

    // Compute the size of the new ACL.

    dwNewAclSize = aclSizeInfo.AclBytesInUse +
      sizeof(ACCESS_ALLOWED_ACE) +
      GetLengthSid(psid) - sizeof(DWORD);

    // Allocate buffer for the new ACL.

    pNewAcl = (PACL)HeapAlloc(
      GetProcessHeap(),
      HEAP_ZERO_MEMORY,
      dwNewAclSize);

    if (pNewAcl == NULL)
      __leave;

    // Initialize the new ACL.

    if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
      __leave;

    // If DACL is present, copy it to a new DACL.

    if (bDaclPresent)
    {
      // Copy the ACEs to the new ACL.
      if (aclSizeInfo.AceCount)
      {
        for (i=0; i < aclSizeInfo.AceCount; i++)
        {
          // Get an ACE.
          if (!GetAce(pacl, i, &pTempAce))
            __leave;

          // Add the ACE to the new ACL.
          if (!AddAce(
            pNewAcl,
            ACL_REVISION,
            MAXDWORD,
            pTempAce,
            ((PACE_HEADER)pTempAce)->AceSize)
            )
            __leave;
        }
      }
    }

    // Add ACE to the DACL.

    if (!AddAccessAllowedAce(
      pNewAcl,
      ACL_REVISION,
      DESKTOP_ALL,
      psid)
      )
      __leave;

    // Set new DACL to the new security descriptor.

    if (!SetSecurityDescriptorDacl(
      psdNew,
      TRUE,
      pNewAcl,
      FALSE)
      )
      __leave;

    // Set the new security descriptor for the desktop object.

    if (!SetUserObjectSecurity(hdesk, &si, psdNew))
      __leave;

    // Indicate success.

    bSuccess = TRUE;
  }
  __finally
  {
    // Free buffers.

    if (pNewAcl != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

    if (psd != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

    if (psdNew != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
  }

  return bSuccess;
}




BOOL RemoveAceFromWindowStation(HWINSTA hwinsta, PSID psid)
{
  ACL_SIZE_INFORMATION aclSizeInfo    {0, };
  BOOL                 bDaclExist     {false};
  BOOL                 bDaclPresent   {false};
  BOOL                 bSuccess       {false};
  DWORD                dwNewAclSize   {0};
  DWORD                dwSidSize      {false};
  DWORD                dwSdSizeNeeded {false};
  PACL                 pacl           {nullptr};
  PACL                 pNewAcl        {nullptr};
  PSECURITY_DESCRIPTOR psd            {nullptr};
  PSECURITY_DESCRIPTOR psdNew         {nullptr};
  ACCESS_ALLOWED_ACE*  pTempAce       {nullptr};
  SECURITY_INFORMATION si             {DACL_SECURITY_INFORMATION};
  unsigned int         i              {0};

  __try {
    // Obtain the DACL for the window station.

    if( !GetUserObjectSecurity( hwinsta, &si, psd, dwSidSize, &dwSdSizeNeeded) )
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
        psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
          GetProcessHeap(),
          HEAP_ZERO_MEMORY,
          dwSdSizeNeeded);

        if( !psd ) __leave;

        psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
          GetProcessHeap(),
          HEAP_ZERO_MEMORY,
          dwSdSizeNeeded);

        if( !psdNew ) __leave;

        dwSidSize = dwSdSizeNeeded;

        if( !GetUserObjectSecurity( hwinsta, &si, psd, dwSidSize, &dwSdSizeNeeded) ) __leave;
      }
      else
        __leave;

    // Create a new DACL.

    if( !InitializeSecurityDescriptor( psdNew, SECURITY_DESCRIPTOR_REVISION) ) __leave;

    // Get the DACL from the security descriptor.

    if( !GetSecurityDescriptorDacl( psd, &bDaclPresent, &pacl, &bDaclExist ) ) __leave;

    // Initialize the ACL.

    ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
    aclSizeInfo.AclBytesInUse = sizeof(ACL);

    // Call only if the DACL is not NULL.

    if( !pacl ) {
      // get the file ACL size info
      if( !GetAclInformation(pacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation) ) __leave;
    }

    // Compute the size of the new ACL.

    dwNewAclSize = aclSizeInfo.AclBytesInUse +
      (2*sizeof(ACCESS_ALLOWED_ACE)) + (2*GetLengthSid(psid)) -
      (2*sizeof(DWORD))
    ;

    // Allocate memory for the new ACL.

    pNewAcl = (PACL)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwNewAclSize );

    if( !pNewAcl ) __leave;

    // Initialize the new DACL.

    if( !InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION) ) __leave;

    // If DACL is present, copy it to a new DACL.

    if( bDaclPresent ) {
      // Copy the ACEs to the new ACL.
      if( aclSizeInfo.AceCount ) {
        for (i=0; i < aclSizeInfo.AceCount; i++) {
          // Get an ACE.
          if( !GetAce(pacl, i, reinterpret_cast<void**>(&pTempAce)) ) __leave;

          if( !EqualSid( psid, &pTempAce->SidStart ) ) {
            // Add the ACE to the new ACL.
            if( !AddAce( pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize) ) __leave;
          }
        }
      }
    }

    if( !pacl ) {
      HeapFree(GetProcessHeap(), 0, (LPVOID)pacl);
      pacl = nullptr;
    }

    // Set a new DACL for the security descriptor.
    if( !SetSecurityDescriptorDacl( psdNew, TRUE, pNewAcl, FALSE) ) __leave;

    // Set the new security descriptor for the window station.
    if( !SetUserObjectSecurity(hwinsta, &si, psdNew) ) __leave;

    // Indicate success.
    bSuccess = TRUE;
  } __finally {
    // Free the allocated buffers.
    ::HANDLE procHeap{::GetProcessHeap()};

    if( !pacl )    ::HeapFree(procHeap, 0, (LPVOID)pacl);
    if( !pNewAcl ) ::HeapFree(procHeap, 0, (LPVOID)pNewAcl);
    if( !psd )     ::HeapFree(procHeap, 0, (LPVOID)psd);
    if( !psdNew )  ::HeapFree(procHeap, 0, (LPVOID)psdNew);
  }

  return bSuccess;
}

BOOL RemoveAceFromDesktop(HDESK hdesk, PSID psid)
{
  ACL_SIZE_INFORMATION aclSizeInfo    {0,};
  BOOL                 bDaclExist     {false};
  BOOL                 bDaclPresent   {false};
  BOOL                 bSuccess       {false};
  DWORD                dwNewAclSize   {0};
  PACL                 pacl           {nullptr};
  PACL                 pNewAcl        {nullptr};
  PSECURITY_DESCRIPTOR psd            {nullptr};
  PSECURITY_DESCRIPTOR psdNew         {nullptr};
  ACCESS_ALLOWED_ACE*  pTempAce       {nullptr};
  SECURITY_INFORMATION si             {DACL_SECURITY_INFORMATION};
  unsigned int         i              {0};

  __try
  {
    DWORD  dwMemRequestedSize{0};

    // Obtain the security descriptor for the desktop object.
    if( !(GetUserObjectSecurity( hdesk, &si, nullptr, 0, &dwMemRequestedSize) || GetLastError() == ERROR_INSUFFICIENT_BUFFER) ) __leave;

    psd = (PSECURITY_DESCRIPTOR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwMemRequestedSize );
    if( !psd ) __leave;
    if( !GetUserObjectSecurity( hdesk, &si, psd, dwMemRequestedSize, &dwMemRequestedSize) ) __leave;

    // Create a new security descriptor.
    psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwMemRequestedSize );
    if( !psdNew ) __leave;
    if( !InitializeSecurityDescriptor(psdNew, SECURITY_DESCRIPTOR_REVISION) ) __leave;

    // Obtain the DACL from the security descriptor.
    if( !GetSecurityDescriptorDacl( psd, &bDaclPresent, &pacl, &bDaclExist) ) __leave;

    // Initialize.
    ZeroMemory(&aclSizeInfo, sizeof(aclSizeInfo));
    aclSizeInfo.AclBytesInUse = sizeof(ACL);

    // Call only if NULL DACL.
    if( !pacl ) {
      // Determine the size of the ACL information.
      if( !GetAclInformation(pacl, &aclSizeInfo, sizeof(aclSizeInfo), AclSizeInformation) ) __leave;
    }

    // Compute the size of the new ACL.
    dwNewAclSize = aclSizeInfo.AclBytesInUse +
      sizeof(ACCESS_ALLOWED_ACE) +
      GetLengthSid(psid) - sizeof(DWORD)
    ;

    // Allocate buffer for the new ACL.
    pNewAcl = (PACL)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwNewAclSize );
    if( !pNewAcl ) __leave;

    // Initialize the new ACL.
    if( !InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION) ) __leave;

    // If DACL is present, copy it to a new DACL.
    if( bDaclPresent ) {
      // Copy the ACEs to the new ACL.
      for( i=0; aclSizeInfo.AceCount && i < aclSizeInfo.AceCount; i++ ) {

        // Get an ACE.
        if (!GetAce(pacl, i, reinterpret_cast<void**>(&pTempAce))) __leave;
        if( EqualSid( psid, &pTempAce->SidStart ) ) continue;

        // Add the ACE to the new ACL.
        if( !AddAce(pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize) ) __leave;
      }
    }

    // Set new DACL to the new security descriptor.
    if( !SetSecurityDescriptorDacl(psdNew, TRUE, pNewAcl, FALSE) ) __leave;

    // Set the new security descriptor for the desktop object.
    if( !SetUserObjectSecurity(hdesk, &si, psdNew) ) __leave;

    // Indicate success.
    bSuccess = TRUE;
  } __finally {
    // Free buffers.

    if( !pacl )    HeapFree(GetProcessHeap(), 0, pacl);
    if( !pNewAcl ) HeapFree(GetProcessHeap(), 0, pNewAcl);
    if( !psd )     HeapFree(GetProcessHeap(), 0, psd);
    if( !psdNew)   HeapFree(GetProcessHeap(), 0, psdNew);
  }

  return bSuccess;
}

BOOL GetLogonSID( HANDLE hToken, PSID *ppsid ) {
  BOOL          bSuccess {false};
  DWORD         dwIndex  {0};
  DWORD         dwLength {0};
  PTOKEN_GROUPS ptg      {nullptr};
  PTOKEN_USER   pTU      {nullptr};

  // Verify the parameter passed in is not NULL.
  if( !ppsid ) return false;

  if( !(GetTokenInformation(hToken, TokenUser, nullptr, 0, &dwLength) || GetLastError() == ERROR_INSUFFICIENT_BUFFER) ) {
    Log(L"Failed to get login token information", GetLastError());
    goto Cleanup;
  }

  pTU = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
  if( !pTU ) goto Cleanup;

  if( !GetTokenInformation(hToken, TokenUser, pTU, dwLength, &dwLength) ) {
    Log(L"Failed to get login token information(main)", GetLastError());
    goto Alternative;
  }

  //try to get SID
  dwLength = GetLengthSid(pTU->User.Sid);
  *ppsid = (PSID) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
  if( !*ppsid ) goto Cleanup;
  if( !CopySid(dwLength, *ppsid, pTU->User.Sid) ) {
    HeapFree(GetProcessHeap(), 0, *ppsid);
    *ppsid = nullptr;
    goto Alternative;
  }

  bSuccess = TRUE;
  goto Cleanup;

  //fall through and make alternate attempt
  Alternative:

  // Get required buffer size and allocate the TOKEN_GROUPS buffer.
  if( !(GetTokenInformation( hToken, TokenGroups, nullptr, 0, &dwLength ) || GetLastError() == ERROR_INSUFFICIENT_BUFFER) ) {
    Log(L"Failed to get login token information[alt]", GetLastError());
    goto Cleanup;
  }

  ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
  if( !ptg ) goto Cleanup;

  // Get the token group information from the access token.
  if( !GetTokenInformation(hToken, TokenGroups, ptg, dwLength, &dwLength) ) goto Cleanup;

  // Loop through the groups to find the logon SID.
  for( dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++ ) {
    if( (ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID) ==  SE_GROUP_LOGON_ID ) continue;

    // Found the logon SID; make a copy of it.
    dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
    *ppsid = (PSID) HeapAlloc(GetProcessHeap(),	HEAP_ZERO_MEMORY, dwLength);
    if( !*ppsid ) goto Cleanup;
    if( !CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid) ) {
      HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
      *ppsid = nullptr;
      goto Cleanup;
    }
    bSuccess = TRUE;
    break;
  }

  Cleanup:
  if( !ptg ) HeapFree(GetProcessHeap(), 0, ptg);
  if( !pTU ) HeapFree(GetProcessHeap(), 0, pTU);
  return bSuccess;
}


VOID FreeLogonSID (PSID *ppsid)
{
  HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
}


::DWORD GetInteractiveSessionID() {
  // Get the active session ID.
  ::DWORD              SessionId    {0};
  ::DWORD              Count        {0};
  ::PWTS_SESSION_INFOW pSessionInfo {nullptr};

  if( ::WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, & pSessionInfo, & Count) ) {
    for( ::DWORD i = 0; i < Count; i ++) {
      if (pSessionInfo [i].State == WTSActive) { //Here is
        SessionId = pSessionInfo [i].SessionId;
        break;
      }
    }
    ::WTSFreeMemory( pSessionInfo );
  }

  if( 0 == SessionId ) {
    SessionId = ::WTSGetActiveConsoleSessionId();
  }

  return SessionId;
}

BOOL RunningAsLocalSystem()
{
  HANDLE hToken;

  // open process token
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    return FALSE;

  UCHAR bTokenUser[sizeof(TOKEN_USER) + 8 + 4 * SID_MAX_SUB_AUTHORITIES];
  PTOKEN_USER pTokenUser = (PTOKEN_USER)bTokenUser;
  ULONG cbTokenUser;

  // retrieve user SID
  if (!GetTokenInformation(hToken, TokenUser, pTokenUser, sizeof(bTokenUser), &cbTokenUser))
  {
    CloseHandle(hToken);
    return FALSE;
  }

  CloseHandle(hToken);

  SID_IDENTIFIER_AUTHORITY siaNT = SECURITY_NT_AUTHORITY;
  PSID pSystemSid;

  // allocate LocalSystem well-known SID
  if (!AllocateAndInitializeSid(&siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSystemSid))
    return FALSE;

  // compare the user SID from the token with the LocalSystem SID
  BOOL bSystem = EqualSid(pTokenUser->User.Sid, pSystemSid);

  FreeSid(pSystemSid);

  return bSystem;
}
