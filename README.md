# rvmparser

Code to work with AVEVA PDMS RVM files. Written completely from scratch, intended to be very fast, small, and with _little dependencies_, so it is trivial to include in existing projects.

Includes a sample application that:
- Reads AVEVA PDMS binary RVM and attribute files.
- Tries to match adjacent geometries to:
  - Avoid adding internal caps between adjacent shapes.
  - Align circumferential sample points between adjacent shapes.
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
The submodule dependencies are:
- [libtess2](https://github.com/memononen/libtess2) is used to triangulate polygon more complex than quads.
- [rapidjson](https://github.com/Tencent/rapidjson/) is used to output attributes as JSON.

The project is just a few source-files, so it should be trivial to use with any build systems.

### Visual Studio / Windows

Open the solution file located in `msvc15` and build the project.

### POSIX

Enter the `make` directory. Type
```
export CXX=clang++
export CC=clang
make
```
Rvmparser doesn't currently build with GCC as the source currently contains use of anonymous aggregates of members with constructors. Use clang instead for now.

## See also
- [Plant Mock-Up Converter](https://github.com/benvautrin/pmuc).