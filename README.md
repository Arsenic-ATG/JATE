# Text-Editor
attempt to make a fully functional bare bones text editor in C ( without any dependencies by directly
 manipulating terminal via [VT100](https://vt100.net) escape sequences ) 

insipred from [kilo text editor](https://github.com/antirez/kilo)

## Status : 👷‍♂️ under construction 
- editor can view a file and supports navigation via arrow keys (editing is not supported till now)

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
