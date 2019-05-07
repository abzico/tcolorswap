/**
 * program to rearrange specified target color in colormap of input gif image to be at first position
 * this is meant to be used with openbor engine
 *
 * This program is designed to work with only 1 single image at a time.
 * You can batch process several images by executing this program via bash script.
 *
 * cli usage
 *  tcolorswap red-value green-value blue-value input-file output-file
 * 
 * red-value    - red color component value
 * green-value  - green color component value
 * blue-value   - blue color component value
 * input-file   - input gif file to modify its color map
 * output-file  - output gif file to output the result
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <gif_lib.h>

// input/output filenames
static const char* input_filename = NULL;
static const char* output_filename = NULL;
// if no red/green/blue component values are specified
// then these are the default values to be used
static unsigned char trans_red = 0;
static unsigned char trans_green = 253;
static unsigned char trans_blue = 255;

// colormap of input image file
static ColorMapObject* colormap;

/**
 * Close decoding gif file.
 * After calling this function, `file` should not be further used anymore.
 */
static void close_dgiffile(GifFileType* file);

/**
 * Close encoding gif file.
 * After calling this function, `file` should not be further used anymore.
 */
static void close_egiffile(GifFileType* file);

/**
 * Free map object.
 * After calling this function, `map` should not be further used anymore.
 */
static void free_mapobject(ColorMapObject* map);

/**
 * Clean up as *neccessary* for input files and object in correct order.
 * Each parameter can be NULL, otherwise it will attemp to clean and free such object accordingly.
 *
 * This function will set NULL to input variables thus users don't have to manually set to NULL again after.
 */
static void cleanup_res(GifFileType** filein, GifFileType** fileout, ColorMapObject** filein_mapobject);

/**
 * Print usage text of this program.
 */
static void cli_print_usage();

/**
 * call exit() with specified status code, but print input `err_str_format` to standard error before exiting.
 */
static void exitnow(int status, const char* err_str_format, ...);

void close_dgiffile(GifFileType* file)
{
  int error_code = 0;
  if (DGifCloseFile(file, &error_code) == GIF_ERROR)
  {
    fprintf(stderr, "Error closing decoding file [error code: %d]\n", error_code);
  }
  // otherwise ok
}

void close_egiffile(GifFileType* file)
{
  int error_code = 0;
  if (EGifCloseFile(file, &error_code) == GIF_ERROR)
  {
    fprintf(stderr, "Error closing encoding file [error code: %d]\n", error_code);
  }
  // otherwise ok
}

void free_mapobject(ColorMapObject* map)
{
  GifFreeMapObject(map);
}

void exitnow(int status, const char* err_str_format, ...)
{
  va_list arg_list;
  va_start(arg_list, err_str_format);

  vfprintf(stderr, err_str_format, arg_list);

  va_end(arg_list);

  // exit with specified status code
  exit(status);
}

void cleanup_res(GifFileType** filein, GifFileType** fileout, ColorMapObject** filein_mapobject)
{
  if (filein != NULL)
  {
    close_dgiffile(*filein);
    *filein = NULL;
  }

  if (fileout != NULL)
  {
    close_egiffile(*fileout);
    *fileout = NULL;
  }

  if (filein_mapobject != NULL)
  {
    free_mapobject(*filein_mapobject);
    *filein_mapobject = NULL;
  }
}

void cli_print_usage()
{
  printf("tcolorswap by Wasin Thonkaew (Angry Baozi Entertainment https://abzi.co\n\n");
  printf("Usage: tcolorswap red-value green-value blue-value input-file output-file\n\n");
  printf("  red-value    - red color component value\n");
  printf("  green-value  - green color component value\n");
  printf("  blue-value   - blue color component value\n");
  printf("  input-file   - input gif file to modify its color map\n");
  printf("  output-file  - output gif file to output the result\n");
}

int main(int argc, char** argv)
{
  // cli's arguments
  // tcolorswap red-value green-value blue-value input-file output-file
  if (argc < 6)
  {
    if (argc >= 2 && strncmp(argv[1], "--help", 6) == 0)
    {
      cli_print_usage();
    }
    else
    {
      fprintf(stderr, "Not enough parameters entered!\n\n");
      cli_print_usage();
    }
    return 1;
  }
  else
  {
    trans_red = (int)strtol(argv[1], NULL, 10);
    trans_green = (int)strtol(argv[2], NULL, 10);
    trans_blue = (int)strtol(argv[3], NULL, 10);

#ifndef NDEBUG
    printf("[DEBUG] %d %d %d\n", trans_red, trans_green, trans_blue);
#endif

    input_filename = argv[4];
    output_filename = argv[5];

#ifndef NDEBUG
    printf("[DEBUG] input filename '%s'\n", input_filename);
    printf("[DEBUG] output filename '%s'\n", output_filename);
#endif
  }

  // open gif file
  // after it opens, it will fill up gif file structure with information like number of colors in colormap
  GifFileType * gif_filein = DGifOpenFileName(input_filename, NULL);
  if (!gif_filein)
  {
    exitnow(1, "Error opening gif file %s\n", input_filename);
  }

  // create colormap mem to hold colors from colormap
  //
  // Note: we possibly has no need to use GifMakeMapObject() function here but just
  // directly access colormap via ->SColorMap->Colors but we will lose automatic checking
  // of POT (power-of-two) of colors in colormap and memory checking.
  colormap = GifMakeMapObject(gif_filein->SColorMap->ColorCount, gif_filein->SColorMap->Colors);
  if (colormap == NULL)
  {
    cleanup_res(&gif_filein, NULL, &colormap);
    exitnow(1, "Error creating colormap object\n");
  }

  // check if input image has color map at all?
  if (gif_filein->SColorMap == NULL)
  {
    cleanup_res(&gif_filein, NULL, &colormap);
    exitnow(1, "No colormap for %s\n", input_filename);
  }

  // check if colormap has at least 1 color
  if (gif_filein->SColorMap->ColorCount <= 0)
  {
    cleanup_res(&gif_filein, NULL, &colormap);
    exitnow(1, "Error number of colors in colormap is 0\n");
  }

  // form the target color
  GifColorType target_trans_color;
  target_trans_color.Red = trans_red;
  target_trans_color.Green = trans_green;
  target_trans_color.Blue = trans_blue;

  // global color map
  // global color map is colormap (not histogram) as shown in `identify -verbose ...`
  // color map always in RGB format, and has maximum of 256 colors in a single map
  // note: local color map is not always available
  int num_color_colormap = colormap->ColorCount; 

#ifndef NDEBUG
  printf("[DEBUG] Colors: %d\n", num_color_colormap); 
#endif

  // access to the colormap's colors memory
  // **remember**: to access map object not directly here
  // libgif doesn't offer in-place modification of colormap, thus directly access makes no sense.
  // accessing via map object is safer, and we will use it (as a copy) to write to output image.
  GifColorType* filein_colors = colormap->Colors;

  // check to make sure there's only one target to be swapping color
  // in the color map
  // then next one we process it
  bool found = false;

  // marked position (index-based) -1 is initial state
  int marked_posidx = -1;
  for (int i=0; i<num_color_colormap; ++i)
  {
    GifColorType c = filein_colors[i];

    if (c.Red == target_trans_color.Red &&
        c.Green == target_trans_color.Green &&
        c.Blue == target_trans_color.Blue)
    {
      // TODO: idea to add checking of shaded of transparent color (just in case) - (optional, less priority)
      // match, and already found
      // error, we won't work with duplicated or shaded of colors variation of target transparent color
      if (found)
      {
        cleanup_res(&gif_filein, NULL, &colormap);
        exitnow(1, "There should not be duplicated transparent color in colormap. Quit now.\n");
      }
      else
      {
        // not found, yet, it's ok, just set the flag
        found = true;

        // mark the position
        marked_posidx = i;
      }
    }
  }

  // if not found, clean resource and exit now
  if (!found)
  {
    cleanup_res(&gif_filein, NULL, &colormap);
    exitnow(1, "Not found target color to rearrange in colormap. Quit now.\n");
  }

  // swap the color to first position of colormap
  GifColorType temp_color = filein_colors[marked_posidx];
  filein_colors[marked_posidx] = filein_colors[0];
  filein_colors[0] = temp_color;

  // now we're ready to write to output file
  int error_code;
  GifFileType* gif_fileout = EGifOpenFileName(output_filename, false, &error_code);
  if (gif_fileout == NULL)
  {
    cleanup_res(&gif_filein, NULL, &colormap);
    exitnow(1, "Error opening output file %s to write [error code: %d]\n", output_filename, error_code);
  }

  // set gif version to GIF89
  EGifSetGifVersion(gif_fileout, true);

  // put screen description
  if(EGifPutScreenDesc(gif_fileout,
        gif_filein->SWidth,
        gif_filein->SHeight,
        gif_filein->SColorResolution,
        gif_filein->SBackGroundColor,
        colormap) == GIF_ERROR)
  {
    cleanup_res(&gif_filein, &gif_fileout, &colormap);
    exitnow(1, "Cannot put screen description to output file %s\n", output_filename);
  }

  // read and handle each type of record accordingly
  // until we reach the end of record
  GifRecordType record_type = UNDEFINED_RECORD_TYPE;
  do {
    // read next record
    if (DGifGetRecordType(gif_filein, &record_type) == GIF_ERROR)
    {
      cleanup_res(&gif_filein, &gif_fileout, &colormap);
      exitnow(1, "Error reading next record\n");
    }

    // variables used inside the switch block
    int extcode = 0;
    GifByteType* extension;

    // handle accordingly for each record type
    switch (record_type)
    {
      case IMAGE_DESC_RECORD_TYPE:
        // read in image description from input file
        if (DGifGetImageDesc(gif_filein) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error getting image description\n");
        }

        // put same image description into output file
        // putting modified colormap into output image
        if (EGifPutImageDesc(gif_fileout,
            gif_filein->Image.Left,
            gif_filein->Image.Top,
            gif_filein->Image.Width,
            gif_filein->Image.Height,
            gif_filein->Image.Interlace,
            gif_filein->Image.ColorMap) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error putting image destination\n");
        }

        int i;
        register GifPixelType* cp;

        GifPixelType* line = (GifPixelType*)malloc(gif_filein->Image.Width * sizeof(GifPixelType));
        for (i=0; i<gif_filein->Image.Height; ++i)
        {
          if (DGifGetLine(gif_filein, line, gif_filein->Image.Width) == GIF_ERROR)
          {
            cleanup_res(&gif_filein, &gif_fileout, &colormap);
            exitnow(1, "Error getting line from input image\n");
          }

          // do translation (only one pixel for target transparent pixel)
          // this happens only once
          for (cp = line; cp < line + gif_filein->Image.Width; ++cp)
          {
            if (*cp == marked_posidx)
            {
              *cp = 0;
            }
            else if (*cp == 0)
            {
              *cp = marked_posidx;
            }
          }

          // put line
          if (EGifPutLine(gif_fileout, line, gif_filein->Image.Width) == GIF_ERROR)
          {
            cleanup_res(&gif_filein, &gif_fileout, &colormap);
            exitnow(1, "Error putting line into output file\n");
          }
        }
        free(line);
        break;

      case EXTENSION_RECORD_TYPE:

        // read extension block
        if (DGifGetExtension(gif_filein, &extcode, &extension) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error reading extension from input file\n");
        }
        // check if extension is not available, then break now
        if (extension == NULL)
          break;

        // start writing the beginning of extension block
        if (EGifPutExtensionLeader(gif_fileout, extcode) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error putting extension leader\n");
        }
        if (EGifPutExtensionBlock(gif_fileout, extension[0], extension + 1) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error putting extension block\n");
        }

        while (extension != NULL)
        {
          if (DGifGetExtensionNext(gif_filein, &extension) == GIF_ERROR)
          {
            cleanup_res(&gif_filein, &gif_fileout, &colormap);
            exitnow(1, "Error getting next extension\n");
          }
          if (extension != NULL)
          {
            if (EGifPutExtensionBlock(gif_fileout, extension[0], extension + 1) == GIF_ERROR)
            {
              cleanup_res(&gif_filein, &gif_fileout, &colormap);
              exitnow(1, "Error putting extension block\n");
            }
          }
        }

        if (EGifPutExtensionTrailer(gif_fileout) == GIF_ERROR)
        {
          cleanup_res(&gif_filein, &gif_fileout, &colormap);
          exitnow(1, "Error putting extension block\n");
        }

        break;

      case TERMINATE_RECORD_TYPE:
        break;
      default:
        break;
    }
  } while (record_type != TERMINATE_RECORD_TYPE);

  cleanup_res(&gif_filein, &gif_fileout, &colormap);
  return 0;
}
