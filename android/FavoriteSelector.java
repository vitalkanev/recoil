/*
 * FavoriteSelector.java - RECOIL for Android
 *
 * Copyright (C) 2015  Piotr Fusik
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

import android.app.ListActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;

interface FavoriteFile
{
	File getFile();
}

public class FavoriteSelector extends ListActivity
{
	private class RootFavoriteFile implements FavoriteFile
	{
		@Override
		public String toString()
		{
			return getString(R.string.root_directory);
		}

		public File getFile()
		{
			return new File("/");
		}
	}

	private class DownloadsFavoriteFile implements FavoriteFile
	{
		private final File directory;

		DownloadsFavoriteFile() throws ReflectiveOperationException
		{
			Object directoryDownloads = Environment.class.getField("DIRECTORY_DOWNLOADS").get(null);
			Method method = Environment.class.getMethod("getExternalStoragePublicDirectory", String.class);
			directory = (File) method.invoke(null, directoryDownloads);
		}

		@Override
		public String toString()
		{
			return getString(R.string.downloads_directory);
		}

		public File getFile()
		{
			return directory;
		}
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		getListView().setTextFilterEnabled(true);

		ArrayList<FavoriteFile> favorites = new ArrayList<FavoriteFile>();
		favorites.add(new RootFavoriteFile());
		try {
			favorites.add(new DownloadsFavoriteFile());
		}
		catch (ReflectiveOperationException ex) {
		}
		setListAdapter(new ArrayAdapter<FavoriteFile>(this, R.layout.filename_list_item, favorites));
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id)
	{
		FavoriteFile favorite = (FavoriteFile) l.getItemAtPosition(position);
		Intent intent = new Intent(Intent.ACTION_VIEW, Uri.fromFile(favorite.getFile()), this, FileSelector.class);
		startActivity(intent);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		getMenuInflater().inflate(R.menu.favorite_selector, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch (item.getItemId()) {
		case R.id.menu_about:
			About.show(this);
			return true;
		default:
			return false;
		}
	}
}
