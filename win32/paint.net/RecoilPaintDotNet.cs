/*
 * RecoilPaintDotNet.cs - Paint.NET file type plugin
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

using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;

using PaintDotNet;
using Recoil;

namespace Recoil.PaintDotNet
{
	// Paint.NET gives us a Stream without the filename, so I create a distinct FileType object for each extension.
	// This could be optimized by considering alias extensions.

	class RecoilFileType : FileType
	{
		public RecoilFileType(string ext, string name) : base(name, FileTypeFlags.SupportsLoading, new string[] { ext })
		{
		}

		protected override Document OnLoad(Stream input)
		{
			// Read.
			byte[] content = new byte[RECOIL.MaxContentLength];
			int contentLength = input.Read(content, 0, content.Length);
			RECOIL recoil = new RECOIL();

			// Decode.
			if (!recoil.Decode(DefaultExtension, content, contentLength))
				throw new Exception("Decoding error");
			int width = recoil.GetWidth();

			// Pass to Paint.NET.
			GCHandle pinnedPixels = GCHandle.Alloc(recoil.GetPixels(), GCHandleType.Pinned);
			using (Bitmap bitmap = new Bitmap(width, recoil.GetHeight(), width << 2, PixelFormat.Format32bppRgb, pinnedPixels.AddrOfPinnedObject())) {
				pinnedPixels.Free();
				return Document.FromImage(bitmap);
			}
		}
	}

	public class RecoilFileTypeFactory : IFileTypeFactory
	{
		public FileType[] GetFileTypeInstances()
		{
			return new FileType[] {
				new RecoilFileType(".256", "80x96, 256 colors"),
				new RecoilFileType(".4mi", "AtariTools-800 4 mono missiles"),
				new RecoilFileType(".4pl", "AtariTools-800 4 mono players"),
				new RecoilFileType(".4pm", "AtariTools-800 4 mono players and 4 mono missiles"),
				new RecoilFileType(".a4r", "Anime 4ever; 80x256, 16-level grayscale, compressed"),
				new RecoilFileType(".acbm", "Amiga Continuous Bitmap"),
				new RecoilFileType(".acs", "AtariTools-800 4x8 font, 4 colors"),
				new RecoilFileType(".agp", "AtariTools-800 graphic"),
				new RecoilFileType(".all", "Graph; 160x192, 5 colors"),
				new RecoilFileType(".ap2", "80x96, 256 colors"),
				new RecoilFileType(".ap3", "80x192, 256 colors, 2 frames"),
				new RecoilFileType(".apc", "Any Point, Any Color; 80x96, 256 colors, 2 frames"),
				new RecoilFileType(".apl", "Atari Player Editor; up to 16 16x48 frames, 4 colors"),
				new RecoilFileType(".app", "80x192, 256 colors, 2 frames, compressed"),
				new RecoilFileType(".apv", "80x192, 256 colors, 2 frames"),
				new RecoilFileType(".art", "Art Studio or Art Director or Ascii-Art Editor or Artist by David Eaton"),
				new RecoilFileType(".atr", "ZX Spectrum attributes"),
				new RecoilFileType(".bg9", "160x192, 16-level grayscale"),
				new RecoilFileType(".bkg", "Movie Maker background; 160x96, 4 colors"),
				new RecoilFileType(".bl1", "DEGAS Elite block; 16 colors"),
				new RecoilFileType(".bl2", "DEGAS Elite block; 4 colors"),
				new RecoilFileType(".bl3", "DEGAS Elite block; mono"),
				new RecoilFileType(".bru", "DEGAS Elite brush; 8x8, mono"),
				new RecoilFileType(".ca1", "CrackArt; 320x200, 16 colors, compressed"),
				new RecoilFileType(".ca2", "CrackArt; 640x200, 4 colors, compressed"),
				new RecoilFileType(".ca3", "CrackArt; 640x400, mono, compressed"),
				new RecoilFileType(".cci", "Champions' Interlace; 160x192, 2 frames, compressed"),
				new RecoilFileType(".cdu", "CDU-Paint; 160x200, 16 colors"),
				new RecoilFileType(".ch4", "4x8 font; mono"),
				new RecoilFileType(".ch6", "6x8 font; mono"),
				new RecoilFileType(".ch8", "8x8 font; mono"),
				new RecoilFileType(".che", "Cheese; 160x200, 16 colors"),
				new RecoilFileType(".chr", "Blazing Paddles font; mono"),
				new RecoilFileType(".cin", "Champions' Interlace; 160x192 or 160x200, 2 frames"),
				new RecoilFileType(".cpr", "Trzmiel; 320x192, mono, compressed"),
				new RecoilFileType(".cpt", "Canvas; compressed"),
				new RecoilFileType(".cwg", "Create with Garfield; 160x200, 16 colors"),
				new RecoilFileType(".dc1", "DuneGraph; 320x200, 256 colors, compressed"),
				new RecoilFileType(".dd", "Doodle; 320x200, 16 colors"),
				new RecoilFileType(".del", "DelmPaint; 320x240, 256 colors, compressed"),
				new RecoilFileType(".dg1", "DuneGraph; 320x200, 256 colors"),
				new RecoilFileType(".dgc", "DuneGraph; 320x200, 256 colors, compressed"),
				new RecoilFileType(".dgp", "DigiPaint; 80x192, 256 colors, 2 frames"),
				new RecoilFileType(".dgu", "DuneGraph; 320x200, 256 colors"),
				new RecoilFileType(".din", "320x192, 10 colors, 2 frames"),
				new RecoilFileType(".dlm", "Dir Logo Maker; 88x128, mono"),
				new RecoilFileType(".dol", "Dolphin Ed; 160x200, 16 colors"),
				new RecoilFileType(".doo", "Doodle; 640x400, mono"),
				new RecoilFileType(".dph", "DelmPaint; 640x480, 256 colors, compressed"),
				new RecoilFileType(".drg", "Atari CAD; 320x160, mono"),
				new RecoilFileType(".drz", "Drazpaint; 160x200, 16 colors"),
				new RecoilFileType(".esc", "EscalPaint; 80x192, 256 colors, 2 frames"),
				new RecoilFileType(".fgs", "Fun Graphics Machine; 320x200, mono"),
				new RecoilFileType(".fnt", "Standard 8x8 font, mono"),
				new RecoilFileType(".fpt", "Face Painter; 160x200, 16 colors"),
				new RecoilFileType(".ftc", "Falcon True Color; 384x240, 65536 colors"),
				new RecoilFileType(".fwa", "Fun with Art; 160x192, 128 colors"),
				new RecoilFileType(".g09", "160x192, 16-level grayscale"),
				new RecoilFileType(".g10", "Graphics 10; up to 80x240, 9 colors"),
				new RecoilFileType(".g11", "Graphics 11; up to 80x240, 16 colors"),
				new RecoilFileType(".gcd", "Gigacad; 320x200, mono"),
				new RecoilFileType(".gfb", "DeskPic"),
				new RecoilFileType(".ghg", "Gephard Hires Graphics; up to 320x200, mono"),
				new RecoilFileType(".gig", "Gigapaint; 160x200, 16 colors"),
				new RecoilFileType(".gih", "Gigapaint; 320x200, mono"),
				new RecoilFileType(".god", "GodPaint; 65536 colors"),
				new RecoilFileType(".gr7", "Graphics 7; up to 160x120, 4 colors"),
				new RecoilFileType(".gr8", "Graphics 8; up to 320x240, mono"),
				new RecoilFileType(".gr9", "Graphics 9; up to 80x240, 16-level grayscale"),
				new RecoilFileType(".hbm", "Hires Bitmap; 320x200, mono"),
				new RecoilFileType(".hed", "Hi-Eddi; 320x200, 16 colors"),
				new RecoilFileType(".hip", "Hard Interlace Picture; 160x200, grayscale, 2 frames"),
				new RecoilFileType(".hir", "Hires; 320x200, mono"),
				new RecoilFileType(".hlr", "ZX Spectrum attributes gigascreen; 256x192, 2 frames"),
				new RecoilFileType(".hpm", "Grass' Slideshow; 160x192, 4 colors, compressed"),
				new RecoilFileType(".hr", "256x239, 3 colors, 2 frames"),
				new RecoilFileType(".hr2", "320x200, 5 colors, 2 frames"),
				new RecoilFileType(".ice", "Interlace Character Editor font, 2 frames"),
				new RecoilFileType(".icn", "DEGAS Elite Icon or ICE CIN"),
				new RecoilFileType(".ifl", "ZX Spectrum Multicolor 8x2; 256x192, 15 colors"),
				new RecoilFileType(".iff", "Interchange File Format"),
				new RecoilFileType(".ige", "Interlace Graphics Editor; 128x96, 16 colors, 2 frames"),
				new RecoilFileType(".ilc", "80x192, 256 colors, 2 frames"),
				new RecoilFileType(".img", "GEM Bit Image or ZX Spectrum Gigascreen"),
				new RecoilFileType(".imn", "ICE MIN; 160x192, 80 colors, 2 frames"),
				new RecoilFileType(".ing", "ING 15; 160x200, 7 colors, 2 frames"),
				new RecoilFileType(".inp", "160x200, 7 colors, 2 frames"),
				new RecoilFileType(".int", "INT95a; up to 160x239, 16 colors, 2 frames"),
				new RecoilFileType(".ip2", "ICE PCIN+; 160x192, 45 colors, 2 frames"),
				new RecoilFileType(".ipc", "ICE PCIN; 160x192, 35 colors, 2 frames"),
				new RecoilFileType(".iph", "Interpaint; 320x200, 16 colors"),
				new RecoilFileType(".ipt", "Interpaint; 160x200, 16 colors"),
				new RecoilFileType(".ir2", "Super IRG 2; 160x192, 25 colors, 2 frames"),
				new RecoilFileType(".irg", "Super IRG; 160x192, 15 colors, 2 frames"),
				new RecoilFileType(".ism", "Image System; 160x200, 16 colors"),
				new RecoilFileType(".ist", "Interlace Studio; 160x200, 2 frames"),
				new RecoilFileType(".jgp", "Jet Graphics Planner; 8x16 tiles, 4 colors"),
				new RecoilFileType(".koa", "Koala Painter; 160x200, 16 colors"),
				new RecoilFileType(".lbm", "Interleaved Bitmap"),
				new RecoilFileType(".leo", "Larka Edytor Obiektow trybu $4+; 8x16 tiles, 5 colors"),
				new RecoilFileType(".max", "XL-Paint MAX; 160x192, 2 frames, compressed"),
				new RecoilFileType(".mbg", "Mad Designer; 512x256, mono"),
				new RecoilFileType(".mc", "Multicolor 8x1; 256x192, 15 colors"),
				new RecoilFileType(".mch", "Graph2Font; up to 176x240, 128 colors"),
				new RecoilFileType(".mcp", "McPainter; 160x200, 16 colors, 2 frames"),
				new RecoilFileType(".mcs", "160x192, 9 colors"),
				new RecoilFileType(".mg1", "MultiArtist; 256x192, 2 frames"),
				new RecoilFileType(".mg2", "MultiArtist; 256x192, 2 frames"),
				new RecoilFileType(".mg4", "MultiArtist; 256x192, 2 frames"),
				new RecoilFileType(".mg8", "MultiArtist; 256x192, 2 frames"),
				new RecoilFileType(".mgp", "Magic Painter; 160x96, 4 colors with optional rainbow effect"),
				new RecoilFileType(".mic", "Micro Illustrator; up to 160x240, 4 colors"),
				new RecoilFileType(".mil", "Micro Illustrator; 160x200, 16 colors"),
				new RecoilFileType(".mis", "AtariTools-800 missile; 2x240, mono"),
				new RecoilFileType(".mon", "Mono Magic; 320x200, mono"),
				new RecoilFileType(".mpp", "Multi Palette Picture; up to 416x273, 1 or 2 frames"),
				new RecoilFileType(".neo", "NEOchrome"),
				new RecoilFileType(".nlq", "Daisy-Dot; 19x16 font, mono"),
				new RecoilFileType(".ocp", "Advanced Art Studio; 160x200, 16 colors"),
				new RecoilFileType(".p64", "Picasso 64; 160x200, 16 colors"),
				new RecoilFileType(".pac", "STAD; 640x400, mono, compressed"),
				new RecoilFileType(".pc1", "DEGAS Elite; 320x200, 16 colors, compressed"),
				new RecoilFileType(".pc2", "DEGAS Elite; 640x200, 4 colors, compressed"),
				new RecoilFileType(".pc3", "DEGAS Elite; 640x400, mono, compressed"),
				new RecoilFileType(".pcs", "PhotoChrome; 320x199, 1 or 2 frames, compressed"),
				new RecoilFileType(".pgc", "Atari Portfolio; 240x64, mono, compressed"),
				new RecoilFileType(".pgf", "Atari Portfolio; 240x64, mono"),
				new RecoilFileType(".pi", "Blazing Paddles; 160x200, 16 colors"),
				new RecoilFileType(".pi1", "DEGAS; up to 416x560, 16 colors"),
				new RecoilFileType(".pi2", "DEGAS; 640x200, 4 colors"),
				new RecoilFileType(".pi3", "DEGAS; 640x400, mono"),
				new RecoilFileType(".pi4", "Fuckpaint; 320x240 or 320x200, 256 colors"),
				new RecoilFileType(".pi9", "Fuckpaint; 320x240 or 320x200, 256 colors"),
				new RecoilFileType(".pic", "Koala MicroIllustrator; 160x192, 4 colors, compressed"),
				new RecoilFileType(".pla", "AtariTools-800 player; 8x240, mono"),
				new RecoilFileType(".plm", "Plama 256; 80x96, 256 colors"),
				new RecoilFileType(".pmd", "PMG Designer by Henryk Karpowicz"),
				new RecoilFileType(".pmg", "Paint Magic; 160x200, 16 colors"),
				new RecoilFileType(".pzm", "EscalPaint; 80x192, 256 colors, 2 frames"),
				new RecoilFileType(".raw", "XL-Paint MAX; 160x192, 16 colors, 2 frames"),
				new RecoilFileType(".rgb", "Atari 8-bit or ZX Spectrum; 3 frames"),
				new RecoilFileType(".rip", "Rocky Interlace Picture; up to 320x239, 1 or 2 frames"),
				new RecoilFileType(".rm0", "Rambrandt; 160x96, 99 colors"),
				new RecoilFileType(".rm1", "Rambrandt; 80x192, 256 colors"),
				new RecoilFileType(".rm2", "Rambrandt; 80x192, 104 colors"),
				new RecoilFileType(".rm3", "Rambrandt; 80x192, 128 colors"),
				new RecoilFileType(".rm4", "Rambrandt; 160x192, 99 colors"),
				new RecoilFileType(".rp", "Rainbow Painter; 160x200, 16 colors"),
				new RecoilFileType(".rpm", "Run Paint; 160x200, 16 colors"),
				new RecoilFileType(".sar", "Saracen Paint; 160x200, 16 colors"),
				new RecoilFileType(".scr", "256x192, 15 colors"),
				new RecoilFileType(".sge", "Semi-Graphic logos Editor; 320x192, mono"),
				new RecoilFileType(".shc", "SAMAR Hi-res Interlace with Map of Colours; 320x192, 2 frames"),
				new RecoilFileType(".shp", "Blazing Paddles or Movie Maker shapes"),
				new RecoilFileType(".spc", "Spectrum 512 Compressed or The Graphics Magician Picture Painter"),
				new RecoilFileType(".sps", "Spectrum 512 Smooshed; 320x199, 512 colors, compressed"),
				new RecoilFileType(".spu", "Spectrum 512; 320x199, 512 colors"),
				new RecoilFileType(".sxs", "16x16 font, mono"),
				new RecoilFileType(".tip", "Taquart Interlace Picture; up to 160x119, 2 frames"),
				new RecoilFileType(".tn1", "Tiny Stuff; 320x200, 16 colors, compressed"),
				new RecoilFileType(".tn2", "Tiny Stuff; 640x200, 4 colors, compressed"),
				new RecoilFileType(".tn3", "Tiny Stuff; 640x400, mono, compressed"),
				new RecoilFileType(".tny", "Tiny Stuff; compressed"),
				new RecoilFileType(".trp", "EggPaint; 65536 colors"),
				new RecoilFileType(".tru", "IndyPaint; 65536 colors"),
				new RecoilFileType(".wnd", "Blazing Paddles window; up to 160x192, 4 colors"),
				new RecoilFileType(".vid", "Vidcom 64; 160x200, 16 colors"),
				new RecoilFileType(".vzi", "VertiZontal Interlacing; 160x200, grayscale, 2 frames"),
				new RecoilFileType(".ximg", "Extended GEM Bit Image; compressed"),
				new RecoilFileType(".xlp", "XL-Paint; 160x192 or 160x200, 7 colors, 2 frames, compressed"),
				new RecoilFileType(".zxp", "ZX-Paintbrush; 256x192, 15 colors")
			};
		}
	}
}
