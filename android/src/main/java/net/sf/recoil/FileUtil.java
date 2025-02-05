/*
 * FileUtil.java - RECOIL for Android
 *
 * Copyright (C) 2013-2018  Piotr Fusik
 *
 * This file is part of RECOIL (Retro Computer Image Library),
 * see http://recoil.sourceforge.net
 *
 * RECOIL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * RECOIL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RECOIL; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

package net.sf.recoil;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.view.MenuItem;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.Set;
import java.util.TreeSet;
import java.util.zip.ZipFile;
import java.util.zip.ZipEntry;

abstract class FileUtil
{
	static String getDisplayName(Uri uri)
	{
		String displayName = uri.getPath();
		String fragment = uri.getFragment();
		if (fragment != null)
			displayName += "#" + fragment;
		return displayName;
	}

	static Uri getInternalStorage()
	{
		return Uri.fromFile(Environment.getExternalStorageDirectory());
	}

	private static String getSdCardPath(Context context)
	{
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
			File[] dirs = context.getExternalFilesDirs(null);
			if (dirs.length > 1 && dirs[1] != null) {
				String path = dirs[1].getPath();
				String externalDir = "/Android/data/net.sf.recoil/files";
				if (path.endsWith(externalDir))
					return path.substring(0, path.length() - externalDir.length());
			}
		}
		return "/storage/sdcard1";
	}

	static Uri getSdCard(Context context)
	{
		String path = getSdCardPath(context);
		File file = new File(path);
		if (file.isDirectory())
			return Uri.fromFile(file);
		return null;
	}

	static boolean isZip(String filename)
	{
		int n = filename.length();
		return n >= 4 && filename.regionMatches(true, n - 4, ".zip", 0, 4);
	}

	private static String getZipPath(String path)
	{
		if (path == null || path.length() == 0)
			return "";
		if (path.endsWith("/"))
			return path;
		return path + '/';
	}

	private static void listDirectory(File dir, TreeSet<String> directories, ArrayList<String> files) throws IOException
	{
		File[] entries = dir.listFiles();
		if (entries == null)
			throw new FileNotFoundException();
		for (File entry : entries) {
			String name = entry.getName();
			if (entry.isDirectory()) {
				if (directories != null)
					directories.add(name);
			}
			else if (RECOIL.isOurFile(name))
				files.add(name);
			else if (directories != null && isZip(name))
				files.add(name);
		}
	}

	private static void listZipDirectory(File zipFile, String zipPath, TreeSet<String> directories, ArrayList<String> files) throws IOException
	{
		zipPath = getZipPath(zipPath);
		int zipPathLen = zipPath.length();
		try (ZipFile zip = new ZipFile(zipFile)) {
			Enumeration<? extends ZipEntry> entries = zip.entries();
			while (entries.hasMoreElements()) {
				ZipEntry entry = entries.nextElement();
				if (!entry.isDirectory()) {
					String name = entry.getName();
					if (name.startsWith(zipPath) && RECOIL.isOurFile(name)) {
						int i = name.indexOf('/', zipPathLen);
						if (i >= 0) {
							// file in a subdirectory
							if (directories != null) {
								// add subdirectory
								directories.add(name.substring(zipPathLen, i));
							}
						}
						else
							files.add(name.substring(zipPathLen));
					}
				}
			}
		}
	}

	static Comparator<? super String> getComparator()
	{
		return Collator.getInstance();
	}

	static ArrayList<String> list(Uri uri, TreeSet<String> directories) throws IOException
	{
		ArrayList<String> files = new ArrayList<String>();
		File file = new File(uri.getPath());
		if (file.isDirectory())
			listDirectory(file, directories, files);
		else
			listZipDirectory(file, uri.getFragment(), directories, files);
		Collections.sort(files, getComparator());
		return files;
	}

	static Uri buildUri(Uri baseUri, String relativePath)
	{
		if (isZip(baseUri.getPath())) {
			String zipPath = baseUri.getFragment();
			zipPath = getZipPath(zipPath) + relativePath;
			return baseUri.buildUpon().fragment(zipPath).build();
		}
		return Uri.withAppendedPath(baseUri, Uri.encode(relativePath));
	}

	static Set<String> getUserFavorites(Context context)
	{
		// as per reference, must not modify the set instance returned by getStringSet
		return PreferenceManager.getDefaultSharedPreferences(context).getStringSet("favorites", Collections.EMPTY_SET);
	}

	static void setFavoriteIcon(MenuItem item, boolean checked)
	{
		item.setIcon(checked ? android.R.drawable.btn_star_big_on : android.R.drawable.btn_star_big_off);
	}

	static boolean toggleFavorite(Context context, TreeSet<String> favorites, Uri uri)
	{
		String s = uri.toString();
		boolean added = favorites.add(s);
		if (!added)
			favorites.remove(s);
		SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(context).edit();
		editor.putStringSet("favorites", favorites);
		editor.apply();
		return added;
	}
}
