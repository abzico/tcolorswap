# tcolorswap
Utility program to swap specified target transparent pixel to be at first position of colormap of input gif image

# Intention

This is meant to be used with game engine that requires first position of color map attached with sprite image to be transparent color, so engine itself can render it properly.

As from research, gimp or other photo editing tool can do it but it requires manually (no batch processing), gimp scripting has no feature or API to access color map (instead although it has somewhat seems to do so but it access histogram colormap of image), `gitclrmp` utility tool is almost the one we want but we need custom solution.

# LICENSE
MIT, Angry Baozi [https://abzi.co](https://abzi.co)
