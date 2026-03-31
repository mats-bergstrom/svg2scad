# Create files suited for openscad from an inkscape SVG file.

## Background
I do 3D modeling using OpenScad and polygons is a very efficient way to do fairly complex designs.  Inkscape provides a very easy way to create and edit polygons.  Therefore I needed a way to to read in an SVG file creted using inkscape and emit a scad file.

# Versions
| Version | Notes |
| ------- | ----- |
| v0.1    | Initial version.  Linear paths are functional, splines are not. |
| v0.2    | Cubic splines are functional.  Quadratic splines are not implemented. |
| v0.21   | Removed duplicate points in splines. |
|         |  |
| v0.3    | Use text elements for code. |
| | Use Text elements for settings. |
| | |
| TBD | |
| | Remove duplicate enties in paths. |
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
| -V2 | Use v0.2x behaviour. |

The output file is named as the input file with ".scad" added.  E.g. The input file "my.svg" will have "my.svg.scad" as output file.


# SVG Elements

## Polygons
Only named polygons (i.e. with svg attribute inkscape:label set) are used.
The name/label can be set in inskcapes dialogue "Object Properties" or in the dialogue "Layers and Objects" with setting "Only SHow Layers" set to OFF.

Each named polygon will result in 3 variables in the output file
* An array containing the points of the path.
* A point corresponding to the maximum point in the path.
* A point corresponding to the minimum point in the path.
The names of the variables are: `s2s_label_path`,
`s2s_label_min` and `s2s_label_max`, respectively

I.e. an SVG path labelled P1 will result in:

```
s2s_P1_path = [ . . . ];
s2s_P1_min = [ xmin, ymin ];
s2s_P1_max = [ xmax, ymax ];

```

## Text Elements
Text witin SVG Text Elements (and contained tspan elements)
are concatenated into a list of strings, one per XML Text Element.
If the first line in an SVG Text Element equals "$code" it is a Code Text Element.
If the first line in an SVG Text Element equals "$settings" it is a Settings Text Element.

### Code Text Elements

Code Text Elements are emitted to the output file after some
substitutions are made:

1. The initial line "$code" is removed.
2. Meta Variables are replaced with their values.

### Settings Text Elements

Settings Text Elements are processed according to the settings.

# Meta Variables
Meta variables follow the pattern:
```
$ name ( arg )
```

| Name | Argument | Value |
| --- | --- | --- |
| path | path-name | Name of the variable holding the path point array. |
| min | path-name | Name of the variable holding the path minimum point. |
| max | path-name | Name of the variable holding the path maximum point. |
| min2zero | path-name | scad translate command to move the path minimum point to `[0,0,0`] |
| max2zero | path-name | scad translate command to move the path maximum point to `[0,0,0`] |
| com2zero | path-name | scad translate command to move the path centre point to `[0,0,0`] |

Example:
```
$com2zero(P1) --> translate([-x_com,-y_com,0])
$path(P1) --> s2s_P1_path
```

# Settings
Settings are simple tag-values using the pattern:
```
tag : val [; tag : val]+ [// Comment]
```

Settings:

| Tag | Value | Effect |
| --- | --- | --- |
| origin | svg | Keep svg origin and coordinate system un change into the output file. |
| origin | scad | Replace y-values: y_scad = page_height - y_svg. "

The `origin` setting is needed as SVG uses the upper-left corner of a page as origin with y-values increasing downwards and scad has y-values increasing "upwards" (when viewing from the positive z-axis).  Using `origin:scad` replaces all y-values in paths to `page_height - original_y`.

# Internals
## Overview
TinyXML is used to parse the SVG file.
* https://github.com/leethomason/tinyxml2
* On ubuntu: sudo apt install tinyxml2-dev

After reading the svg document, an "S2S" (SVG2Scad) object is created using
a factory function.
The S2S object then visits the document using the tinyxml::XMLVisitor
and builds up lists of the paths and other relevant elements.

The S2S object then compiles the information and, unparses to the output file.
