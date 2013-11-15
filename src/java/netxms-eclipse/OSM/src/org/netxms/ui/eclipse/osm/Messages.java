package org.netxms.ui.eclipse.osm;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.osm.messages"; //$NON-NLS-1$
	public static String AbstractGeolocationView_ZoomIn;
	public static String AbstractGeolocationView_ZoomOut;
	public static String GeoMapViewer_DownloadError;
	public static String GeoMapViewer_DownloadJob_Title;
	public static String GeoMapViewer_LoadMissingJob_Title;
	public static String LocationMap_InitError1;
	public static String LocationMap_InitError2;
	public static String LocationMap_PartNamePrefix;
	public static String OpenLocationMap_Error;
	public static String OpenLocationMap_ErrorText;
	public static String OpenWorldMap_Error;
	public static String OpenWorldMap_ErrorText;
	public static String WorldMap_JobError;
	public static String WorldMap_JobTitle;
	public static String WorldMap_PlaceObject;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
 }


	private static Messages instance = new Messages();

	public static Messages get()
	{
		return instance;
	}

}
