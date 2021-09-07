# Text-Editor
attempt to make a fully functional bare bones text editor in C ( without any dependencies by directly
 manipulating terminal via [VT100](https://vt100.net) escape sequences ) 

insipred from [kilo text editor](https://github.com/antirez/kilo)

## Status : Alpha Version

The editor can now :
- [x] View already existing file on the system. 
- [x] Edit a text file
- [x] Check if the file is in modified state or not ( and warn if you try to exit a modified file without saving )
- [x] Save chagnes to the open file ( using `Ctrl-s` )
- [x] Quit (using `Ctrl-q` )

---

But still it can't :
- [ ] Create a new file
- [ ] Save a blank file ( Save-as feature )
- [ ] support text highlithing for C/C++

| **⚠️ WARNING:** The software is still in alpha version so if you planning to use it, I suggest making regular backups of your work (using a version control) in case you run into bugs in the editor.|
| --- |

## Platform supported 
Linux, macOS, windows(with cygwin)

## Get it running 

- git clone or download 

- navigate to the repository  

- run a `make` command on your terminal to compile source ( if you don't have gnu-make then you can also manually compile the source code (src/editor.c) with a standard C compiler )

- run exectuatable with the name `editor` 


```bash
$ cd text-editor
$ make
$ ./editor <optional: file name that you want to open>
```

## Thank You for visiting 
