/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: nxmodule.h
**
**/

#ifndef _nxmodule_h_
#define _nxmodule_h_


//
// Forward declaration of server classes
//

class ClientSession;
class Node;
class Event;


//
// Module registration structure
//

typedef struct
{
   DWORD dwSize;           // Size of structure in bytes
   TCHAR szName[MAX_OBJECT_NAME];
   void (* pfMain)(void);  // Pointer to module's main()
   BOOL (* pfCommandHandler)(DWORD dwCommand, CSCPMessage *pMsg, ClientSession *pSession);
   BOOL (* pfTrapHandler)(SNMP_PDU *pdu, Node *pNode);
   BOOL (* pfEventHandler)(Event *event);
   HMODULE hModule;
} NXMODULE;


//
// Functions
//

void LoadNetXMSModules(void);


//
// Global variables
//

extern DWORD g_dwNumModules;
extern NXMODULE *g_pModuleList;


#endif   /* _nxmodule_h_ */
