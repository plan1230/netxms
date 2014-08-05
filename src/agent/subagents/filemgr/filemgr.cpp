/*
 ** File management subagent
 ** Copyright (C) 2014 Raden Solutions
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
 **/

#include "filemgr.h"

/**
 * Root folders
 */
static StringList *g_rootFileManagerFolders;

/**
 * Monitored file list
 */
MonitoredFileList g_monitorFileList;

#ifdef _WIN32

/**
 * Convert path from UNIX to local format
 */
static void ConvertPathToHost(TCHAR *path)
{
   for(int i = 0; path[i] != 0; i++)
      if (path[i] == _T('/'))
         path[i] = _T('\\');
}

#else

#define ConvertPathToHost(x)

#endif

#ifdef _WIN32

/**
 * Convert path from local to UNIX format
 */
static void ConvertPathToNetwork(TCHAR *path)
{
   for(int i = 0; path[i] != 0; i++)
      if (path[i] == _T('\\'))
         path[i] = _T('/');
}

#else

#define ConvertPathToNetwork(x)

#endif

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
   g_rootFileManagerFolders = new StringList();
   ConfigEntry *root = config->getEntry(_T("/filemgr/RootFolder"));
   if (root != NULL)
   {
      for(int i = 0; i < root->getValueCount(); i++)
      {
         g_rootFileManagerFolders->add(root->getValue(i));
         AgentWriteDebugLog(5, _T("FILEMGR: added root folder %s"), root->getValue(i));
      }
   }
   AgentWriteDebugLog(2, _T("FILEMGR: subagent initialized"));
   return TRUE;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   delete g_rootFileManagerFolders;
}

#ifndef _WIN32

/**
 * Converts path to absolute removing "//", "../", "./" ...
 */
static TCHAR *getLinuxRealPath(TCHAR *path)
{
   if(path == NULL || path[0] == 0)
      return NULL;
   TCHAR *result = (TCHAR*)malloc(sizeof(TCHAR)*MAX_PATH);
   _tcscpy(result,path);
   TCHAR *current = result;

   //just remove all dots before path
   if(!_tcsncmp(current,_T("../"),3))
      memmove(current,current+3,(_tcslen(current+3) + 1) * sizeof(TCHAR));

   if(!_tcsncmp(current,_T("./"),2))
      memmove(current,current+2,(_tcslen(current+2) + 1) * sizeof(TCHAR));

   while(current[0] != 0)
   {
      if(current[0] == '/')
      {
         if(current[1] != 0)
         {
            switch(current[1])
            {
               case '/':
                  memmove(current,current+1,(_tcslen(current+1) + 1) * sizeof(TCHAR));
                  break;
               case '.':
                  if(current[2] != 0)
                  {
                     if(current[2] == '.' && (current[3] == 0 || current[3] == '/'))
                     {
                        if(current == result)
                        {
                           memmove(current,current+3,(_tcslen(current+3) + 1) * sizeof(TCHAR));
                        }
                        else
                        {
                           TCHAR *tmp = current;
                           do
                           {
                              tmp--;
                              if(tmp[0] == '/')
                              {
                                 break;
                              }
                           } while(result != tmp);
                           memmove(tmp,current+3,(_tcslen(current+3) + 1) * sizeof(TCHAR));
                        }
                     }
                  }
                  else
                  {
                     memmove(current,current+2,(_tcslen(current+2) + 1) * sizeof(TCHAR));
                  }
               default:
                  current++;
            }
         }
         else
         {
            current++;
         }
      }
      else
      {
         current++;
      }
   }
   return result;
}

#endif

/**
 * Takes folder/file path - make it absolute (result will be written back to the folder variable)
 * and check that this folder/file is under allowed root path.
 * If second parameter is set to true - then request is for getting content and "/" path should be acepted
 * and afterwards treatet as: "give list of all allowd folders".
 */
static BOOL CheckFullPath(TCHAR *folder, bool withHomeDir)
{
   AgentWriteDebugLog(3, _T("FILEMGR: CheckFullPath: input is %s"), folder);
   if (withHomeDir && !_tcscmp(folder, FS_PATH_SEPARATOR))
   {
      return TRUE;
   }

#ifdef _WIN32
   TCHAR *fullPathT = _tfullpath(NULL, folder, MAX_PATH);
#else
   TCHAR *fullPathT = getLinuxRealPath(folder);
#endif
   AgentWriteDebugLog(3, _T("FILEMGR: CheckFullPath: Full path %s"), fullPathT);
   if (fullPathT != NULL)
   {
      _tcscpy(folder, fullPathT);
      safe_free(fullPathT);
   }
   else
   {
      return FALSE;
   }
   for(int i = 0; i < g_rootFileManagerFolders->size(); i++)
   {
      if (!_tcsncmp(g_rootFileManagerFolders->get(i), folder, _tcslen(g_rootFileManagerFolders->get(i))))
         return TRUE;
   }

   return FALSE;
}

#define REGULAR_FILE    1
#define DIRECTORY       2
#define SYMLINC         4

/**
 * Puts in response list of containing files
 */
static void GetFolderContent(TCHAR *folder, CSCPMessage *msg, bool rootFolder)
{
   NX_STAT_STRUCT st;

   msg->SetVariable(VID_RCC, ERR_SUCCESS);
   UINT32 count = 0;
   UINT32 varId = VID_INSTANCE_LIST_BASE;

   if (!_tcscmp(folder, FS_PATH_SEPARATOR) && rootFolder)
   {
      for(int i = 0; i < g_rootFileManagerFolders->size(); i++)
      {
         if (_taccess(g_rootFileManagerFolders->get(i), 4) != 0)
            continue;

         if (CALL_STAT(g_rootFileManagerFolders->get(i), &st) == 0)
         {
            msg->SetVariable(varId++, g_rootFileManagerFolders->get(i));
            msg->SetVariable(varId++, (QWORD)st.st_size);
            msg->SetVariable(varId++, (QWORD)st.st_mtime);
            UINT32 type = 0;
#ifndef _WIN32
            if(S_ISLNK(st.st_mode))
            {
               type |= SYMLINC;
               NX_STAT_STRUCT symlincSt;
               if (CALL_STAT_FOLLOW_SYMLINK(g_rootFileManagerFolders->get(i), &symlincSt) == 0)
               {
                  type |= S_ISDIR(symlincSt.st_mode) ? DIRECTORY : 0;

               }
            }
#endif
            type |= S_ISREG(st.st_mode) ? REGULAR_FILE : 0;
            type |= S_ISDIR(st.st_mode) ? DIRECTORY : 0;
            msg->SetVariable(varId++, type);
            TCHAR fullName[MAX_PATH];
            _tcscpy(fullName, g_rootFileManagerFolders->get(i));
            msg->SetVariable(varId++, fullName);

            varId += 5;
            count++;
         }
         else
         {
            AgentWriteDebugLog(3, _T("FILEMGR: GetFolderContent: Not possible to get folder %s"), g_rootFileManagerFolders->get(i));
         }
      }
      msg->SetVariable(VID_INSTANCE_COUNT, count);
      return;
   }

   _TDIR *dir = _topendir(folder);
   if (dir != NULL)
   {
      struct _tdirent *d;
      while((d = _treaddir(dir)) != NULL)
      {
         if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
         {
            continue;
         }

         TCHAR fullName[MAX_PATH];
         _tcscpy(fullName, folder);
         _tcscat(fullName, FS_PATH_SEPARATOR);
         _tcscat(fullName, d->d_name);

         if (_taccess(fullName, 4) != 0)
            continue;

         if (CALL_STAT(fullName, &st) == 0)
         {
            msg->SetVariable(varId++, d->d_name);
            msg->SetVariable(varId++, (QWORD)st.st_size);
            msg->SetVariable(varId++, (QWORD)st.st_mtime);
            UINT32 type = 0;
#ifndef _WIN32
            if(S_ISLNK(st.st_mode))
            {
               type |= SYMLINC;
               NX_STAT_STRUCT symlincSt;
               if (CALL_STAT_FOLLOW_SYMLINK(d->d_name, &symlincSt) == 0)
               {
                  type |= S_ISDIR(symlincSt.st_mode) ? DIRECTORY : 0;

               }
            }
#endif
            type |= S_ISREG(st.st_mode) ? REGULAR_FILE : 0;
            type |= S_ISDIR(st.st_mode) ? DIRECTORY : 0;
            msg->SetVariable(varId++, type);
            ConvertPathToNetwork(fullName);
            msg->SetVariable(varId++, fullName);

            /*
            S_IFMT     0170000   bit mask for the file type bit fields

            S_IFSOCK   0140000   socket
            S_IFLNK    0120000   symbolic link
            S_IFREG    0100000   regular file
            S_IFBLK    0060000   block device
            S_IFDIR    0040000   directory
            S_IFCHR    0020000   character device
            S_IFIFO    0010000   FIFO
            */

            varId += 5;
            count++;
         }
         else
         {
             AgentWriteDebugLog(6, _T("FILEMGR: GetFolderContent: Not possible to get folder %s"), fullName);
         }
      }
      msg->SetVariable(VID_INSTANCE_COUNT, count);
      _tclosedir(dir);
   }
   else
   {
      msg->SetVariable(VID_RCC, RCC_IO_ERROR);
   }
}

/**
 * Delete file/folder
 */
static BOOL Delete(const TCHAR *name)
{
   NX_STAT_STRUCT st;

   if (CALL_STAT(name, &st) != 0)
   {
      return FALSE;
   }

   bool result = true;

   if (S_ISDIR(st.st_mode))
   {
      // get element list and for each element run Delete
      _TDIR *dir = _topendir(name);
      if (dir != NULL)
      {
         struct _tdirent *d;
         while((d = _treaddir(dir)) != NULL)
         {
            if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
            {
               continue;
            }
            TCHAR newName[MAX_PATH];
            _tcscpy(newName, name);
            _tcscat(newName, FS_PATH_SEPARATOR);
            _tcscat(newName, d->d_name);
            result = result && Delete(newName);
         }
         _tclosedir(dir);
      }
      //remove directory
#ifdef _WIN32
      return RemoveDirectory(name);
#else
      return _trmdir(name) == 0;
#endif
   }
#ifdef _WIN32
   return DeleteFile(name);
#else
   return _tremove(name) == 0;
#endif
}

/**
 * Rename file/folder
 */
static BOOL Rename(TCHAR* oldName, TCHAR * newName)
{
   if (_trename(oldName, newName) == 0)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

#ifndef _WIN32

/**
 * Copy file/folder
 */
static BOOL CopyFile(NX_STAT_STRUCT *st, TCHAR* oldName, TCHAR* newName)
{
   int oldFile, newFile;
   oldFile = _topen(oldName, O_RDONLY | O_BINARY);
   newFile = _topen(newName, O_CREAT | O_BINARY | O_WRONLY, st->st_mode); // should be copyed with the same acess rights
   if (oldFile == -1 || newFile == -1)
   {
      return FALSE;
   }

   BYTE *readBytes;
   int readSize = 16384, readIn, readOut;
   readBytes = (BYTE*)malloc(readSize);

   while((readIn = read (oldFile, readBytes, readSize)) > 0)
   {
      readOut = write(newFile, readBytes, (ssize_t) readIn);
      if(readOut != readIn)
      {
         close(oldFile);
         close(newFile);
         return FALSE;
      }
    }

    /* Close file descriptors */
    close(oldFile);
    close(newFile);
    return TRUE;
}

#endif

/**
 * Move file/folder
 */
static BOOL MoveFile(TCHAR* oldName, TCHAR* newName)
{
#ifdef _WIN32
   return MoveFileEx(oldName, newName, MOVEFILE_COPY_ALLOWED);
#else
   if (Rename(oldName, newName))
   {
      return TRUE;
   }

   NX_STAT_STRUCT st;

   if (CALL_STAT(oldName, &st) != 0)
   {
      return FALSE;
   }

   if (S_ISDIR(st.st_mode))
   {
      _tmkdir(newName, st.st_mode);
      _TDIR *dir = _topendir(oldName);
      if (dir != NULL)
      {
         struct _tdirent *d;
         while((d = _treaddir(dir)) != NULL)
         {
            if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
            {
               continue;
            }
            TCHAR nextNewName[MAX_PATH];
            _tcscpy(nextNewName, newName);
            _tcscat(nextNewName, _T("/"));
            _tcscat(nextNewName, d->d_name);

            TCHAR nextOldaName[MAX_PATH];
            _tcscpy(nextOldaName, oldName);
            _tcscat(nextOldaName, _T("/"));
            _tcscat(nextOldaName, d->d_name);

            MoveFile(nextOldaName, nextNewName);
         }
         _tclosedir(dir);
      }
      _trmdir(oldName);
   }
   else
   {
      if (!CopyFile(&st, oldName, newName))
         return false;
   }
   return TRUE;
#endif /* _WIN32 */
}

/**
 * Send file
 */
 THREAD_RESULT THREAD_CALL SendFile(void *dataStruct)
{
   MessageData *data = (MessageData*)dataStruct;
   //prepare file name

   AgentWriteDebugLog(5, _T("CommSession::getLocalFile(): request for file \"%s\", follow = %s"),
               data->fileName, data->follow ? _T("true") : _T("false"));
   BOOL result = AgentSendFileToServer(data->session, data->id, data->fileName, (int)data->offset);
   if(data->follow && result)
   {
      TCHAR *fileID = _tcsdup(data->fileNameCode);
      TCHAR *realName = _tcsdup(data->fileName);
      g_monitorFileList.addMonitoringFile(fileID);
      FollowData *flData = new FollowData();
      flData->serverAddress = data->session->getServerAddress();
      flData->pszFile = realName;
      flData->fileId = fileID;
      flData->offset = 0;
      flData->session = data->session;
      ThreadCreateEx(SendFileUpdatesOverNXCP, 0, (void *)flData);
   }
   safe_free(data->fileName);
   safe_free(data->fileNameCode);
   delete data;
   return THREAD_OK;
}

/**
 * Process commands like get files in folder, delete file/folder, copy file/folder, move file/folder
 */
static BOOL ProcessCommands(UINT32 command, CSCPMessage *request, CSCPMessage *response, void *session)
{
   switch(command)
   {
      case CMD_GET_FOLDER_CONTENT:
      {
         TCHAR directory[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, directory, MAX_PATH);
         response->SetId(request->GetId());
         if (directory == NULL)
         {
            response->SetVariable(VID_RCC, RCC_IO_ERROR);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): File name should be set."));
            return TRUE;
         }
         ConvertPathToHost(directory);

         bool rootFolder = request->GetVariableShort(VID_ROOT) ? 1 : 0;
         if (CheckFullPath(directory, rootFolder) && ((AbstractCommSession *)session)->isMasterServer())
         {
            GetFolderContent(directory, response, rootFolder);
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): Access denied"));
            response->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMGR_DELETE_FILE:
      {
         TCHAR file[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, file, MAX_PATH);
         response->SetId(request->GetId());
         if(file == NULL)
         {
            response->SetVariable(VID_RCC, RCC_IO_ERROR);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): File name should be set."));
            return TRUE;
         }
         ConvertPathToHost(file);

         if(CheckFullPath(file, false) && ((AbstractCommSession *)session)->isMasterServer())
         {
            if (Delete(file))
            {
               response->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->SetVariable(VID_RCC, RCC_IO_ERROR);
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): Access denied"));
            response->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMGR_RENAME_FILE:
      {
         TCHAR oldName[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, oldName, MAX_PATH);
         TCHAR newName[MAX_PATH];
         request->GetVariableStr(VID_NEW_FILE_NAME, newName, MAX_PATH);
         response->SetId(request->GetId());
         if(oldName == NULL && newName == NULL)
         {
            response->SetVariable(VID_RCC, RCC_IO_ERROR);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): File names should be set."));
            return TRUE;
         }
         ConvertPathToHost(oldName);
         ConvertPathToHost(newName);

         if (CheckFullPath(oldName, false) && CheckFullPath(newName, false) && ((AbstractCommSession *)session)->isMasterServer())
         {
            if (Rename(oldName, newName))
            {
               response->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->SetVariable(VID_RCC, RCC_IO_ERROR);
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): Access denied"));
            response->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMGR_MOVE_FILE:
      {
         TCHAR oldName[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, oldName, MAX_PATH);
         TCHAR newName[MAX_PATH];
         request->GetVariableStr(VID_NEW_FILE_NAME, newName, MAX_PATH);
         response->SetId(request->GetId());
         if(oldName == NULL && newName == NULL)
         {
            response->SetVariable(VID_RCC, RCC_IO_ERROR);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): File names should be set."));
            return TRUE;
         }
         ConvertPathToHost(oldName);
         ConvertPathToHost(newName);

         if (CheckFullPath(oldName, false) && CheckFullPath(newName, false) && ((AbstractCommSession *)session)->isMasterServer())
         {
            if(MoveFile(oldName, newName))
            {
               response->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->SetVariable(VID_RCC, RCC_IO_ERROR);
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): Access denied"));
            response->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_FILEMGR_UPLOAD:
      {
         TCHAR name[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, name, MAX_PATH);
         response->SetId(request->GetId());
         if (name == NULL)
         {
            response->SetVariable(VID_RCC, RCC_IO_ERROR);
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): File name should be set."));
            return TRUE;
         }
         ConvertPathToHost(name);

         if (CheckFullPath(name, false) && ((AbstractCommSession *)session)->isMasterServer())
         {
            response->SetVariable(VID_RCC, ((AbstractCommSession *)session)->openFile(name, request->GetId()));
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): Access denied"));
            response->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_GET_FILE_DETAILS:
      {
         TCHAR fileName[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, fileName, MAX_PATH);
         ExpandFileName(fileName, fileName, MAX_PATH, false);
         response->SetId(request->GetId());

      	if (((AbstractCommSession *)session)->isMasterServer() && CheckFullPath(fileName, false))
         {
            NX_STAT_STRUCT fs;

            //prepare file name
            if (NX_STAT(fileName, &fs) == 0)
            {
               response->SetVariable(VID_FILE_SIZE, (UINT64)fs.st_size);
               response->SetVariable(VID_MODIFY_TIME, (UINT64)fs.st_mtime);
               response->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->SetVariable(VID_RCC, ERR_FILE_STAT_FAILED);
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("FILEMGR: ProcessCommands(): Access denied"));
            response->SetVariable(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_GET_AGENT_FILE:
      {
         response->SetId(request->GetId());
         TCHAR fileName[MAX_PATH];
         request->GetVariableStr(VID_FILE_NAME, fileName, MAX_PATH);
         ExpandFileName(fileName, fileName, MAX_PATH, false);

         if (((AbstractCommSession *)session)->isMasterServer() && CheckFullPath(fileName, false))
         {
            TCHAR *fileNameCode = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
            request->GetVariableStr(VID_NAME, fileNameCode, MAX_PATH);

            MessageData *data = new MessageData();
            data->fileName = _tcsdup(fileName);
            data->fileNameCode = fileNameCode;
            data->follow = request->GetVariableShort(VID_FILE_FOLLOW) ? true : false;
            data->id = request->GetId();
            data->offset = request->GetVariableLong(VID_FILE_OFFSET);
            data->session = ((AbstractCommSession *)session);

            ThreadCreateEx(SendFile, 0, data);

            response->SetVariable(VID_RCC, ERR_SUCCESS);
         }
         else
         {
            response->SetVariable(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      case CMD_CANCEL_FILE_MONITORING:
      {
         response->SetId(request->GetId());
         if (((AbstractCommSession *)session)->isMasterServer())
         {
            TCHAR fileName[MAX_PATH];
            request->GetVariableStr(VID_FILE_NAME, fileName, MAX_PATH);
            if(g_monitorFileList.removeMonitoringFile(fileName))
            {
               response->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               response->SetVariable(VID_RCC, ERR_BAD_ARGUMENTS);
            }
         }
         else
         {
            response->SetVariable(VID_RCC, ERR_ACCESS_DENIED);
         }
         return TRUE;
      }
      default:
         return FALSE;
   }
}

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("FILEMGR"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, ProcessCommands,
   0, NULL, // parameters
   0, NULL, //enums
   0, NULL, // tables
   0, NULL, // actions
   0, NULL  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(FILEMGR)
{
   *ppInfo = &m_info;
   return TRUE;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
