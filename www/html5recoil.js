/*
 * html5recoil.js - HTML 5 viewer
 *
 * Copyright (C) 2013-2018  Piotr Fusik
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

function recoil2canvas(contents, mainFilename)
{
	var recoil = new RECOIL();
	recoil.readFile = function(filename, content, contentLength) {
		if (!contents.hasOwnProperty(filename))
			return -1;
		var source = contents[filename];
		if (contentLength > source.length)
			contentLength = source.length;
		for (var i = 0; i < contentLength; i++)
			content[i] = source[i];
		return contentLength;
	}

	var content = contents[mainFilename];
	if (!recoil.decode(mainFilename, content, content.length)) {
		alert("Error!");
		return;
	}

	var canvas = document.getElementById("canvas");
	var width = recoil.getWidth();
	var height = recoil.getHeight();
	var pixels = recoil.getPixels();
	canvas.width = width;
	canvas.height = height;
	var context = canvas.getContext("2d");
	var imageData = context.createImageData(width, height);
	for (var i = 0; i < width * height; i++) {
		var rgb = pixels[i];
		var j = i << 2;
		imageData.data[j] = rgb >> 16;
		imageData.data[j + 1] = rgb >> 8 & 0xff;
		imageData.data[j + 2] = rgb & 0xff;
		imageData.data[j + 3] = 0xff;
	}
	context.putImageData(imageData, 0, 0);

	var status = document.getElementById("status");
	status.innerHTML = recoil.getPlatform() + ", " + recoil.getOriginalWidth() + "x" + recoil.getOriginalHeight();

	if (canvas.webkitRequestFullScreen || canvas.mozRequestFullScreen)
		document.getElementById("fullScreenButton").style.display = "";
}

function openFiles(files)
{
	var mainFilename;
	var contents = new Object();
	var count = files.length;
	Array.prototype.forEach.call(files, function(file) {
			var reader = new FileReader();
			reader.onload = function (e) {
				if (RECOIL.isOurFile(file.name))
					mainFilename = file.name;
				contents[file.name] = new Uint8Array(e.target.result);
				if (--count == 0) {
					if (mainFilename === undefined)
						alert("Unsupported file");
					else
						recoil2canvas(contents, mainFilename);
				}
			};
			reader.readAsArrayBuffer(file);
		});
}
