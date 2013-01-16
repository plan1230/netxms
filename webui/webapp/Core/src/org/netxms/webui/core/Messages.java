package org.netxms.webui.core;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.webui.core.messages"; //$NON-NLS-1$
	
	public String AbstractSelector_CopyToClipboard;
	public String AbstractSelector_Select;
	public String ApplicationActionBarAdvisor_About;
	public String ApplicationActionBarAdvisor_AboutActionName;
	public String ApplicationActionBarAdvisor_AboutText1;
	public String ApplicationActionBarAdvisor_AboutText2;
	public String ApplicationActionBarAdvisor_Config;
	public String ApplicationActionBarAdvisor_File;
	public String ApplicationActionBarAdvisor_Help;
	public String ApplicationActionBarAdvisor_Monitor;
	public String ApplicationActionBarAdvisor_Navigation;
	public String ApplicationActionBarAdvisor_OpenPerspective;
	public String ApplicationActionBarAdvisor_Progress;
	public String ApplicationActionBarAdvisor_ShowView;
	public String ApplicationActionBarAdvisor_Tools;
	public String ApplicationActionBarAdvisor_View;
	public String ApplicationActionBarAdvisor_Window;
	public String ApplicationWorkbenchWindowAdvisor_AppTitle;
	public String ApplicationWorkbenchWindowAdvisor_CannotChangePswd;
	public String ApplicationWorkbenchWindowAdvisor_ConnectionError;
	public String ApplicationWorkbenchWindowAdvisor_Error;
	public String ApplicationWorkbenchWindowAdvisor_Exception;
	public String ApplicationWorkbenchWindowAdvisor_Information;
	public String ApplicationWorkbenchWindowAdvisor_PasswordChanged;
	public String ConsoleJob_ErrorDialogTitle;
	public String FilterText_CloseFilter;
	public String FilterText_Filter;
	public String FilterText_FilterIsEmpty;
	public String LoginForm_AdvOptionsDisabled;
	public String LoginForm_Error;
	public String LoginForm_LoginButton;
	public String LoginForm_Options;
	public String LoginForm_Password;
	public String LoginForm_Title;
	public String LoginForm_UserName;
	public String LoginForm_Version;
	public String LoginJob_InitExtensions;
	public String LoginJob_LoadingEvents;
	public String LoginJob_Subscribing;
	public String LoginJob_SyncObjects;
	public String LoginJob_SyncUsers;
	public String LoginSettingsDialog_ServerAddress;
	public String LoginSettingsDialog_Title;
	public String PasswordExpiredDialog_ConfirmNewPassword;
	public String PasswordExpiredDialog_NewPassword;
	public String PasswordExpiredDialog_Prompt;
	public String PasswordExpiredDialog_Title;
	public String RefreshAction_Name;
	public String WidgetHelper_Action_Copy;
	public String WidgetHelper_Action_Cut;
	public String WidgetHelper_Action_Delete;
	public String WidgetHelper_Action_Paste;
	public String WidgetHelper_Action_SelectAll;
	
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}

	
	/**
	 * Get message class for current locale
	 * 
	 * @return
	 */
	public static Messages get()
	{
		return RWT.NLS.getISO8859_1Encoded(BUNDLE_NAME, Messages.class);
	}
}
