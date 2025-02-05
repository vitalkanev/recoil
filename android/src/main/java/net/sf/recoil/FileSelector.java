/*
 * FileSelector.java - RECOIL for Android
 *
 * Copyright (C) 2013-2020  Piotr Fusik
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

import android.Manifest;
import android.app.ListActivity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Filterable;
import android.widget.ListView;
import android.widget.SearchView;
import android.widget.TextView;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.TreeSet;

public class FileSelector extends ListActivity
{
	private Uri uri;
	private int directoryCount;
	private boolean isSearch;

	private boolean isDirectoryOrZip(int position)
	{
		if (position < directoryCount)
			return true;
		String name = (String) getListAdapter().getItem(position);
		return FileUtil.isZip(name);
	}

	private class FileAdapter extends ArrayAdapter<String>
	{
		private FileAdapter(Collection<String> directories, Collection<String> files)
		{
			super(FileSelector.this, R.layout.filename_list_item);
			addAll(directories);
			addAll(files);
		}

		public View getView(int position, View convertView, ViewGroup parent)
		{
			TextView view = (TextView) super.getView(position, convertView, parent);
			int icon = isDirectoryOrZip(position) ? R.drawable.ic_folder : R.drawable.ic_image;
			view.setCompoundDrawablesWithIntrinsicBounds(icon, 0, 0, 0);
			return view;
		}
	}

	private void list()
	{
		ArrayList<String> files;
		TreeSet<String> directories = new TreeSet<String>(FileUtil.getComparator());
		int emptyViewId;
		try {
			files = FileUtil.list(uri, directories);
			emptyViewId = R.layout.no_files;
		}
		catch (IOException ex) {
			files = new ArrayList<String>();
			directories.clear();
			emptyViewId = R.layout.access_denied;
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
				String permission = Manifest.permission.READ_EXTERNAL_STORAGE;
				if (checkSelfPermission(permission) == PackageManager.PERMISSION_DENIED)
					requestPermissions(new String[] { permission }, 1);
			}
		}
		View emptyView = getLayoutInflater().inflate(emptyViewId, null);
		ListView listView = getListView();
		((ViewGroup) listView.getParent()).addView(emptyView);
		listView.setEmptyView(emptyView);
		directoryCount = directories.size();
		setListAdapter(new FileAdapter(directories, files));
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		uri = getIntent().getData();
		if (uri == null)
			uri = FileUtil.getInternalStorage();
		getActionBar().setDisplayHomeAsUpEnabled(true);
		setTitle(getString(R.string.selector_title, FileUtil.getDisplayName(uri)));
		list();
	}

	@Override
	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
	{
		super.onRequestPermissionsResult(requestCode, permissions, grantResults);
		if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
			View accessDenied = getListView().getEmptyView();
			if (accessDenied != null)
				((ViewGroup) accessDenied.getParent()).removeView(accessDenied);
			list();
		}
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id)
	{
		String name = (String) l.getItemAtPosition(position);
		Class klass = isDirectoryOrZip(position) ? FileSelector.class : Viewer.class;
		Intent intent = new Intent(Intent.ACTION_VIEW, FileUtil.buildUri(uri, name), this, klass);
		startActivity(intent);
	}

	private boolean isBuiltinFavorite()
	{
		return uri.equals(FileUtil.getInternalStorage())
			|| uri.equals(FileUtil.getSdCard(this));
	}

	private boolean isUserFavorite()
	{
		return FileUtil.getUserFavorites(this).contains(uri.toString());
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		getMenuInflater().inflate(R.menu.file_selector, menu);

		SearchView searchView = (SearchView) menu.findItem(R.id.menu_search).getActionView();
		searchView.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
				public boolean onQueryTextChange(String newText) {
					((Filterable) getListAdapter()).getFilter().filter(newText);
					return true;
				}
				public boolean onQueryTextSubmit(String newText) {
					return onQueryTextChange(newText);
				}
			});

		MenuItem favoriteMenuItem = menu.findItem(R.id.menu_favorite);
		if (isBuiltinFavorite())
			favoriteMenuItem.setVisible(false);
		else
			FileUtil.setFavoriteIcon(favoriteMenuItem, isUserFavorite());
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch (item.getItemId()) {
		case android.R.id.home:
			finish();
			return true;
		case R.id.menu_favorite:
			FileUtil.setFavoriteIcon(item, FileUtil.toggleFavorite(this, new TreeSet(FileUtil.getUserFavorites(this)), uri));
			return true;
		default:
			return false;
		}
	}
}
