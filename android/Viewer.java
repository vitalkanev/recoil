/*
 * Viewer.java - RECOIL for Android
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

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Gallery;
import android.widget.Toast;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

public class Viewer extends Activity implements AdapterView.OnItemSelectedListener
{
	private Uri baseUri;
	private ArrayList<String> filenames;

	private String split(Uri uri)
	{
		String filename = uri.getFragment();
		if (filename == null)
			filename = uri.getPath();

		int i = filename.lastIndexOf('/') + 1;
		// nice hack - the following substrings do what we want if there is no slash
		String path = filename.substring(0, i);
		filename = filename.substring(i);

		if (uri.getFragment() == null)
			baseUri = Uri.fromFile(new File(path));
		else
			baseUri = uri.buildUpon().fragment(path).build();
		return filename;
	}

	int getFileCount()
	{
		return filenames.size();
	}

	Uri getUri(int position)
	{
		return FileUtil.buildUri(baseUri, filenames.get(position));
	}

	public void onItemSelected(AdapterView<?> parent, View view, int position, long id)
	{
		setTitle(getString(R.string.viewing_title, filenames.get(position)));
	}

	public void onNothingSelected(AdapterView<?> parent)
	{
		// can we ever get here?
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		Uri uri = getIntent().getData();
		String filename = split(uri);
		try {
			filenames = FileUtil.list(baseUri, null);
		}
		catch (IOException ex) {
			Toast.makeText(this, R.string.error_listing_files, Toast.LENGTH_SHORT).show();
			return;
		}

		Gallery gallery = (Gallery) getLayoutInflater().inflate(R.layout.gallery, null);
		gallery.setHorizontalFadingEdgeEnabled(false);
		gallery.setAdapter(new GalleryAdapter(this));
		gallery.setOnItemSelectedListener(this);
		gallery.setSelection(filenames.indexOf(filename));
		setContentView(gallery);
	}
}
