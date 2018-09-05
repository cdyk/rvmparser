# rvmparser

Code to work with AVEVA PDMS RVM files. Written completely from scratch, intended to be very fast, small, and with _little dependencies_, so it is trivial to include in existing projects.

Includes a sample application that:
- Reads AVEVA PDMS binary RVM and attribute files.
- Triangulate primitive shapes with a prescribed error tolerance.
- Optionally merges groups using a prescribed list of groups.
- Exports geometry as Wavefront OBJ files
- Exports attributes as json.

## Usage

```
Usage: rvmparser [options] files                
                                                                                       
Files with .rvm-suffix will be interpreted as a geometry files, and files with .txt    
suffix will be interpreted as attribute files.                                         
You typically want to pass one of each.                                                
                                                                                       
Options:                                                                               
    --keep-groups=filename.txt   Provide a list of group names to keep. Groups not     
                                 itself or with a child in this list will be merged    
                                 with the first parent that should be kept.            
    --output-json=filename.json  Write hierarchy with attributes to a json file.       
    --output-txt=filename.txt    Dump all group names to a text file.                  
    --output-obj=filenamestem    Write geometry to an obj file, .obj and .mtl          
                                 are added to filenamestem.                            
    --group-bounding-boxes       Include wireframe of boundingboxes of groups in output
    --color-attribute=key        Specify which attributes that contain color, empty    
                                 imply that material id of group is used.              
    --tolerance=value            Tessellation tolerance, given in world frame.         
    --cull-scale=value           Cull objects that are smaller than cull-scale times   
                                 tolerance. Set to a negative value to disable culling.
```

## Building

Clone the git repo, and update submodules
```
git submodule update --init --recursive
```
and open the solution file in msvc15. The project is just a few source-files, so it should be trivial to use other build systems.

Currently, the example command-line utility (`main.cpp`) only builds on windows, as it uses the windows memmap API, but the parser itself is just passed pointers and size of an in-memory buffer, so the rest of the code should be fully cross-platform.

Dependencies:
- libtess2](https://github.com/memononen/libtess2) to triangulate polygon more complex than quads.

## See also
- [Plant Mock-Up Converter](https://github.com/benvautrin/pmuc).