# rvmparser

[![Build Status](https://github.com/cdyk/rvmparser/actions/workflows/build.yml/badge.svg)](https://github.com/cdyk/rvmparser/actions/)
[![Publish](https://github.com/cdyk/rvmparser/actions/workflows/publish.yml/badge.svg)](https://github.com/cdyk/rvmparser/actions/)


Code to work with AVEVA PDMS RVM files. Written completely from scratch, intended to be very fast, small, and with _little dependencies_, so it is trivial to include in existing projects.

Includes a sample application that:
- Reads AVEVA PDMS binary RVM and attribute files.
- Tries to match adjacent geometries to:
  - Avoid adding internal caps between adjacent shapes.
  - Align circumferential sample points between adjacent shapes.
- Triangulate primitive shapes with a prescribed error tolerance.
- Optionally merges groups using a prescribed list of groups.
- Exports geometry as Wavefront OBJ files
- Exports geometry as GLTF files
- Exports attributes as json.
- Exports database as .rev text files

## Usage

```
Usage: rvmparser [options] files                

Files with .rvm-suffix will be interpreted as a geometry files, and files with .txt or .att suffix
will be interpreted as attribute files. A rvm file typically has a matching attribute file.

Options:
  --keep-regex=<keep-regex>           Prune hierarchy by flattening node hierarchy that has names
                                      that do not match the regular expression. Note that the full
                                      name must match the regex, not just a part of the name, e.g.,
                                      the regex ^/.* will match all names that start with /.
  --keep-groups=filename.txt          Provide a list of group names to keep. Groups not itself or
                                      with a child in this list will be merged with the first
                                      parent that should be kept.
  --discard-groups=filename.txt       Provide a list of group names to discard, one name per line.
                                      Groups with its name in this list will be discarded along
                                      with its children. Default is no groups are discarded.
  --output-json=<filename.json>       Write hierarchy with attributes to a json file.
  --output-txt=<filename.txt>         Dump all group names to a text file.
  --output-rev=filename.rev           Write database as a text .rev file.
  --output-obj=<filenamestem>         Write geometry to an obj file. The suffices .obj and .mtl are
                                      added to the filenamestem.
  --output-gltf=<filename.gltf>       Write geometry into a GLTF file (pure JSON with buffers base64
             or <filename.glb>        encoded inline) or a GLB file (JSON with binary buffers in a
                                      GLB container). Type of file is specified by the suffix.
  --output-gltf-attributes=<bool>     Include rvm attributes in the extra member of nodes. Default
                                      value is true.
  --output-gltf-center=<bool>         Position the model at the bounding box center and store the
                                      position of this origin in the asset's extra field. This is
                                      useful if the model is so far away from the origin that float
                                      precision becomes an issue. Defaults value is false.
  --output-gltf-rotate-z-to-y=<bool>  Add an extra node below the root that adds a clockwise
                                      rotation of 90 degrees about the X axis such that the +Z axis
                                      will map to the +Y axis, which is the up-direction of GLTF-
                                      files. Default value is true.
  --output-gltf-split-level=<uint>    Specify a level in the hierarchy to split the output into
                                      multiple files, where 0 implies no split. Geometries and
                                      attributes below the split point are included in the first
                                      file, while subsequent files while have empty nodes just to
                                      represent the hierarchy. Default value is 0.
  --group-bounding-boxes              Include wireframe of boundingboxes of groups in output.
  --color-attribute=key               Specify which attributes that contain color, empty key
                                      implies that material id of group is used.
  --tolerance=value                   Tessellation tolerance, given in world frame. Default value
                                      is 0.1.
  --cull-scale=value                  Cull objects smaller than cull-scale times tolerance. Set to
                                      a negative value to disable culling. Disabled by default.
```

## Binary releases

Download automatic release builds from [releases](https://github.com/cdyk/rvmparser/releases)

## Building from source

Clone the git repo, and update submodules
```
git submodule update --init --recursive
```
The submodule dependencies are:
- [libtess2](https://github.com/memononen/libtess2) is used to triangulate polygon more complex than quads.
- [rapidjson](https://github.com/Tencent/rapidjson/) is used to output attributes as JSON.

The project is just a few source-files, so it should be trivial to use with any build systems.


### Windows (Visual Studio)

Open `msvc15\rvmparser.sln` in visual studio and build the solution.


### Linux and MacOS (gcc or clang)

Enter the `make` directory and type `make`.


## See also
- [Plant Mock-Up Converter](https://github.com/benvautrin/pmuc).

## License

This application is available to anybody free of charge, under the terms of the MIT License (see LICENSE).
