# CSASM (TOK Port)
A program that can inspect AngelScript bin files from the game Paper Mario: The Origami King

## Installation
Download `csasm.zip` from the latest release in https://github.com/Darxoon/tok-csasm/releases. Unzip the file and move csasm.exe to a convenient location.

If you want to access the executable from any folder and not just the folder you put it in, you should add it to your PATH. This is not required though.
If you nonetheless want to do this, follow these steps:
 * Copy the folder, where csasm.exe is stored, to the clipboard. The folder that it's stored in shouldn't contain a lot of miscellaneous files and should stay consistent 
 (so preferably a custom folder for the executable and not something like your Downloads folder, for example)
 * In the windows search, search for "Path" and click on `Edit the system environment variables`. The following window should come up.
 ![Screenshot of "Edit the system environment variables" dialog](https://i.imgur.com/YtMxZXH.png)
 * Click on "Environment variables"
 * In the list at the bottom, search for `Path` in the first column. Double click on it.
 * In the upcoming dialog, click on `New` and paste the location of the folder you just copied.
 * Now, you can type `csasm` into your command line from any folder on your system!
 

## Usage
Clicking on csasm.exe won't cause anything to happen. Instead, open a terminal, command line or PowerShell window. Run the following command and fill in the arguments in angled brackets (< and >):

```csasm <decompressed scripts folder>  <registry.json> <AngelScript .bin file> [optional: -o <output file>```

The decompressed scripts folder refers to the path of your decompressed scripts. To get this, you need a dumped ROM of Paper Mario: The Origami King. 
To decompress the files, use https://github.com/darxoon/TOKElfTool and select the scripts folder inside romfs in the "File > Zstd Tools > Decrypt All" dialog.

"registry.json" refers to a dump of the API of Paper Mario: The Origami King.

The AngelScript .bin file is the file you want to dump.

If you include `-o <output file>`, then you need to fill in `<output file>` with a file path and it will write the results to that file, whether or not the file already exists.
If it exists, it will overwrite the existing contents. If none of this is included, it will be printed into the command line.
