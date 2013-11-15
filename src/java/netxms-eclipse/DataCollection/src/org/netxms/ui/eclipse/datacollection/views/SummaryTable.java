/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.views;

import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.views.helpers.TableContentProvider;
import org.netxms.ui.eclipse.datacollection.views.helpers.TableItemComparator;
import org.netxms.ui.eclipse.datacollection.views.helpers.TableLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Display results of table tool execution
 */
public class SummaryTable extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.datacollection.views.SummaryTable"; //$NON-NLS-1$
	
	private NXCSession session;
	private int tableId;
	private long baseObjectId;
	private SortableTableViewer viewer;
	private Action actionRefresh;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = (NXCSession)ConsoleSharedData.getSession();
		
		// Secondary ID must by in form nodeId&pollType
		String[] parts = site.getSecondaryId().split("&"); //$NON-NLS-1$
		if (parts.length != 2)
			throw new PartInitException("Internal error"); //$NON-NLS-1$
		
		tableId = Integer.parseInt(parts[0]);
		baseObjectId = Long.parseLong(parts[1]);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		viewer = new SortableTableViewer(parent, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new TableContentProvider());
		viewer.setLabelProvider(new TableLabelProvider());

		createActions();
		contributeToActionBars();
		createPopupMenu();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refreshTable();
			}
		};

		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}

	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionExportAllToCsv);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionExportAllToCsv);
		manager.add(actionRefresh);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionExportToCsv);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/**
	 * Refresh table
	 */
	public void refreshTable()
	{
		viewer.setInput(null);
		new ConsoleJob(Messages.get().SummaryTable_JobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Table table = session.queryDciSummaryTable(tableId, baseObjectId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						updateViewer(table);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().SummaryTable_JobError;
			}
		}.start();
	}
	
	/**
	 * Update viewer with fresh table data
	 * 
	 * @param table table
	 */
	private void updateViewer(final Table table)
	{
		if (!viewer.isInitialized())
		{
			final String[] names = table.getColumnDisplayNames();
			final int[] widths = new int[names.length];
			Arrays.fill(widths, 100);
			viewer.createColumns(names, widths, 0, SWT.UP);
			WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "SummaryTable." + Integer.toString(tableId)); //$NON-NLS-1$
			viewer.getTable().addDisposeListener(new DisposeListener() {
				@Override
				public void widgetDisposed(DisposeEvent e)
				{
					WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "SummaryTable." + Integer.toString(tableId)); //$NON-NLS-1$
				}
			});
			viewer.setComparator(new TableItemComparator(table.getColumnDataTypes()));
		}
		viewer.setInput(table);
	}
	
	/**
	 * @param table
	 */
	public void setTable(Table table)
	{
		AbstractObject object = session.findObjectById(baseObjectId);
		setPartName(table.getTitle() + " - " + ((object != null) ? object.getObjectName() : ("[" + baseObjectId + "]")));  //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		updateViewer(table);
	}
}
