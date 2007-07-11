/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2007 Victor Kirhenshtein
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
** File: comm.cpp
**
**/

#include "nxmc.h"
#include "logindlg.h"


//
// Login data
//

struct LOGIN_DATA
{
	DWORD flags;
	int objectCacheMode;		// 0 = disabled, 1 = clear current, 2 = use existing
	TCHAR server[MAX_DB_STRING];
	TCHAR login[MAX_DB_STRING];
	TCHAR password[MAX_DB_STRING];
	nxBusyDialog *dlg;
};


//
// Login thread
//

static THREAD_RESULT THREAD_CALL LoginThread(void *arg)
{
	LOGIN_DATA *data = ((LOGIN_DATA *)arg);
	DWORD rcc;

	rcc = NXCConnect(data->flags, data->server, data->login,
	                 data->password, 0, NULL, NULL, &g_hSession,
	                 _T("NetXMS Console/") NETXMS_VERSION_STRING, NULL);

	// Synchronize objects
	if (rcc == RCC_SUCCESS)
	{
      BYTE serverId[8];
		TCHAR serverIdAsText[32];
		wxString cacheFile;

      data->dlg->SetStatusText(_T("Synchronizing objects..."));
      if (data->objectCacheMode != 0)
		{
			NXCGetServerID(g_hSession, serverId);
			BinToStr(serverId, 8, serverIdAsText);
			cacheFile = wxStandardPaths::Get().GetUserDataDir();
			cacheFile += FS_PATH_SEPARATOR;
			cacheFile += _T("cache.");
			cacheFile += serverIdAsText;
			cacheFile += _T(".");
			cacheFile += data->login;
			if (data->objectCacheMode == 1)
				wxRemoveFile(cacheFile);
			wxLogDebug(_T("Using object cache file: %s"), cacheFile.c_str());
		}
      rcc = NXCSubscribe(g_hSession, NXC_CHANNEL_OBJECTS);
      if (rcc == RCC_SUCCESS)
         rcc = NXCSyncObjectsEx(g_hSession, (data->objectCacheMode != 0) ? cacheFile.c_str() : NULL, TRUE);
	}

	data->dlg->ReportCompletion(rcc);
   return THREAD_OK;
}


//
// Do login
//

DWORD DoLogin(nxLoginDialog &dlgLogin)
{
	LOGIN_DATA data;
	nxBusyDialog dlgWait(wxGetApp().GetMainFrame(), _T("Connecting to server..."));

	data.flags = 0;
	if (dlgLogin.m_isMatchVersion)
		data.flags |= NXCF_EXACT_VERSION_MATCH;
	if (dlgLogin.m_isEncrypt)
		data.flags |= NXCF_ENCRYPT;
	if (dlgLogin.m_isCacheDisabled)
	{
		data.objectCacheMode = 0;
	}
	else if (dlgLogin.m_isClearCache)
	{
		data.objectCacheMode = 1;
	}
	else
	{
		data.objectCacheMode = 2;
	}
	nx_strncpy(data.server, dlgLogin.m_server, MAX_DB_STRING);
	nx_strncpy(data.login, dlgLogin.m_login, MAX_DB_STRING);
	nx_strncpy(data.password, dlgLogin.m_password, MAX_DB_STRING);
	data.dlg = &dlgWait;
	wxLogDebug(_T("Attempting to establish connection with server: HOST=%s LOGIN=%s PASSWD=%s FLAGS=%04X"),
	           data.server, data.login, data.password, data.flags);
	ThreadCreate(LoginThread, 0, &data);
	dlgWait.ShowModal();
	return dlgWait.m_rcc;
}
