# Create files suited for openscad from an inkscape SVG file.

## Background
I do 3D modeling using OpenScad and polygons is a very efficient way to do fairly complex designs.  Inkscape provides a very easy way to create and edit polygons.  Therefore I needed a way to to read in an SVG file creted using inkscape and emit a scad file.

# Versions
| Version | Notes |
| ------- | ----- |
| v0.1    | Initial version.  Linear paths are functional, splines are not. |
| v0.2    | Cubic splines are functional.  Quadratic splines are not implemented. |
|         |  |
| TBD     | Remove duplicate enties in paths. |
| | Add arcs in paths. |
| | Use text elements for in-line code and directives. |

# Use
```
$ svg2scad [options] input.svg
```
| Option | |
| ---- | ---- |
| -v | Verbose mode. |
| -d | Enable debug prints. |
| -dL <double> | Set delta length (in mm) used in splines.  Default is 2.0. |


# Internals
## Overview
TinyXML is used to parse the SVG file.
* https://github.com/leethomason/tinyxml2
* On ubuntu: sudo apt install tinyxml2-dev

The class SVGVisitor inherits the tinyxml2::XMLVisitor and overloads the virtual functions for entering and exiting XML entities.

The class SVGPathToken holds a path token (from the d attribute).
The class SVGPathTokens holds a list of tokens (full contents of the d attribute).

The namespace 2s2 contains most definitions related to scad generation.

The class s2s::s2D handles the “2D” functionality; namsepace and paths.

Unparsing generates several openscad "variables" for each path
* An XY path list.
* An XY point corresponding the minimum x and y values.
* An XY point corresponding the maximum x and y values.
