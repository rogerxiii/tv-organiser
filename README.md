# tv-organiser
Whether from torrents or NZBs, downloading TV shows gives many folders and files with unappealing, incomplete or even gibberish names. This Windows-only command line tool moves together and renames all episodes using data from IMDB.  
  
## Usage
It is highly recommended to put this tool in a folder which is in PATH so it can be used freely.
Alternatively, you can copy the tool to the folder where you want to run it. Double click or launch from the command prompt with optional flags.  
There are 2 ways of supplying an IMDB number:
- Enter it when the program asks for it.
- Have a file called _imdb.txt_ in the same folder as the tool containing the IMDB number (this can be a hidden file to not clutter the folder structure).

The IMDB number can be formatted as _tt1234567_ or simply _1234567_.  
 
## Features
- Renames all episodes to the same format
- Pattern system to detect a wide variety of formatting of episode and season number which is easily expanded
- Takes individual episodes out of their folders into the root
- Can deal with double episodes
- Can deal with unicode names
- Customizable with range of optional flags
- Automatically handles subtitles either in the root folder or in the _subs/_ subfolder

#### Optional Flags
![](https://i.rogerxiii.com/34022020220934222234.png)

## Example
Let's take as an example this first season of the show _Dark_, which looks as follows:  
  
![](https://i.rogerxiii.com/59122019131859221359.png)

With each folder containing an episode file like this one:  
  
![](https://i.rogerxiii.com/08122019131808231308.png)
  
After running this tool the folder looks like this:
  
![](https://i.rogerxiii.com/44122019181844191844.png)
  

## Building Instructions
_libcurl_ is statically linked for convenience sake, and the library is included in the github repo. You may also link it dynamically, but note that the dll will have to be provided whenever the program is run.  
Please note that for the time being this tool is **Windows only**.

## Future Plans
- Create an installer which adds this program to PATH
- Make the tool work on Linux