/**
 * 
 */
package org.netxms.ui.eclipse.reporter.widgets.helpers;

import java.util.HashMap;
import java.util.Map;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementMap;
import org.simpleframework.xml.Root;

/**
 * Report's parameter
 */
@Root(name="parameter", strict=false)
public class ReportParameter
{
	public static final int INTEGER = 0;
	public static final int STRING = 1;
	public static final int OBJECT_ID = 2;
	public static final int USER_ID = 3;
	public static final int OBJECT_LIST = 4;
	public static final int TIMESTAMP = 5;
	
	private static final Map<String, Integer> TYPE_MAP;
	
	static
	{
		TYPE_MAP = new HashMap<String, Integer>();
		TYPE_MAP.put("java.lang.Integer", INTEGER);
		TYPE_MAP.put("java.lang.String", STRING);
		TYPE_MAP.put("java.util.Date", TIMESTAMP);
		TYPE_MAP.put("timestamp", TIMESTAMP);
		TYPE_MAP.put("object", OBJECT_ID);
		TYPE_MAP.put("objectList", OBJECT_LIST);
		TYPE_MAP.put("user", USER_ID);
	}
	
	@Attribute
	private String name;

	@Attribute
	private String javaClass;
	
	@Element(name="parameterDescription")
	private String description;
	
	@Element(name="defaultValueExpression")
	private String defaultValue;
	
	@ElementMap(entry="property", key="name", value="value", inline=true, attribute=true)
	private Map<String, String> properties;
	
	/**
	 * Get display name for parameter
	 * 
	 * @return
	 */
	public String getDisplayName()
	{
		String value = properties.get("netxms.displayName");
		return (value != null) ? value : description;
	}

	/**
	 * Get data type for parameter
	 * 
	 * @return
	 */
	public int getDataType()
	{
		String nxtype = properties.get("netxms.type");
		Integer code = TYPE_MAP.get((nxtype != null) ? nxtype : javaClass);
		return (code != null) ? code : STRING;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the defaultValue
	 */
	public String getDefaultValue()
	{
		return defaultValue;
	}

	/**
	 * @return the javaClass
	 */
	public String getJavaClass()
	{
		return javaClass;
	}

	/**
	 * @return the properties
	 */
	public Map<String, String> getProperties()
	{
		return properties;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
}
