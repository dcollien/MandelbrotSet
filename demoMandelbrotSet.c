#include <stdio.h>
#include <stdlib.h>

#include "MandelbrotSet.h"

int main(int argc, char *argv[]) {
   int x, y;

   int width = 150;
   int height = 150;

   mandelbrotCoord center = { -0.5, 0.0 };
   int zoom = 6;

   int **pixelValues;

   MandelbrotSet fractal = createMandelbrotSet(width, height);
   MandelbrotSet_setPosition(fractal, center, zoom);
   MandelbrotSet_fastGenerate(fractal);

   pixelValues = MandelbrotSet_getScores(fractal);

   // print out pixel values in PGM image format
   printf("P2\n");
   printf("%d %d\n", width, height);
   printf("%d\n", DEFAULT_MAX_ITERATIONS);
   for (y = 0; y != height; ++y) {
      for (x = 0; x != width; ++x) {
         printf("%3d", pixelValues[y][x]);
         if (x != width-1) {
            printf(" ");
         }
      }
      printf("\n");
   }

   freeMandelbrotSet(fractal);

   return EXIT_SUCCESS;
}
