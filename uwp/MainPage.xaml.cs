/*
 * MainPage.xaml.cs - Universal Windows application
 *
 * Copyright (C) 2014-2016  Piotr Fusik
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

using System;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.Storage.Streams;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;

namespace RECOIL
{
	public sealed partial class MainPage : Page
	{
		public MainPage()
		{
			InitializeComponent();
		}

		public async Task ShowFile(StorageFile file)
		{
			if (file == null)
				return;

			// read
			byte[] content;
			try {
				ulong length = (await file.GetBasicPropertiesAsync()).Size;
				if (length > Recoil.RECOIL.MaxContentLength) {
					await new MessageDialog("File too long").ShowAsync();
					return;
				}
				IBuffer buffer = await FileIO.ReadBufferAsync(file);
				content = buffer.ToArray();
			}
			catch (Exception) {
				await new MessageDialog("Error reading file").ShowAsync();
				return;
			}

			// decode
			Recoil.RECOIL recoil = new Recoil.RECOIL();
			if (!recoil.Decode(file.Name, content, content.Length)) {
				await new MessageDialog("Decoding error").ShowAsync();
				return;
			}
			int width = recoil.GetWidth();
			int height = recoil.GetHeight();
			int[] pixels = recoil.GetPixels();

			// convert to BGRA
			WriteableBitmap bitmap = new WriteableBitmap(width, height);
			byte[] line = new byte[width << 2];
			for (int x = 0; x < width; x++)
				line[(x << 2) + 3] = 0xff; // alpha
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int rgb = pixels[y * width + x];
					line[x << 2] = (byte) rgb;
					line[(x << 2) + 1] = (byte) (rgb >> 8);
					line[(x << 2) + 2] = (byte) (rgb >> 16);
				}
				line.CopyTo(0, bitmap.PixelBuffer, (uint) (y * width << 2), width << 2);
			}

			// display
			Image.Source = bitmap;
		}

		async void OpenFile(object sender, RoutedEventArgs e)
		{
			StorageFile file = await FilePicker.PickFile();
			await ShowFile(file);
		}
	}
}
