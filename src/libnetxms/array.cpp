/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: array.cpp
**
**/

#include "libnetxms.h"

/**
 * Default object destructor
 */
static void ObjectDestructor(void *object)
{
	free(object);
}

/**
 * Constructor
 *
 * @param initial initial array size
 * @param grow number of elements to add when array has to grow
 * @param owner if set to true, array is an owner of added objects and will destroy them on removed from array
 */
Array::Array(int initial, int grow, bool owner)
{
	m_size = 0;
	m_grow = (grow > 0) ? grow : 16;
	m_allocated = (initial >= 0) ? initial : 16;
   m_elementSize = sizeof(void *);
	m_data = (m_allocated > 0) ? (void **)malloc(m_elementSize * m_allocated) : NULL;
	m_objectOwner = owner;
	m_objectDestructor = ObjectDestructor;
}

/**
 * Destructor
 */
Array::~Array()
{
	if (m_objectOwner)
	{
		for(int i = 0; i < m_size; i++)
			destroyObject(m_data[i]);
	}
	safe_free(m_data);
}

/**
 * Add pointer-sized element by value
 */
int Array::add(void *object)
{
	if (m_size == m_allocated)
	{
		m_allocated += m_grow;
		m_data = (void **)realloc(m_data, m_elementSize * m_allocated);
	}
   if (m_elementSize <= sizeof(void *))
	   m_data[m_size++] = object;
   else
   	memcpy(&m_data[m_size++], element, m_elementSize);
	return m_size - 1;
}

/**
 * Set object at given index. If index is within array bounds, object at this position will be replaced,
 * otherwise array will be expanded as required. Other new positions created during expansion will
 * be filled with NULL values.
 */
void Array::set(int index, void *object)
{
	if (index < 0)
		return;

	if (index < m_size)
	{
		if (m_objectOwner)
			destroyObject(m_data[index]);
	}
	else
	{
		// Expand array
		if (index >= m_allocated)
		{
			m_allocated += m_grow * ((index - m_allocated) / m_grow + 1);
			m_data = (void **)realloc(m_data, m_elementSize * m_allocated);
		}
		memset(&m_data[m_size], 0, m_elementSize * (index - m_size));
		m_size = index + 1;
	}

   if (m_elementSize <= sizeof(void *))
   	m_data[index] = object;
   else
   	memcpy(&m_data[index], element, m_elementSize);
}

/**
 * Replace object at given index. If index is beyond array bounds,
 * this method will do nothing.
 */
void Array::replace(int index, void *object)
{
	if ((index < 0) || (index >= m_size))
		return;

	if (m_objectOwner)
		destroyObject(m_data[index]);

   if (m_elementSize <= sizeof(void *))
   	m_data[index] = object;
   else
   	memcpy(&m_data[index], element, m_elementSize);
}

/**
 * Remove element at given index
 */
void Array::internalRemove(int index, bool allowDestruction)
{
	if ((index < 0) || (index >= m_size))
		return;

	if (m_objectOwner && allowDestruction)
		destroyObject(m_data[index]);
	m_size--;
	memmove(&m_data[index], &m_data[index + 1], m_elementSize * (m_size - index));
}

/**
 * Clear array
 */
void Array::clear()
{
	if (m_objectOwner)
	{
		for(int i = 0; i < m_size; i++)
			destroyObject(m_data[i]);
	}

	m_size = 0;
	if (m_allocated > m_grow)
	{
		m_data = (void **)realloc(m_data, m_elementSize * m_grow);
		m_allocated = m_grow;
	}
}

/**
 * Get index of given object
 */
int Array::indexOf(void *object)
{
	for(int i = 0; i < m_size; i++)
      if (m_data[i] == object)
         return i;
   return -1;
}
