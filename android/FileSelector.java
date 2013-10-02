/*
 * FileSelector.java - RECOIL for Android
 *
 * Copyright (C) 2013  Piotr Fusik and Adrian Matoga
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
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;
import java.io.IOException;
import java.util.ArrayList;
import java.util.TreeSet;

public class FileSelector extends ListActivity
{
	private Uri uri;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		getListView().setTextFilterEnabled(true);
		uri = getIntent().getData();
		if (uri == null)
			uri = Uri.parse("file:///");

		ArrayList<String> files;
		TreeSet<String> directories = new TreeSet<String>();
		try {
			files = FileUtil.list(uri, directories);
			files.addAll(0, directories);
		}
		catch (IOException ex) {
			Toast.makeText(this, R.string.access_denied, Toast.LENGTH_SHORT).show();
			files = new ArrayList<String>();
		}

		setListAdapter(new ArrayAdapter<String>(this, R.layout.filename_list_item, files));
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id)
	{
		String name = (String) l.getItemAtPosition(position);
		Class klass = RECOIL.isOurFile(name) ? Viewer.class : FileSelector.class;
		Intent intent = new Intent(Intent.ACTION_VIEW, FileUtil.buildUri(uri, name), this, klass);
		startActivity(intent);
	}
}
