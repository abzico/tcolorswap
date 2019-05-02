# Test case

Sprite test case provided here is an asset copyrighted by Secret Character; in which it's a sprite from iOS game "Zombie Hero : Revenge of Kiki".

The sprite is post-processed by
- rescaling from 1000x1000 to 250x250 px
- flatten and remove alpha channel completely, and at the same time replace transparent pixel with rgb(0,253,255)

Now you can use `tcolorswap` to move the target transparent pixel to be at the first location inside colormap attached to this image after the post-processing.

You can use imagemagick's `identify` i.e. `identify -verbose <image>` to check that its `ColorMap` is correct.
