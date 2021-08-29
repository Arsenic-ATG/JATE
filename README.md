# Text-Editor
attempt to make a fully functional bare bones text editor in C ( without any dependencies by directly
 manipulating terminal via [VT100](https://vt100.net) escape sequences ) 

insipred from [kilo text editor](https://github.com/antirez/kilo)

## Status : 👷‍♂️ under construction 
- The Editor can view a file and supports navigation via arrow keys
- Update: Basic editing support is added after commit 7db5c29806f05d846d8a818e69e7bbdc4775df49

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
