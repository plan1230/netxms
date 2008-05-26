/* 
** HP Data Protector integration module
** Copyright (C) 2008 Victor Kirhenshtein
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
** File: hpdp.cpp
**
**/

#include "hpdp.h"


//
// SNMP trap handler
//

static BOOL TrapHandler(SNMP_PDU *pdu, Node *pNode)
{
	if (_tcscmp(pdu->GetTrapId()->GetValueAsText(), _T(".1.3.6.1.4.1.??????")))
		return FALSE;

	return TRUE;
}


//
// Module entry point
//

extern "C" BOOL EXPORT NetXMSModuleInit(NXMODULE *module)
{
	module->dwSize = sizeof(NXMODULE);
	_tcscpy(module->szName, _T("HPDP"));
	module->pfTrapHandler = TrapHandler;
	return TRUE;
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
