/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: nxconfig.h
**
**/

#ifndef _nxconfig_h_
#define _nxconfig_h_

#include <nms_common.h>
#include <nms_util.h>


//
// Config entry
//

class ConfigEntryList;

class LIBNETXMS_EXPORTABLE ConfigEntry
{
private:
	TCHAR *m_name;
	ConfigEntry *m_parent;
	ConfigEntry *m_next;
	ConfigEntry *m_childs;
	int m_valueCount;
	TCHAR **m_values;
	TCHAR *m_file;
	int m_line;
	int m_id;

	void linkEntry(ConfigEntry *entry) { entry->m_next = m_next; m_next = entry; }
	void addEntry(ConfigEntry *entry) { entry->m_parent = this; entry->m_next = m_childs; m_childs = entry; }

public:
	ConfigEntry(const TCHAR *name, ConfigEntry *parent, const TCHAR *file, int line, int id);
	~ConfigEntry();

	ConfigEntry *getNext() { return m_next; }

	const TCHAR *getName() { return m_name; }
	int getId() { return m_id; }
	int getValueCount() { return m_valueCount; }
	int getConcatenatedValuesLength();
	
	const TCHAR *getValue(int index = 0);
	LONG getValueInt(int index = 0, LONG defaultValue = 0);
	DWORD getValueUInt(int index = 0, DWORD defaultValue = 0);
	INT64 getValueInt64(int index = 0, INT64 defaultValue = 0);
	QWORD getValueUInt64(int index = 0, QWORD defaultValue = 0);
	bool getValueBoolean(int index = 0, bool defaultValue = false);

	const TCHAR *getSubEntryValue(const TCHAR *name, int index = 0, const TCHAR *defaultValue = NULL);
	LONG getSubEntryValueInt(const TCHAR *name, int index = 0, LONG defaultValue = 0);
	DWORD getSubEntryValueUInt(const TCHAR *name, int index = 0, DWORD defaultValue = 0);
	INT64 getSubEntryValueInt64(const TCHAR *name, int index = 0, INT64 defaultValue = 0);
	QWORD getSubEntryValueUInt64(const TCHAR *name, int index = 0, QWORD defaultValue = 0);
	bool getSubEntryValueBoolean(const TCHAR *name, int index = 0, bool defaultValue = false);

	const TCHAR *getFile() { return m_file; }
	int getLine() { return m_line; }

	void addValue(const TCHAR *value);
	void setValue(const TCHAR*value);

	ConfigEntry *findEntry(const TCHAR *name);
	ConfigEntryList *getSubEntries(const TCHAR *mask);
	ConfigEntryList *getOrderedSubEntries(const TCHAR *mask);

	void print(FILE *file, int level);
};


//
// List of config entries
//

class ConfigEntryList
{
private:
	ConfigEntry **m_list;
	int m_size;

public:
	ConfigEntryList(ConfigEntry **list, int size) { m_list = list; m_size = size; }
	~ConfigEntryList() { safe_free(m_list); }

	int getSize() { return m_size; }
	ConfigEntry *getEntry(int index) { return ((index >= 0) && (index < m_size)) ? m_list[index] : NULL; }

	void sortById();
};


//
// Config class
//

class LIBNETXMS_EXPORTABLE Config
{
private:
	ConfigEntry *m_root;
	int m_errorCount;

protected:
	virtual void onError(const TCHAR *errorMessage);
	
	void error(const TCHAR *format, ...);

public:
	Config();
	~Config();

	bool loadXmlConfig(const TCHAR *file, const char *topLevelTag = NULL);
	bool loadXmlConfigFromMemory(const char *xml, int xmlSize, const TCHAR *name = NULL, const char *topLevelTag = NULL);
	bool loadIniConfig(const TCHAR *file, const TCHAR *defaultIniSection);
	bool loadConfig(const TCHAR *file, const TCHAR *defaultIniSection);

	bool loadConfigDirectory(const TCHAR *path, const TCHAR *defaultIniSection);

	ConfigEntry *getEntry(const TCHAR *path);
	const TCHAR *getValue(const TCHAR *path, const TCHAR *defaultValue = NULL);
	LONG getValueInt(const TCHAR *path, LONG defaultValue);
	DWORD getValueUInt(const TCHAR *path, DWORD defaultValue);
	INT64 getValueInt64(const TCHAR *path, INT64 defaultValue);
	QWORD getValueUInt64(const TCHAR *path, QWORD defaultValue);
	bool getValueBoolean(const TCHAR *path, bool defaultValue);
	ConfigEntryList *getSubEntries(const TCHAR *path, const TCHAR *mask);
	ConfigEntryList *getOrderedSubEntries(const TCHAR *path, const TCHAR *mask);

	bool parseTemplate(const TCHAR *section, NX_CFG_TEMPLATE *cfgTemplate);

	int getErrorCount() { return m_errorCount; }

	void print(FILE *file);
};


#endif
