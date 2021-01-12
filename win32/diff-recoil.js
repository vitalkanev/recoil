// wscript.exe "PATH\TO\diff-recoil.js" %base %mine %bname %yname //E:javascript

// Get command-line arguments

var argc = WScript.Arguments.length;
if (argc != 2 && argc != 4) {
	WScript.Echo("Specify two filenames and optionally two titles");
	WScript.Quit(1);
}
var pic1 = WScript.Arguments(0);
var pic2 = WScript.Arguments(1);
var title1 = WScript.Arguments(argc - 2);
var title2 = WScript.Arguments(argc - 1);

// Find TortoiseIDiff directory

var wsh = new ActiveXObject("WScript.Shell");
var tpath;
try {
	tpath = wsh.RegRead("HKLM\\SOFTWARE\\TortoiseSVN\\Directory");
}
catch (e) {
	try {
		tpath = wsh.RegRead("HKLM\\SOFTWARE\\TortoiseGit\\Directory");
	}
	catch (e) {
		WScript.Echo("TortoiseIDiff not found!");
		WScript.Quit(1);
	}
}

// Convert images to PNG

var fso = new ActiveXObject("Scripting.FileSystemObject");
var exe = fso.GetParentFolderName(WScript.ScriptFullName);
exe = fso.BuildPath(exe, "recoil2png.exe");

function recoil2png(pic)
{
	var png = fso.BuildPath(fso.GetSpecialFolder(2), fso.GetTempName());
	wsh.Run("cmd /c \"\"" + exe + "\" -o \"" + png + "\" \"" + pic + "\"\"", 0, true);
	return png;
}

var png1 = recoil2png(pic1);
var png2 = recoil2png(pic2);

// Display results

wsh.Run("\"" + tpath+ "bin\\TortoiseIDiff.exe\" /left:\"" + png1 + "\" /right:\"" + png2 + "\" /lefttitle:\"" + title1 + "\" /righttitle:\"" + title2 + "\"", 4, true);

// Delete temporary files when done

fso.DeleteFile(png2);
fso.DeleteFile(png1);
