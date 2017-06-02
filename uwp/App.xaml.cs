/*
 * App.xaml.cs - Universal Windows application
 *
 * Copyright (C) 2014-2017  Piotr Fusik
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

using Windows.ApplicationModel.Activation;
using Windows.Storage;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace RECOIL
{
	sealed partial class App : Application
	{
		public App()
		{
			InitializeComponent();
		}

		static Frame Launch(bool activate)
		{
			Frame frame = Window.Current.Content as Frame;
			if (frame == null) {
				frame = new Frame();
				Window.Current.Content = frame;
			}

			if (activate) {
				if (frame.Content == null)
					frame.Navigate(typeof(MainPage));
				Window.Current.Activate();
			}
			return frame;
		}

		protected override void OnLaunched(LaunchActivatedEventArgs e)
		{
			Launch(!e.PrelaunchActivated);
		}

		protected override async void OnFileActivated(FileActivatedEventArgs args)
		{
			MainPage page = Launch(true).Content as MainPage;
			if (page != null) {
				StorageFile file = args.Files[0] as StorageFile;
				await page.ShowFile(file);
				await page.SetNeighboring(args.NeighboringFilesQuery, file);
			}
		}
	}
}
