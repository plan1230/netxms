/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: package.cpp
**
**/

#include "libnxcl.h"


//
// Lock/unlock package database
//

static DWORD LockPackageDB(NXCL_Session *pSession, BOOL bLock)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = pSession->CreateRqId();

   msg.SetCode(bLock ? CMD_LOCK_PACKAGE_DB : CMD_UNLOCK_PACKAGE_DB);
   msg.SetId(dwRqId);
   pSession->SendMsg(&msg);

   return pSession->WaitForRCC(dwRqId);
}


//
// Client interface: lock package database
//

DWORD LIBNXCL_EXPORTABLE NXCLockPackageDB(NXC_SESSION hSession)
{
   return LockPackageDB((NXCL_Session *)hSession, TRUE);
}


//
// Client interface: unlock package database
//

DWORD LIBNXCL_EXPORTABLE NXCUnlockPackageDB(NXC_SESSION hSession)
{
   return LockPackageDB((NXCL_Session *)hSession, FALSE);
}


//
// Retrieve package list from server
//

DWORD LIBNXCL_EXPORTABLE NXCGetPackageList(NXC_SESSION hSession, DWORD *pdwNumPackages, 
                                           NXC_PACKAGE_INFO **ppList)
{
   CSCPMessage msg, *pResponce;
   DWORD dwResult, dwRqId, dwPkgId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_PACKAGE_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   *pdwNumPackages = 0;
   *ppList = NULL;
   if (dwResult == RCC_SUCCESS)
   {
      *pdwNumPackages = 0;
      do
      {
         pResponce = ((NXCL_Session *)hSession)->WaitForMessage(CMD_PACKAGE_INFO, dwRqId);
         if (pResponce != NULL)
         {
            dwPkgId = pResponce->GetVariableLong(VID_PACKAGE_ID);
            if (dwPkgId != 0)
            {
               *ppList = (NXC_PACKAGE_INFO *)realloc(*ppList, sizeof(NXC_PACKAGE_INFO) * (*pdwNumPackages + 1));
               (*ppList)[*pdwNumPackages].dwId = dwPkgId;
               pResponce->GetVariableStr(VID_FILE_NAME, 
                                         (*ppList)[*pdwNumPackages].szFileName, 
                                         MAX_DB_STRING);
               pResponce->GetVariableStr(VID_PLATFORM_NAME, 
                                         (*ppList)[*pdwNumPackages].szPlatform, 
                                         MAX_PLATFORM_NAME_LEN);
               pResponce->GetVariableStr(VID_PACKAGE_VERSION, 
                                         (*ppList)[*pdwNumPackages].szVersion, 
                                         MAX_AGENT_VERSION_LEN);
               (*pdwNumPackages)++;
            }
            delete pResponce;
         }
         else
         {
            dwResult = RCC_TIMEOUT;
            *pdwNumPackages = 0;
            safe_free(*ppList);
            *ppList = NULL;
            break;
         }
      } while(dwPkgId != 0);
   }

   return dwResult;
}


//
// Remove package from server
//

DWORD LIBNXCL_EXPORTABLE NXCRemovePackage(NXC_SESSION hSession, DWORD dwPkgId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_PACKAGE_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PACKAGE_ID, dwPkgId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Install package to server
//

DWORD LIBNXCL_EXPORTABLE NXCInstallPackage(NXC_SESSION hSession, TCHAR *pszPkgFile, DWORD *pdwPkgId)
{
   return 0;
}


//
// Parse NPI file
//

DWORD LIBNXCL_EXPORTABLE NXCParseNPIFile(TCHAR *pszInfoFile, NXC_PACKAGE_INFO *pInfo)
{
   FILE *fp;
   DWORD dwResult = RCC_SUCCESS;
   TCHAR szBuffer[256], *ptr;

   fp = fopen(pszInfoFile, "r");
   if (fp != NULL)
   {
      while(1)
      {
         fgets(szBuffer, 256, fp);
         if (feof(fp))
            break;
         ptr = _tcschr(szBuffer, _T('\n'));
         if (ptr != NULL)
            *ptr = 0;
         StrStrip(szBuffer);
         if ((szBuffer[0] == _T('#')) || (szBuffer[0] == 0))
            continue;   // Empty line or comment
      }
      fclose(fp);
   }
   else
   {
      dwResult = RCC_IO_ERROR;
   }

   return dwResult;
}
