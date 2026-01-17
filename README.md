# svg2scad - parse an SVG file and produce openscad files.


Each path in the SVG file is generates an openscad list of X-Y points
in a file named as as the SVG file with the suffix `.scad` added.

The X-Y list is named as `path_<prefix>_<path-name>`.

`<path-name>` corresponds to the value of the `inkscape:label` attribute
of the path element.  If the attribute is missing, the X-Y list is not
generated.

`<prefix>` is recursively built from the values of the `inkscape:label`
attribute of the group elements that contain the path element,
separated by a `_`.

Examples
The SVG:
```
<g inkscape:label="L1">
  <path
	inkscape:label="P1"
   	d="M 55,82 93,63 139,109 Z"
   	/>
</g>
```
will produce
```
path_L1_P1 = [
	       [55,82],
	       [93,63],
	       [139,109],
	     ];
path_L1_P1_min = [55,63];
path_L1_P1_max = [139,109];
```
