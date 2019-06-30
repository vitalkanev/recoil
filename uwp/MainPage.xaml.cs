/*
 * MainPage.xaml.cs - Universal Windows application
 *
 * Copyright (C) 2014-2019  Piotr Fusik
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
using System.Collections.Generic;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.ApplicationModel.DataTransfer;
using Windows.Graphics.Imaging;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Search;
using Windows.Storage.Streams;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;

namespace RECOIL
{
	public sealed partial class MainPage : Page
	{
		IReadOnlyList<StorageFile> Files;
		int Index;

		public MainPage()
		{
			InitializeComponent();
			DataTransferManager.GetForCurrentView().DataRequested += OnDataRequested;
		}

		const string Disclaimer = "\nAre you opening a graphics file originating from a 20th century computer? " +
			"This app only supports such files.";

		public async Task ShowFile(StorageFile file)
		{
			// read
			byte[] content;
			try {
				ulong length = (await file.GetBasicPropertiesAsync()).Size;
				if (length > Recoil.RECOIL.MaxContentLength) {
					await new MessageDialog("File too long." + Disclaimer).ShowAsync();
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
				await new MessageDialog("Decoding error." + Disclaimer).ShowAsync();
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
			FileName.Text = file.Name;
			SaveAsButton.Visibility = Visibility.Visible;
			ShareButton.Visibility = Visibility.Visible;
			CopyButton.Visibility = Visibility.Visible;
		}

		void SetIndex(int index)
		{
			Index = index;
			PreviousButton.IsEnabled = index > 0;
			NextButton.IsEnabled = index + 1 < Files.Count;
		}

		public async Task ShowFiles(IReadOnlyList<StorageFile> files)
		{
			if (files.Count > 0) {
				await ShowFile(files[0]);
				if (files.Count > 1) {
					Files = files;
					SetIndex(0);
					PreviousButton.Visibility = Visibility.Visible;
					NextButton.Visibility = Visibility.Visible;
				}
				else {
					Files = null;
					PreviousButton.Visibility = Visibility.Collapsed;
					NextButton.Visibility = Visibility.Collapsed;
				}
			}
		}

		async void OpenFile(object sender, RoutedEventArgs e)
		{
			await ShowFiles(await FilePicker.PickFiles());
		}

		async void OpenPrevious(object sender, RoutedEventArgs e)
		{
			if (Files != null && Index > 0) {
				SetIndex(Index - 1);
				await ShowFile(Files[Index]);
			}
		}

		async void OpenNext(object sender, RoutedEventArgs e)
		{
			if (Files != null && Index + 1 < Files.Count) {
				SetIndex(Index + 1);
				await ShowFile(Files[Index]);
			}
		}

		async Task SaveTo(IRandomAccessStream stream)
		{
			BitmapEncoder encoder = await BitmapEncoder.CreateAsync(BitmapEncoder.PngEncoderId, stream);
			WriteableBitmap bitmap = (WriteableBitmap) Image.Source;
			encoder.SetPixelData(BitmapPixelFormat.Bgra8, BitmapAlphaMode.Ignore,
				(uint) bitmap.PixelWidth, (uint) bitmap.PixelHeight, 96, 96, bitmap.PixelBuffer.ToArray());
			await encoder.FlushAsync();
		}

		async void SaveAs(object sender, RoutedEventArgs e)
		{
			FileSavePicker picker = new FileSavePicker {
				SuggestedStartLocation = PickerLocationId.PicturesLibrary,
				SuggestedFileName = FileName.Text,
				FileTypeChoices = { { "PNG Image", new string[] { ".png" } } } };
			StorageFile file = await picker.PickSaveFileAsync();
			if (file != null) {
				try {
					using (IRandomAccessStream stream = await file.OpenAsync(FileAccessMode.ReadWrite)) {
						await SaveTo(stream);
					}
				}
				catch (Exception) {
					await new MessageDialog("Error saving file").ShowAsync();
				}
			}
		}

		async Task PutBitmap(DataPackage package)
		{
			InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
			await SaveTo(stream);
			package.SetBitmap(RandomAccessStreamReference.CreateFromStream(stream));
		}

		async void Copy(object sender, RoutedEventArgs e)
		{
			DataPackage package = new DataPackage();
			await PutBitmap(package);
			try {
				Clipboard.SetContent(package);
			}
			catch (Exception) {
				await new MessageDialog("Clipboard error").ShowAsync();
			}
		}

		async void OnDataRequested(DataTransferManager sender, DataRequestedEventArgs e)
		{
			if (Image.Source == null)
				return; // do we need this?
			DataRequestDeferral deferral = e.Request.GetDeferral();
			DataPackage package = e.Request.Data;
			package.Properties.Title = FileName.Text;
			await PutBitmap(package);
			deferral.Complete();
		}

		void Share(object sender, RoutedEventArgs e)
		{
			DataTransferManager.ShowShareUI();
		}
	}
}
