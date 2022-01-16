# J.A.T.E.
_**J**ust **A**nother **T**erminal-based **E**ditor (JATE)_ is an attempt to make a fully functional bare bones text editor in C ( without any dependencies by directly manipulating terminal via [V100 escape sequences](https://vt100.net/docs/vt100-ug/chapter3.html) )

Insipred from [kilo text editor](https://github.com/antirez/kilo)

## Status : [Beta Version](https://github.com/Arsenic-ATG/Text-Editor/releases)

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

| **⚠️ WARNING:** The software is still in Beta version so if you planning to use it, I suggest making regular backups of your work in case you run into bugs in the editor.|
| --- |

## Platform supported 
Linux, macOS, windows(with cygwin)

## Prerequisites

- [Cmake](https://cmake.org)
- [C compiler](https://en.wikipedia.org/wiki/Compiler)

## Get it running 

- `git clone` or download.

- Navigate to the repository.

- Run [`cmake .`](https://cmake.org/cmake/help/latest/manual/cmake.1.html) command and let CMake generate a [Makefile](https://en.wikipedia.org/wiki/Make_(software)) according to your system configuration.

- Run a `make` command on your terminal to compile source.

- Run exectuatable with the name `JATE`.

```bash
$ cd text-editor
$ cmake .
$ make
$ ./JATE <optional: file name that you want to open>
```

## Thank You for visiting 
