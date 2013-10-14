/*
 * GalleryAdapter.java - RECOIL for Android
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

import android.graphics.Bitmap;
import android.net.Uri;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.zip.ZipFile;

class GalleryAdapter extends BaseAdapter
{
	private final Viewer viewer;
	private final Uri baseUri;
	private final ArrayList<String> files;

	GalleryAdapter(Viewer viewer, Uri baseUri, ArrayList<String> files)
	{
		this.viewer = viewer;
		this.baseUri = baseUri;
		this.files = files;
	}

	/**
	 * Reads bytes from the stream into the byte array
	 * until end of stream or array is full.
	 * @param is source stream
	 * @param b output array
	 * @return number of bytes read
	 */
	private static int readAndClose(InputStream is, byte[] b) throws IOException
	{
		int got = 0;
		int len = b.length;
		try {
			while (got < len) {
				int i = is.read(b, got, len - got);
				if (i <= 0)
					break;
				got += i;
			}
		}
		finally {
			is.close();
		}
		return got;
	}

	public View getView(int position, View convertView, ViewGroup parent)
	{
		Uri uri = FileUtil.buildUri(baseUri, files.get(position));

		// Read file
		byte[] content = new byte[RECOIL.MAX_CONTENT_LENGTH];
		int contentLength;
		String filename = uri.getPath();
		try {
			if (FileUtil.isZip(filename)) {
				ZipFile zip = new ZipFile(filename);
				try {
					filename = uri.getFragment();
					InputStream is = zip.getInputStream(zip.getEntry(filename));
					contentLength = readAndClose(is, content);
				}
				finally {
					zip.close();
				}
			}
			else {
				InputStream is = new FileInputStream(filename);
				contentLength = readAndClose(is, content);
			}
		}
		catch (IOException ex) {
			viewer.showError(R.string.error_reading_file);
			return null;
		}

		// Decode
		RECOIL recoil = new RECOIL();
		if (!recoil.decode(filename, content, contentLength)) {
			viewer.showError(R.string.invalid_file);
			return null;
		}
		int[] pixels = recoil.getPixels();
		int width = recoil.getWidth();
		int height = recoil.getHeight();

		// Set alpha
		int pixelsLength = width * height;
		for (int i = 0; i < pixelsLength; i++)
			pixels[i] |= 0xff000000;

		// Display
		Bitmap bitmap = Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888);
		bitmap.setHasAlpha(false);
		ImageView imageView = convertView instanceof ImageView ? (ImageView) convertView : new ImageView(viewer);
		imageView.setImageBitmap(bitmap);
		return imageView;
	}

	public long getItemId(int position)
	{
		return position;
	}

	public Object getItem(int position)
	{
		return position;
	}

	public int getCount()
	{
		return files.size();
	}
}
