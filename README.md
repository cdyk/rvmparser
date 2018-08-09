# rvmparser

Code that reads binary RVM files from AVEVA PDMS and optionally triangulates primitives. Includes example Wavefrom OBJ exporter.


## Background

Rvmparser is written completely from scratch with a minimum amount of dependencies. The goal is to provide a lean and fast parser that is trivial to include into existing projects.

The file format was dechiphered by inspecting hex-dumps of RVM-files and reading through the source of [pmoc](https://github.com/benvautrin/pmuc). Currently it depends on [libtess2](https://github.com/memononen/libtess2) to triangulate facet groups. 


## Status

Most fields are recognized and handled, so the code parses the files I have tested.

Triangulation is currently implemented for box, circular torus segment, cylinder, and facet group.


## Todo
- Triangulate all primitive types.
- Make triangle tessellation driven by approximation error.
- Parse attribute files.


## Building

Development is done on msvc15 and an appropriate solution file is included in the repo. Otherwise, it is just a few source files so including it in an existing project or setting it up in other development environments is trivial.

Currently, the example command-line utility (`main.cpp`) only builds on windows, as it uses the windows memmap API, but the parser itself is just passed pointers and size of an in-memory buffer, so the rest of the code should be fully cross-platform.
