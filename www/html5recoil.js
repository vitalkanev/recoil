/*
 * html5recoil.js - HTML 5 viewer
 *
 * Copyright (C) 2013-2021  Piotr Fusik
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

let recoilFiles;

function recoil2canvas(mainFilename)
{
	const recoil = new RECOIL();
	recoil.readFile = (filename, content, contentLength) => {
		if (!recoilFiles.hasOwnProperty(filename))
			return -1;
		const source = recoilFiles[filename];
		if (contentLength > source.length)
			contentLength = source.length;
		for (let i = 0; i < contentLength; i++)
			content[i] = source[i];
		return contentLength;
	};

	const content = recoilFiles[mainFilename];
	if (!recoil.decode(mainFilename, content, content.length)) {
		alert("Error!");
		return;
	}

	const canvas = document.getElementById("canvas");
	const width = recoil.getWidth();
	const height = recoil.getHeight();
	const pixels = recoil.getPixels();
	canvas.width = width;
	canvas.height = height;
	const context = canvas.getContext("2d");
	const imageData = context.createImageData(width, height);
	for (let i = 0; i < width * height; i++) {
		const rgb = pixels[i];
		const j = i << 2;
		imageData.data[j] = rgb >> 16;
		imageData.data[j + 1] = rgb >> 8 & 0xff;
		imageData.data[j + 2] = rgb & 0xff;
		imageData.data[j + 3] = 0xff;
	}
	context.putImageData(imageData, 0, 0);

	const status = document.getElementById("status");
	status.innerHTML = recoil.getPlatform() + ", " + recoil.getOriginalWidth() + "x" + recoil.getOriginalHeight();

	if (canvas.webkitRequestFullScreen || canvas.mozRequestFullScreen)
		document.getElementById("fullScreenButton").style.display = "";

	const pngLink = document.getElementById("pngLink");
	pngLink.href = canvas.toDataURL("image/png");
	pngLink.download = mainFilename + ".png";
	pngLink.style.display = "";
}

function populateFiles(contents, mainFilenames)
{
	const select = document.getElementById("fileSelect");
	switch (mainFilenames.length) {
	case 0:
		alert("No supported file selected");
		return;
	case 1:
		select.style.display = "none";
		break;
	default:
		select.innerHTML = "";
		for (const name of mainFilenames) {
			const option = document.createElement("option");
			option.text = name;
			option.value = name;
			select.add(option);
		}
		select.style.display = "";
		break;
	}
	recoilFiles = contents;
	recoil2canvas(mainFilenames[0]);
}

function openFiles(files)
{
	const mainFilenames = [];
	const contents = {};
	let count = files.length;
	for (const file of files) {
		if (RECOIL.isOurFile(file.name))
			mainFilenames.push(file.name);
		const reader = new FileReader();
		reader.onload = e => {
			contents[file.name] = new Uint8Array(e.target.result);
			if (--count == 0)
				populateFiles(contents, mainFilenames);
		};
		reader.readAsArrayBuffer(file);
	}
}
