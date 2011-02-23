/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

package edu.cornell.graphics.exr;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.TimeZone;

/**
 * Small class which holds a handful of standard EXR
 * attributes from <code>ImfStandardAttributes.h</code>
 * An attribute not present in the EXR file will be null
 * or NaN for floating point types.
 * 
 * @author edgar
 */
public class Attributes {

	/**
	 * owner -- name of the owner of the image
	 */
	private String owner = null;
	
	/**
	 * comments -- additional image information in human-readable
	 * form, for example a verbal description of the image
	 */
	private String comments = null;
	
	/**
	 * capDate -- the date when the image was created or captured,
	 * in local time, and formatted as
	 *
     *    YYYY:MM:DD hh:mm:ss
	 *
	 * where YYYY is the year (4 digits, e.g. 2003), MM is the month
	 * (2 digits, 01, 02, ... 12), DD is the day of the month (2 digits,
	 * 01, 02, ... 31), hh is the hour (2 digits, 00, 01, ... 23), mm
	 * is the minute, and ss is the second (2 digits, 00, 01, ... 59).
	 *
	 */
	private String capDate = null;
	
	/**
	 * utcOffset -- offset of local time at capDate from
	 * Universal Coordinated Time (UTC), in seconds:
	 *
     *    UTC == local time + utcOffset
	 */
	private float utcOffset = Float.NaN;
	
	
	/* String with the Java version of the time */
	private static final String DATE_FORMAT = "yyyy:MM:dd HH:mm:ss";
	
	/* The basic UTC zone */
	private final static TimeZone UTC = TimeZone.getTimeZone("UTC");
	
	
	/* Explicit constructor */
	protected Attributes(String owner, String comments, String capDate,
			float utcOffset) {
		this.owner     = owner;
		this.comments  = comments;
		this.capDate   = capDate;
		this.utcOffset = utcOffset;
	}
	
	/**
	 * Internal constructor: when its parameter is true
	 * it sets the owner and date fields automatically
	 * as described in {@link #Attributes()}, otherwise
	 * it leaves the default values without change.
	 * This method is used by the JNI. 
	 */
	private Attributes(boolean hasAutoDefaults) {
		if (hasAutoDefaults) {
			setOwner( System.getProperty("user.name") );
			setDate( Calendar.getInstance() );
		}
	}
	
	/**
	 * The default constructor sets automagically the owner and the
	 * date fields. The owner will be the value of the System
	 * property <code>user.name</code> as defined in 
	 * {@link System#getProperties()}. All other fields are left empty.
	 */
	public Attributes() {
		this(true);
	}
	
	/**
	 * Sets the creation time attribute according to the given
	 * calendar, respecting time zones.
	 * If it is <code>null</code> it clears the attribute;
	 * 
	 * @param cal Calendar set up with the desired creation date
	 *                     or <code>null</code>.
	 */
	public void setDate(Calendar cal) {
		
		if (cal == null) {
			capDate = null;
			utcOffset = Float.NaN;
		}
		else {
			// For Java the offset is with respect to UTC, eg:
			// offset(EDT) = -5 hr + 1 hr
			// but for EXR the offset is with respect to the local time, so that
			// UTC = local + offset
			utcOffset = (cal.get(Calendar.ZONE_OFFSET) + 
					cal.get(Calendar.DST_OFFSET)) / (-1000.0f);
			
			SimpleDateFormat formatter = new SimpleDateFormat(DATE_FORMAT);
			Date date = cal.getTime();
			this.capDate   = formatter.format(date);
		}
	}
	
	/**
	 * Sets the creation time attribute with the given UTC time. Using
	 * this method will record the date as UTC and the time zone information
	 * will be lost. To keep the time zone data use {@link #setDate(Calendar)}.
	 * If it is <code>null</code> it clears the attribute;
	 * 
	 * @param date the desired creation date or <code>null</code>.
	 */
	public void setDate(Date date) {
		
		if (date == null) {
			capDate = null;
			utcOffset = Float.NaN;
		}
		else {
			utcOffset = 0.0f;
			SimpleDateFormat formatter = new SimpleDateFormat(DATE_FORMAT);
			formatter.setTimeZone(UTC);
			this.capDate   = formatter.format(date);
		}
	}
	
	/** Retrieves the owner. */
	public String getOwner() {
		return owner;
	}
	
	/** Sets the owner, or <code>null</code>. */
	public void setOwner(String owner) {
		this.owner = owner;
	}

	/** Retrieves the comments. */
	public String getComments() {
		return comments;
	}

	/** Sets the comments, or <code>null</code>. */
	public void setComments(String comments) {
		this.comments = comments;
	}
	
	/**
	 * Returns the creation date (in UTC time) or <code>null</code>
	 * if the attribute is not present.
	 * 
	 * @return the UTC creation date of <code>null</code>.
	 */
	public Date getDate() {
		
		if (capDate == null || 
				Float.isInfinite(utcOffset) || Float.isNaN(utcOffset)) {
			return null;
		}
		
		try {
			SimpleDateFormat formatter = new SimpleDateFormat(DATE_FORMAT);
			formatter.setTimeZone(UTC);
			Date date = formatter.parse(capDate);
			date.setTime(date.getTime() + (long)(utcOffset*1000.0f));
			
			return date;
			
		} catch (ParseException e) {
			return null;
		}
		
	}
	
	/**
	 * Package-only function to set the capture date fields directly.
	 * This method validates the fields and if the input is invalid it
	 * just sets the fields to null/NaN
	 * 
	 * @param dateStr local time in the format "yyyy:MM:dd HH:mm:ss"
	 * @param offset seconds from the the local time to UTC time
	 *               such that <code>UTC == local time + utcOffset</code>.
	 */
	protected void setCapDate(String dateStr, float offset) {
		
		if (dateStr == null || dateStr.length() == 0 || 
				Float.isInfinite(offset) || Float.isNaN(offset)
				) {
			capDate   = null;
			utcOffset = Float.NaN;
		}
		
		try {
			SimpleDateFormat formatter = new SimpleDateFormat(DATE_FORMAT);
			Date date = formatter.parse(dateStr);
			date.setTime(date.getTime() + (long)(offset*1000.0f));
			
			capDate   = dateStr;
			utcOffset = offset;
			
		} catch (ParseException e) {
			capDate   = null;
			utcOffset = Float.NaN;
		}		
	}
	
	/**
	 * Returns a string representation of the attributes. Note that
	 * this method reconstructs the string representation every time it
	 * is called, thus it is slow to use it repeatedly.
	 * 
	 * @return the string representation of the attributes.
	 */
	public String toString() {
		
		StringBuilder sb = new StringBuilder();
		sb.append('[');
		boolean needsSeparator = false;
		if (owner != null && owner.length() > 0) {
			sb.append("owner: ");
			sb.append(owner);
			needsSeparator = true;
		}
		Date date = getDate();
		if (date != null) {
			if(needsSeparator) {
				sb.append(", ");
			}
			sb.append("date: ");
			sb.append(date);
			needsSeparator = true;
		}
		if (comments != null && comments.length() > 0) {
			if(needsSeparator) {
				sb.append(", ");
			}
			sb.append("comments: ");
			sb.append(comments);
		}
		
		sb.append(']');
		return sb.toString();
	}
	
}