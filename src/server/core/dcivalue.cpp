/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: dcivalue.cpp
**
**/

#include "nxcore.h"


//
// Default constructor
//

ItemValue::ItemValue()
{
   m_szString[0] = 0;
   m_iInt32 = 0;
   m_iInt64 = 0;
   m_dwInt32 = 0;
   m_qwInt64 = 0;
   m_dFloat = 0;
   m_dwTimeStamp = (DWORD)time(NULL);
}


//
// Construct value object from string value
//

ItemValue::ItemValue(const TCHAR *pszValue, DWORD dwTimeStamp)
{
   nx_strncpy(m_szString, pszValue, MAX_DB_STRING);
   m_iInt32 = strtol(m_szString, NULL, 0);
   m_iInt64 = strtoll(m_szString, NULL, 0);
   m_dwInt32 = strtoul(m_szString, NULL, 0);
   m_qwInt64 = strtoull(m_szString, NULL, 0);
   m_dFloat = strtod(m_szString, NULL);

   if (dwTimeStamp == 0)
      m_dwTimeStamp = (DWORD)time(NULL);
   else
      m_dwTimeStamp = dwTimeStamp;
}


//
// Construct value object from another ItemValue object
//

ItemValue::ItemValue(const ItemValue *pValue)
{
   strcpy(m_szString, pValue->m_szString);
   m_iInt32 = pValue->m_iInt32;
   m_iInt64 = pValue->m_iInt64;
   m_dwInt32 = pValue->m_dwInt32;
   m_qwInt64 = pValue->m_qwInt64;
   m_dFloat = pValue->m_dFloat;
   m_dwTimeStamp = pValue->m_dwTimeStamp;
}


//
// Destructor
//

ItemValue::~ItemValue()
{
}


//
// Assignment operators
//

const ItemValue& ItemValue::operator=(const ItemValue &src)
{
   strcpy(m_szString, src.m_szString);
   m_iInt32 = src.m_iInt32;
   m_iInt64 = src.m_iInt64;
   m_dwInt32 = src.m_dwInt32;
   m_qwInt64 = src.m_qwInt64;
   m_dFloat = src.m_dFloat;
   return *this;
}

const ItemValue& ItemValue::operator=(const TCHAR *pszStr)
{
   nx_strncpy(m_szString, pszStr, MAX_DB_STRING);
   m_iInt32 = strtol(m_szString, NULL, 0);
   m_iInt64 = strtoll(m_szString, NULL, 0);
   m_dwInt32 = strtoul(m_szString, NULL, 0);
   m_qwInt64 = strtoull(m_szString, NULL, 0);
   m_dFloat = strtod(m_szString, NULL);
   return *this;
}

const ItemValue& ItemValue::operator=(double dFloat)
{
   m_dFloat = dFloat;
   sprintf(m_szString, "%f", m_dFloat);
   m_iInt32 = (LONG)m_dFloat;
   m_iInt64 = (INT64)m_dFloat;
   m_dwInt32 = (DWORD)m_dFloat;
   m_qwInt64 = (QWORD)m_dFloat;
   return *this;
}

const ItemValue& ItemValue::operator=(LONG iInt32)
{
   m_iInt32 = iInt32;
   sprintf(m_szString, "%d", m_iInt32);
   m_dFloat = (double)m_iInt32;
   m_iInt64 = (INT64)m_iInt32;
   m_dwInt32 = (DWORD)m_iInt32;
   m_qwInt64 = (QWORD)m_iInt32;
   return *this;
}

const ItemValue& ItemValue::operator=(INT64 iInt64)
{
   m_iInt64 = iInt64;
   sprintf(m_szString, INT64_FMT, m_iInt64);
   m_dFloat = (double)m_iInt64;
   m_iInt32 = (LONG)m_iInt64;
   m_dwInt32 = (DWORD)m_iInt64;
   m_qwInt64 = (QWORD)m_iInt64;
   return *this;
}

const ItemValue& ItemValue::operator=(DWORD dwInt32)
{
   m_dwInt32 = dwInt32;
   sprintf(m_szString, "%u", m_dwInt32);
   m_dFloat = (double)m_dwInt32;
   m_iInt32 = (LONG)m_dwInt32;
   m_iInt64 = (INT64)m_dwInt32;
   m_qwInt64 = (QWORD)m_dwInt32;
   return *this;
}

const ItemValue& ItemValue::operator=(QWORD qwInt64)
{
   m_qwInt64 = qwInt64;
   sprintf(m_szString, UINT64_FMT, m_qwInt64);
   m_dFloat = (double)((INT64)m_qwInt64);
   m_iInt32 = (LONG)m_qwInt64;
   m_iInt64 = (INT64)m_qwInt64;
   return *this;
}


//
// Calculate difference between two values
//

void CalculateItemValueDiff(ItemValue &result, int nDataType,
                            ItemValue &value1, ItemValue &value2)
{
   switch(nDataType)
   {
      case DCI_DT_INT:
         result = (LONG)value1 - (LONG)value2;
         break;
      case DCI_DT_UINT:
         result = (DWORD)value1 - (DWORD)value2;
         break;
      case DCI_DT_INT64:
         result = (INT64)value1 - (INT64)value2;
         break;
      case DCI_DT_UINT64:
         result = (QWORD)value1 - (QWORD)value2;
         break;
      case DCI_DT_FLOAT:
         result = (double)value1 - (double)value2;
         break;
      case DCI_DT_STRING:
         result = (LONG)((_tcscmp((const TCHAR *)value1, (const TCHAR *)value2) == 0) ? 0 : 1);
         break;
      default:
         // Delta calculation is not supported for other types
         result = value1;
         break;
   }
}


//
// Calculate average value for set of values
//

#define CALC_AVG_VALUE(vtype) \
{ \
   vtype var; \
   var = 0; \
   for(i = 0, nValueCount = 0; i < nNumValues; i++) \
   { \
      if (ppValueList[i]->GetTimeStamp() != 1) \
      { \
         var += (vtype)(*ppValueList[i]); \
         nValueCount++; \
      } \
   } \
   result = var / (vtype)nValueCount; \
}

void CalculateItemValueAverage(ItemValue &result, int nDataType,
                               int nNumValues, ItemValue **ppValueList)
{
   int i, nValueCount;

   switch(nDataType)
   {
      case DCI_DT_INT:
         CALC_AVG_VALUE(LONG);
         break;
      case DCI_DT_UINT:
         CALC_AVG_VALUE(DWORD);
         break;
      case DCI_DT_INT64:
         CALC_AVG_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
         CALC_AVG_VALUE(QWORD);
         break;
      case DCI_DT_FLOAT:
         CALC_AVG_VALUE(double);
         break;
      case DCI_DT_STRING:
         result = _T("");   // Average value for string is meaningless
         break;
      default:
         break;
   }
}


//
// Calculate mean absolute deviation for set of values
//

#define CALC_MD_VALUE(vtype) \
{ \
   vtype mean, dev; \
   mean = 0; \
   for(i = 0, nValueCount = 0; i < nNumValues; i++) \
   { \
      if (ppValueList[i - 1]->GetTimeStamp() != 1) \
      { \
         mean += (vtype)(*ppValueList[i - 1]); \
         nValueCount++; \
      } \
   } \
   mean /= (vtype)nValueCount; \
   dev = 0; \
   for(i = 0, nValueCount = 0; i < nNumValues; i++) \
   { \
      if (ppValueList[i - 1]->GetTimeStamp() != 1) \
      { \
         dev += ABS((vtype)(*ppValueList[i - 1]) - mean); \
         nValueCount++; \
      } \
   } \
   result = dev / (vtype)nValueCount; \
}

void CalculateItemValueMD(ItemValue &result, int nDataType,
                          int nNumValues, ItemValue **ppValueList)
{
   int i, nValueCount;

   switch(nDataType)
   {
      case DCI_DT_INT:
#define ABS(x) ((x) < 0 ? -(x) : (x))
         CALC_MD_VALUE(LONG);
         break;
      case DCI_DT_INT64:
         CALC_MD_VALUE(INT64);
         break;
      case DCI_DT_FLOAT:
         CALC_MD_VALUE(double);
         break;
      case DCI_DT_UINT:
#undef ABS
#define ABS(x) (x)
         CALC_MD_VALUE(DWORD);
         break;
      case DCI_DT_UINT64:
         CALC_MD_VALUE(QWORD);
         break;
      case DCI_DT_STRING:
         result = _T("");   // Mean deviation for string is meaningless
         break;
      default:
         break;
   }
}
