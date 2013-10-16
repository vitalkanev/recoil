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
import android.widget.Gallery;
import android.widget.Toast;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

public class Viewer extends Activity
{
	private String filename;

	private String getParent(String path)
	{
		int i = path.lastIndexOf('/');
		if (i < 0) {
			filename = path;
			return "";
		}
		filename = path.substring(i + 1);
		return path.substring(0, i + 1);
	}

	private Uri getParent(Uri uri)
	{
		String path = uri.getFragment();
		if (path != null)
			return uri.buildUpon().fragment(getParent(path)).build();
		return Uri.fromFile(new File(getParent(uri.getPath())));
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setTitle(R.string.viewing_title);

		Uri uri = getIntent().getData();
		Uri baseUri = getParent(uri);
		ArrayList<String> files;
		try {
			files = FileUtil.list(baseUri, null);
		}
		catch (IOException ex) {
			Toast.makeText(this, R.string.error_listing_files, Toast.LENGTH_SHORT).show();
			return;
		}

		Gallery gallery = new Gallery(this);
		gallery.setAdapter(new GalleryAdapter(this, baseUri, files));
		gallery.setSelection(files.indexOf(filename));
		setContentView(gallery);
	}
}
