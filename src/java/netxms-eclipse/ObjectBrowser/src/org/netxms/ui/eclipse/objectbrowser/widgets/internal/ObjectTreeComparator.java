/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.objectbrowser.widgets.internal;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.netxms.client.objects.GenericObject;

/**
 * Comparator for object tree
 *
 */
public class ObjectTreeComparator extends ViewerComparator
{
	// Categories
	private static final int CATEGORY_CONTAINER = 10;
	private static final int CATEGORY_OTHER = 255;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#category(java.lang.Object)
	 */
	@Override
	public int category(Object element)
	{
		if (((GenericObject)element).getObjectId() < 10)
			return (int)((GenericObject)element).getObjectId();
		if ((((GenericObject)element).getObjectClass() == GenericObject.OBJECT_CONTAINER) ||
		    (((GenericObject)element).getObjectClass() == GenericObject.OBJECT_TEMPLATEGROUP) ||
		    (((GenericObject)element).getObjectClass() == GenericObject.OBJECT_POLICYGROUP))
			return CATEGORY_CONTAINER;
		return CATEGORY_OTHER;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		return super.compare(viewer, e1, e2);
	}
}
