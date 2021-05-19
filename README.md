# Text-Editor
attempt to make a fully fundtional bare bones text editor in C ( withough any dependencies by direcly manipulating terminal via [VT100](https://vt100.net) escape sequences ) 

insipred from [kilo text editor](https://github.com/antirez/kilo)

## Status : 👷‍♂️ under construction 
- editor can view a file and supports navigation via arrow keys (editing is not suported till now)

## Platform supported 
Linux, macOS, windows(with cygwin)

## Get it running 

- git clone or download 

- navigate to the repostiroy 

- run a `make` command on your terminal to compile source ( if you don't have cmake then you can also manually compile the source code (src/editor.c) with a standard C compiler )

- run exectuatable with the name `editor` 


```bash
$ cd text-editor
$ make
$ ./editor
```

## Thank You for visiting 
