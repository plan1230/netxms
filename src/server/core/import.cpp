/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: import.cpp
**
**/

#include "nxcore.h"
#include "nxcore_websvc.h"

#define DEBUG_TAG _T("import")

/**
 * Check if given event exist either in server configuration or in configuration being imported
 */
static bool IsEventExist(const TCHAR *name, Config *config)
{
   shared_ptr<EventTemplate> e = FindEventTemplateByName(name);
	if (e != nullptr)
		return true;

	ConfigEntry *eventsRoot = config->getEntry(_T("/events"));
	if (eventsRoot != nullptr)
	{
		ObjectArray<ConfigEntry> *events = eventsRoot->getSubEntries(_T("event#*"));
		for(int i = 0; i < events->size(); i++)
		{
			ConfigEntry *event = events->get(i);
			if (!_tcsicmp(event->getSubEntryValue(_T("name"), 0, _T("<unnamed>")), name))
         {
            delete events;
				return true;
         }
		}
      delete events;
	}

	return false;
}

/**
 * Validate DCI from template
 */
static bool ValidateDci(Config *config, ConfigEntry *dci, const TCHAR *templateName, TCHAR *errorText, int errorTextLen)
{
	ConfigEntry *thresholdsRoot = dci->findEntry(_T("thresholds"));
	if (thresholdsRoot == nullptr)
		return true;

	bool success = true;
	ObjectArray<ConfigEntry> *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
	for(int i = 0; i < thresholds->size(); i++)
	{
		ConfigEntry *threshold = thresholds->get(i);
		if (!IsEventExist(threshold->getSubEntryValue(_T("activationEvent")), config))
		{
			_sntprintf(errorText, errorTextLen,
						  _T("Template \"%s\" DCI \"%s\" threshold %d attribute \"activationEvent\" refers to unknown event"),
						  templateName, dci->getSubEntryValue(_T("description"), 0, _T("<unnamed>")), i + 1);
			success = false;
			break;
		}
		if (!IsEventExist(threshold->getSubEntryValue(_T("deactivationEvent")), config))
		{
			_sntprintf(errorText, errorTextLen,
						  _T("Template \"%s\" DCI \"%s\" threshold %d attribute \"deactivationEvent\" refers to unknown event"),
						  templateName, dci->getSubEntryValue(_T("description"), 0, _T("<unnamed>")), i + 1);
			success = false;
			break;
		}
	}
	delete thresholds;
	return success;
}

/**
 * Validate template
 */
static bool ValidateTemplate(Config *config, ConfigEntry *root, TCHAR *errorText, int errorTextLen)
{
	nxlog_debug_tag(DEBUG_TAG, 6, _T("ValidateConfig(): validating template \"%s\""), root->getSubEntryValue(_T("name"), 0, _T("<unnamed>")));

	ConfigEntry *dcRoot = root->findEntry(_T("dataCollection"));
	if (dcRoot == nullptr)
		return true;

	bool success = true;
	const TCHAR *name = root->getSubEntryValue(_T("name"), 0, _T("<unnamed>"));

	ObjectArray<ConfigEntry> *dcis = dcRoot->getSubEntries(_T("dci#*"));
	for(int i = 0; i < dcis->size(); i++)
	{
		if (!ValidateDci(config, dcis->get(i), name, errorText, errorTextLen))
		{
			success = false;
			break;
		}
	}
	delete dcis;

   if (success)
   {
	   ObjectArray<ConfigEntry> *dctables = dcRoot->getSubEntries(_T("dctable#*"));
	   for(int i = 0; i < dctables->size(); i++)
	   {
		   if (!ValidateDci(config, dctables->get(i), name, errorText, errorTextLen))
		   {
			   success = false;
			   break;
		   }
	   }
	   delete dctables;
   }

   return success;
}

/**
 * Validate configuration before import
 */
bool ValidateConfig(Config *config, UINT32 flags, TCHAR *errorText, int errorTextLen)
{
   int i;
	ObjectArray<ConfigEntry> *events = nullptr, *traps = nullptr, *templates = nullptr;
	ConfigEntry *eventsRoot, *trapsRoot, *templatesRoot;
   bool success = false;

	nxlog_debug_tag(DEBUG_TAG, 4, _T("ValidateConfig() called, flags = 0x%04X"), flags);

   // Validate events
	eventsRoot = config->getEntry(_T("/events"));
	if (eventsRoot != nullptr)
	{
		events = eventsRoot->getSubEntries(_T("event#*"));
		for(i = 0; i < events->size(); i++)
		{
			ConfigEntry *event = events->get(i);
			nxlog_debug_tag(DEBUG_TAG, 6, _T("ValidateConfig(): validating event %s"), event->getSubEntryValue(_T("name"), 0, _T("<unnamed>")));

			UINT32 code = event->getSubEntryValueAsUInt(_T("code"));
			if ((code >= FIRST_USER_EVENT_ID) || (code == 0))
			{
				ConfigEntry *e = event->findEntry(_T("name"));
				if (e == nullptr)
				{
					_sntprintf(errorText, errorTextLen, _T("Mandatory attribute \"name\" missing for entry %s"), event->getName());
					goto stop_processing;
				}
			}
		}
	}

	// Validate traps
	trapsRoot = config->getEntry(_T("/traps"));
	if (trapsRoot != nullptr)
	{
		traps = trapsRoot->getSubEntries(_T("trap#*"));
		for(i = 0; i < traps->size(); i++)
		{
			ConfigEntry *trap = traps->get(i);
			nxlog_debug_tag(DEBUG_TAG, 6, _T("ValidateConfig(): validating trap \"%s\""), trap->getSubEntryValue(_T("description"), 0, _T("<unnamed>")));
			if (!IsEventExist(trap->getSubEntryValue(_T("event")), config))
			{
				_sntprintf(errorText, errorTextLen, _T("Trap \"%s\" references unknown event"), trap->getSubEntryValue(_T("description"), 0, _T("")));
				goto stop_processing;
			}
		}
	}

	// Validate templates
	templatesRoot = config->getEntry(_T("/templates"));
	if (templatesRoot != nullptr)
	{
		templates = templatesRoot->getSubEntries(_T("template#*"));
		for(i = 0; i < templates->size(); i++)
		{
			if (!ValidateTemplate(config, templates->get(i), errorText, errorTextLen))
				goto stop_processing;
		}
	}

   success = true;

stop_processing:
	delete events;
	delete traps;
	delete templates;

	nxlog_debug_tag(DEBUG_TAG, 4, _T("ValidateConfig() finished, status = %d"), success);
	if (!success)
		nxlog_debug_tag(DEBUG_TAG, 4, _T("ValidateConfig(): %s"), errorText);
   return success;
}

/**
 * Import event
 */
static UINT32 ImportEvent(ConfigEntry *event, bool overwrite)
{
	const TCHAR *name = event->getSubEntryValue(_T("name"));
	if (name == nullptr)
		return RCC_INTERNAL_ERROR;

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	UINT32 code = 0;
	uuid guid = event->getSubEntryValueAsUUID(_T("guid"));
	if (!guid.isNull())
	{
	   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT event_code FROM event_cfg WHERE guid=?"));
	   if (hStmt == nullptr)
	   {
	      DBConnectionPoolReleaseConnection(hdb);
	      return RCC_DB_FAILURE;
	   }
	   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
	   DB_RESULT hResult = DBSelectPrepared(hStmt);
	   if (hResult != nullptr)
	   {
	      code = DBGetFieldULong(hResult, 0, 0);
	      DBFreeResult(hResult);
	   }
	   DBFreeStatement(hStmt);
	   if (code != 0)
	   {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportEvent: found existing event with GUID %s (code=%d)"), (const TCHAR *)guid.toString(), code);
	   }
	   else
	   {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportEvent: event with GUID %s not found"), (const TCHAR *)guid.toString());
	   }
	}
	else
	{
	   code = event->getSubEntryValueAsUInt(_T("code"), 0, 0);
	   if (code >= FIRST_USER_EVENT_ID)
	   {
	      code = 0;
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportEvent: event without GUID and code not in system range"));
	   }
	   else
	   {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportEvent: using provided event code %d"), code);
	   }
	}

	// Create or update event template in database
   const TCHAR *msg = event->getSubEntryValue(_T("message"), 0, name);
   const TCHAR *descr = event->getSubEntryValue(_T("description"));
   const TCHAR *tags = event->getSubEntryValue(_T("tags"));
	TCHAR query[8192];
   if ((code != 0) && IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_code"), code))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportEvent: found existing event with code %d (%s)"), code, overwrite ? _T("updating") : _T("skipping"));
      if (overwrite)
      {
         _sntprintf(query, 8192, _T("UPDATE event_cfg SET event_name=%s,severity=%d,flags=%d,message=%s,description=%s,tags=%s WHERE event_code=%d"),
                    (const TCHAR *)DBPrepareString(hdb, name), event->getSubEntryValueAsInt(_T("severity")),
                    event->getSubEntryValueAsInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
                    (const TCHAR *)DBPrepareString(hdb, descr), (const TCHAR *)DBPrepareString(hdb, tags), code);
      }
      else
      {
         query[0] = 0;
      }
   }
   else if (IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_name"), name))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportEvent: found existing event with name %s (%s)"), name, overwrite ? _T("updating") : _T("skipping"));
      if (overwrite)
      {
         _sntprintf(query, 8192, _T("UPDATE event_cfg SET severity=%d,flags=%d,message=%s,description=%s,tags=%s WHERE event_name=%s"),
                    event->getSubEntryValueAsInt(_T("severity")),
                    event->getSubEntryValueAsInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
                    (const TCHAR *)DBPrepareString(hdb, descr), (const TCHAR *)DBPrepareString(hdb, tags), (const TCHAR *)DBPrepareString(hdb, name));
      }
      else
      {
         query[0] = 0;
      }
   }
   else
   {
      if (guid.isNull())
         guid = uuid::generate();
      if (code == 0)
         code = CreateUniqueId(IDG_EVENT);
      _sntprintf(query, 8192, _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,")
                              _T("message,description,guid,tags) VALUES (%d,%s,%d,%d,%s,%s,'%s',%s)"),
                 code, (const TCHAR *)DBPrepareString(hdb, name), event->getSubEntryValueAsInt(_T("severity")),
					  event->getSubEntryValueAsInt(_T("flags")), (const TCHAR *)DBPrepareString(hdb, msg),
					  (const TCHAR *)DBPrepareString(hdb, descr), (const TCHAR *)guid.toString(), (const TCHAR *)DBPrepareString(hdb, tags));
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportEvent: added new event: code=%d, name=%s, guid=%s"), code, name, (const TCHAR *)guid.toString());
   }
	UINT32 rcc = (query[0] != 0) ? (DBQuery(hdb, query) ? RCC_SUCCESS : RCC_DB_FAILURE) : RCC_SUCCESS;

	DBConnectionPoolReleaseConnection(hdb);
	return rcc;
}

/**
 * Import SNMP trap configuration
 */
static UINT32 ImportTrap(ConfigEntry *trap, bool overwrite) // TODO transactions needed?
{
   UINT32 rcc = RCC_INTERNAL_ERROR;
   shared_ptr<EventTemplate> eventTemplate = FindEventTemplateByName(trap->getSubEntryValue(_T("event"), 0, _T("")));
	if (eventTemplate == nullptr)
		return rcc;

	uuid guid = trap->getSubEntryValueAsUUID(_T("guid"));
   if (guid.isNull())
   {
      guid = uuid::generate();
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportTrap: GUID not found in config, generated GUID %s"), (const TCHAR *)guid.toString());
   }
   UINT32 id = ResolveTrapGuid(guid);
   if ((id != 0) && !overwrite)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportTrap: skipping existing entry with GUID %s"), (const TCHAR *)guid.toString());
      return RCC_SUCCESS;
   }

	SNMPTrapConfiguration *trapCfg = new SNMPTrapConfiguration(trap, guid, id, eventTemplate->getCode());
	if (!trapCfg->getOid().isValid())
	{
	   delete trapCfg;
		return rcc;
	}

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = (id == 0) ?
	   DBPrepare(hdb, _T("INSERT INTO snmp_trap_cfg (snmp_oid,event_code,description,user_tag,transformation_script,trap_id,guid) VALUES (?,?,?,?,?,?,?)")) :
	   DBPrepare(hdb, _T("UPDATE snmp_trap_cfg SET snmp_oid=?,event_code=?,description=?,user_tag=?,transformation_script=? WHERE trap_id=?"));

	if (hStmt != nullptr)
	{
	   TCHAR oid[1024];
	   trapCfg->getOid().toString(oid, 1024);
	   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, oid, DB_BIND_STATIC);
	   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, trapCfg->getEventCode());
	   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, trapCfg->getDescription(), DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, trapCfg->getEventTag(), DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_TEXT, trapCfg->getScriptSource(), DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, trapCfg->getId());
      if (id == 0)
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, trapCfg->getGuid());

      if (DBBegin(hdb))
      {
         if (DBExecute(hStmt) && trapCfg->saveParameterMapping(hdb))
         {
            AddTrapCfgToList(trapCfg);
            trapCfg->notifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_CREATED);
            rcc = RCC_SUCCESS;
            DBCommit(hdb);
         }
         else
         {
            DBRollback(hdb);
            rcc = RCC_DB_FAILURE;
         }
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBFreeStatement(hStmt);
	}
	else
	{
	   rcc = RCC_DB_FAILURE;
	}

	if (rcc != RCC_SUCCESS)
	   delete trapCfg;

	DBConnectionPoolReleaseConnection(hdb);

	return rcc;
}

/**
 * Find (and create as necessary) parent object for imported template
 */
static shared_ptr<NetObj> FindTemplateRoot(ConfigEntry *config)
{
	ConfigEntry *pathRoot = config->findEntry(_T("path"));
	if (pathRoot == nullptr)
      return g_templateRoot;  // path not specified in config

   shared_ptr<NetObj> parent = g_templateRoot;
	ObjectArray<ConfigEntry> *path = pathRoot->getSubEntries(_T("element#*"));
	for(int i = 0; i < path->size(); i++)
	{
      const TCHAR *name = path->get(i)->getValue();
      shared_ptr<NetObj> o = parent->findChildObject(name, OBJECT_TEMPLATEGROUP);
      if (o == nullptr)
      {
         o = MakeSharedNObject<TemplateGroup>(name);
         NetObjInsert(o, true, false);
         o->addParent(parent);
         parent->addChild(o);
         o->unhide();
         o->calculateCompoundStatus();	// Force status change to NORMAL
      }
      parent = o;
   }
   delete path;
   return parent;
}

/**
 * Fill rule ordering array
 */
static ObjectArray<uuid> *GetRuleOrdering(ConfigEntry *ruleOrdering)
{
   ObjectArray<uuid> *ordering = nullptr;
   if(ruleOrdering != nullptr)
   {
      ObjectArray<ConfigEntry> *rules = ruleOrdering->getOrderedSubEntries(_T("rule#*"));
      if (rules->size() > 0)
      {
         ordering = new ObjectArray<uuid>(0, 16, Ownership::True);
         for(int i = 0; i < rules->size(); i++)
         {
            ordering->add(new uuid(uuid::parse(rules->get(i)->getValue())));
         }
      }
      delete rules;
   }

   return ordering;
}

/**
 * Delete template group if it is empty
 */
static void DeleteEmptyTemplateGroup(shared_ptr<NetObj> templateGroup)
{
   if (templateGroup->getChildCount() != 0)
      return;

   SharedObjectArray<NetObj> *parents = templateGroup->getParents(OBJECT_TEMPLATEGROUP);
   shared_ptr<NetObj> parent = parents->getShared(0);
   delete parents;

   ObjectTransactionStart();
   templateGroup->deleteObject();
   ObjectTransactionEnd();

   if (parent != nullptr)
      DeleteEmptyTemplateGroup(parent);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): template group %s [%d] with GUID %s deleted as there was empty"), templateGroup->getName(), templateGroup->getId());
}

/**
 * Import configuration
 */
UINT32 ImportConfig(Config *config, UINT32 flags)
{
	ObjectArray<ConfigEntry> *events = nullptr, *traps = nullptr, *templates = nullptr, *rules = nullptr,
	                         *scripts = nullptr, *objectTools = nullptr, *summaryTables = nullptr,
	                         *webServiceDef = nullptr, *actions = nullptr;
	ConfigEntry *eventsRoot, *trapsRoot, *templatesRoot, *rulesRoot,
	            *scriptsRoot, *objectToolsRoot, *summaryTablesRoot, *webServiceDefRoot, *actionsRoot;
	UINT32 rcc = RCC_SUCCESS;
	int i;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportConfig() called, flags=0x%04X"), flags);

   // Import events
	eventsRoot = config->getEntry(_T("/events"));
	if (eventsRoot != nullptr)
	{
		events = eventsRoot->getSubEntries(_T("event#*"));
		nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): %d events to import"), events->size());
		for(i = 0; i < events->size(); i++)
		{
			rcc = ImportEvent(events->get(i), (flags & CFG_IMPORT_REPLACE_EVENTS) != 0);
			if (rcc != RCC_SUCCESS)
				goto stop_processing;
		}

		if (events->size() > 0)
		{
			ReloadEvents();
			NotifyClientSessions(NX_NOTIFY_RELOAD_EVENT_DB, 0);
		}
		nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): events imported"));
	}

	// Import traps
	trapsRoot = config->getEntry(_T("/traps"));
	if (trapsRoot != nullptr)
	{
		traps = trapsRoot->getSubEntries(_T("trap#*"));
		nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): %d SNMP traps to import"), traps->size());
		for(i = 0; i < traps->size(); i++)
		{
			rcc = ImportTrap(traps->get(i), (flags & CFG_IMPORT_REPLACE_TRAPS) != 0);
			if (rcc != RCC_SUCCESS)
				goto stop_processing;
		}
		nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): SNMP traps imported"));
	}

	// Import templates
	templatesRoot = config->getEntry(_T("/templates"));
	if (templatesRoot != nullptr)
	{
		templates = templatesRoot->getSubEntries(_T("template#*"));
		for(i = 0; i < templates->size(); i++)
		{
		   ConfigEntry *tc = templates->get(i);
		   uuid guid = tc->getSubEntryValueAsUUID(_T("guid"));
		   shared_ptr<Template> object = static_pointer_cast<Template>(FindObjectByGUID(guid, OBJECT_TEMPLATE));
		   if (object != nullptr)
		   {
		      if (flags & CFG_IMPORT_REPLACE_TEMPLATES)
		      {
	            nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): updating existing template %s [%d] with GUID %s"), object->getName(), object->getId(), (const TCHAR *)guid.toString());
		         object->updateFromImport(tc);
		      }
		      else
		      {
	            nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): existing template %s [%d] with GUID %s skipped"), object->getName(), object->getId(), (const TCHAR *)guid.toString());
		      }

            if (flags & CFG_IMPORT_REPLACE_TEMPLATE_NAMES_LOCATIONS)
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): existing template %s [%d] with GUID %s renamed to %s"), object->getName(), object->getId(), (const TCHAR *)guid.toString(), tc->getSubEntryValue(_T("name")));

               object->setName(tc->getSubEntryValue(_T("name")));
               shared_ptr<NetObj> parent = FindTemplateRoot(tc);
               if (!parent->isChild(object->getId()))
               {
                  nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): existing template %s [%d] with GUID %s moved to %s template group"), object->getName(), object->getId(), (const TCHAR *)guid.toString(), parent->getName());
                  ObjectTransactionStart();
                  SharedObjectArray<NetObj> *parents = object->getParents(OBJECT_TEMPLATEGROUP);
                  if (parents->size() > 0)
                  {
                     shared_ptr<NetObj> parent = parents->getShared(0);
                     parent->deleteChild(*object);
                     object->deleteParent(*parent);

                     if (flags & CFG_IMPORT_DELETE_EMPTY_TEMPLATE_GROUPS)
                     {
                        DeleteEmptyTemplateGroup(parent);
                     }
                  }
                  delete parents;
                  object->addParent(parent);
                  parent->addChild(object);
                  ObjectTransactionEnd();
               }
            }
		   }
		   else
		   {
            ObjectTransactionStart();
            nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): template with GUID %s not found"), (const TCHAR *)guid.toString());
            shared_ptr<NetObj> parent = FindTemplateRoot(tc);
            object = MakeSharedNObject<Template>(tc->getSubEntryValue(_T("name"), 0, _T("Unnamed Object")), tc->getSubEntryValueAsUUID(_T("guid")));
            object->updateFromImport(tc);
            NetObjInsert(object, true, true);
            object->addParent(parent);
            parent->addChild(object);
            object->unhide();
            ObjectTransactionEnd();
		   }
		}
		nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): templates imported"));
	}

   // Import actions
   actionsRoot = config->getEntry(_T("/actions"));
   if (actionsRoot != nullptr)
   {
      actions = actionsRoot->getSubEntries(_T("action#*"));
      for(i = 0; i < actions->size(); i++)
      {
         ImportAction(actions->get(i), (flags & CFG_IMPORT_REPLACE_ACTIONS) != 0);
      }
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): Actions imported"));
   }

	// Import rules
	rulesRoot = config->getEntry(_T("/rules"));
	if (rulesRoot != nullptr)
	{
		rules = rulesRoot->getOrderedSubEntries(_T("rule#*"));
		if (rules->size() > 0)
		{
		   //get rule ordering
		   ObjectArray<uuid> *ruleOrdering = GetRuleOrdering(config->getEntry(_T("/ruleOrdering")));
         for(i = 0; i < rules->size(); i++)
         {
            EPRule *rule = new EPRule(rules->get(i));
            g_pEventPolicy->importRule(rule, (flags & CFG_IMPORT_REPLACE_EPP_RULES) != 0, ruleOrdering);
         }
         delete ruleOrdering;
         if (!g_pEventPolicy->saveToDB())
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): unable to import event processing policy rules"));
            rcc = RCC_DB_FAILURE;
            goto stop_processing;
         }
		}
		nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): event processing policy rules imported"));
	}

	// Import scripts
	scriptsRoot = config->getEntry(_T("/scripts"));
	if (scriptsRoot != nullptr)
	{
		scripts = scriptsRoot->getSubEntries(_T("script#*"));
		for(i = 0; i < scripts->size(); i++)
		{
         ImportScript(scripts->get(i), (flags & CFG_IMPORT_REPLACE_SCRIPTS) != 0);
		}
		nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): scripts imported"));
	}

	// Import object tools
	objectToolsRoot = config->getEntry(_T("/objectTools"));
	if (objectToolsRoot != nullptr)
	{
		objectTools = objectToolsRoot->getSubEntries(_T("objectTool#*"));
		for(i = 0; i < objectTools->size(); i++)
		{
         ImportObjectTool(objectTools->get(i), (flags & CFG_IMPORT_REPLACE_OBJECT_TOOLS) != 0);
		}
		nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): object tools imported"));
	}

	// Import summary tables
	summaryTablesRoot = config->getEntry(_T("/dciSummaryTables"));
	if (summaryTablesRoot != nullptr)
	{
	   summaryTables = summaryTablesRoot->getSubEntries(_T("table#*"));
		for(i = 0; i < summaryTables->size(); i++)
		{
         ImportSummaryTable(summaryTables->get(i), (flags & CFG_IMPORT_REPLACE_SUMMARY_TABLES) != 0);
		}
		nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): DCI summary tables imported"));
	}

	//Import web service definitions
   webServiceDefRoot = config->getEntry(_T("/webServiceDefinitions"));
   if (webServiceDefRoot != nullptr)
   {
      webServiceDef = webServiceDefRoot->getSubEntries(_T("webServiceDefinition#*"));
      for(i = 0; i < webServiceDef->size(); i++)
      {
         ImportWebServiceDefinition(webServiceDef->get(i), (flags & CFG_IMPORT_REPLACE_WEB_SVCERVICE_DEFINITIONS) != 0);
      }
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ImportConfig(): DCI summary tables imported"));
   }

stop_processing:
	delete events;
	delete traps;
	delete templates;
   delete rules;
   delete scripts;
   delete objectTools;
   delete summaryTables;
   delete webServiceDef;
   delete actions;

	nxlog_debug_tag(DEBUG_TAG, 4, _T("ImportConfig() finished, rcc = %d"), rcc);
	return rcc;
}

/**
 * Import local configuration (configuration files stored on server)
 */
void ImportLocalConfiguration(bool overwrite)
{
   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, path);
   _tcscat(path, SDIR_TEMPLATES);

   int count = 0;
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Import configuration files from %s"), path);
   _TDIR *dir = _topendir(path);
   if (dir != nullptr)
   {
      _tcscat(path, FS_PATH_SEPARATOR);
      int insPos = (int)_tcslen(path);

      struct _tdirent *f;
      while((f = _treaddir(dir)) != nullptr)
      {
         if (MatchString(_T("*.xml"), f->d_name, FALSE))
         {
            _tcscpy(&path[insPos], f->d_name);
            Config *config = new Config(false);
            if (config->loadXmlConfig(path, "configuration"))
            {
               ImportConfig(config, overwrite ? CFG_IMPORT_REPLACE_EVERYTHING : 0);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 1, _T("Error loading configuration from %s"), path);
            }
            delete config;
         }
      }
      _tclosedir(dir);
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("%d configuration files processed"), count);
}
