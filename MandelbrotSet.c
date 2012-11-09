#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include "MandelbrotSet.h"

#define ESCAPE_RADIUS_SQ 4

struct mandelbrotSetData {
   int width;
   int height;
   mandelbrotCoord center;
   
   real resolution;
   real top;
   real left;

   int **pixelScores;
   int maxIterations;

   bool isGenerated;
};

// generate a rectangular section of the mandelbrot set pixel by pixel
static void generateRectangle(MandelbrotSet fractal, int startX, int startY, int width, int height);

// generate a rectangular section of the mandelbrot set by filling in chunks expected to be the same color
static void generateDivideAndConquer(MandelbrotSet fractal, int startX, int startY, int width, int height);

static void allocatePixelScores(MandelbrotSet fractal);
static void freePixelScores(MandelbrotSet fractal);

// determine how long it takes a coordinate to escape (if at all),
// before maximum number of iterations is reached
static inline int  escapeScore(MandelbrotSet fractal, mandelbrotCoord coord);

// generate the value at a pixel coordinate and store it in the pixel store
static inline void generateSetPixel(MandelbrotSet fractal, int row, int col);

static inline bool generateBlockRow(MandelbrotSet fractal, int row, int colStart, int width);
static inline bool generateBlockCol(MandelbrotSet fractal, int col, int rowStart, int height);


MandelbrotSet createMandelbrotSet(int width, int height) {
   MandelbrotSet fractal = malloc(sizeof (struct mandelbrotSetData));
   assert(fractal != NULL);

   fractal->width  = width;
   fractal->height = height;

   fractal->maxIterations = DEFAULT_MAX_ITERATIONS;

   fractal->pixelScores = NULL;
   allocatePixelScores(fractal);
   assert(fractal->pixelScores != NULL);

   fractal->isGenerated = false;

   return fractal;
}

void freeMandelbrotSet(MandelbrotSet fractal) {
   int row;
   freePixelScores(fractal);
   free(fractal);
}

void MandelbrotSet_setPosition(MandelbrotSet fractal, mandelbrotCoord center, int zoom) {
   fractal->center = center;

   // calculate the mandelbrot-space distance between pixels
   fractal->resolution = 1.0/((real)((unsigned long long)1 << zoom));

   // width and height in fractal coordinates (from image coordinates)
   real fractalWidth  = fractal->width  * fractal->resolution;
   real fractalHeight = fractal->height * fractal->resolution;

   // top-left coordinate of the viewport rectangle in fractal coordinates
   fractal->left = center.x - (fractalWidth/2.0);
   fractal->top  = center.y + (fractalHeight/2.0);

   fractal->isGenerated = false;
}


void MandelbrotSet_generate(MandelbrotSet fractal) {
   generateRectangle(fractal, 0, 0, fractal->width, fractal->height);
   fractal->isGenerated = true;
}

void MandelbrotSet_fastGenerate(MandelbrotSet fractal) {
   generateDivideAndConquer(fractal, 0, 0, fractal->width, fractal->height);
   fractal->isGenerated = true;
}

// returns a borrowed reference (freed when fractal is freed)
int **MandelbrotSet_getScores(MandelbrotSet fractal) {
   if (!fractal->isGenerated) {
      fprintf(stderr, "Mandelbrot Set has changed and requires regenerating.\n");
      return NULL;
   } else {
      return fractal->pixelScores;
   }
}

void MandelbrotSet_setMaxIterations(MandelbrotSet fractal, int maxIterations) {
   fractal->maxIterations = maxIterations;
}


// Static functions

static void freePixelScores(MandelbrotSet fractal) {
   int row;
   if (fractal->pixelScores != NULL) {
      for (row = 0; row != fractal->height; ++row) {
         free(fractal->pixelScores[row]);
      }
      free(fractal->pixelScores);
   }
}

static void allocatePixelScores(MandelbrotSet fractal) {
   int row;
   if (fractal->pixelScores == NULL) {
      // only need to allocate if not yet allocated

      fractal->pixelScores = (int **)malloc(sizeof(int*) * fractal->height);
      for (row = 0; row != fractal->height; ++row) {
         fractal->pixelScores[row] = (int *)malloc(sizeof(int) * fractal->width);
      }
   }
}

static void generateRectangle(MandelbrotSet fractal, int startX, int startY, int width, int height) {
   int row, col;

   for (row = startY; row != startY+height; ++row) {
      for (col = startX; col != startX+width; ++col) {
         generateSetPixel(fractal, row, col);
      }
   }
}

// TODO: consider implementing circle tiling optimisation to compare: http://mrob.com/pub/muency/circletiling.html

static void generateDivideAndConquer(MandelbrotSet fractal, int startX, int startY, int width, int height) {
   // Mariani/Silver optimisation algorithm http://mrob.com/pub/muency/marianisilveralgorithm.html
   // may miss cusps narrower than 1 pixel

   int row, col;
   bool canSkip;
   
   int firstRow = startY;
   int lastRow  = startY + height - 1;
   int firstCol = startX;
   int lastCol  = startX + width - 1;

   int newWidth, newHeight;

   if (width < 3 || height < 3) {
      // stopping case, generate the slow way
      generateRectangle(fractal, startX, startY, width, height);
   } else {
      // check top and bottom most rows
      canSkip = generateBlockRow(fractal, firstRow, firstCol, width);
      canSkip = canSkip && generateBlockRow(fractal, lastRow, firstCol, width);

      // check left and right most columns (not including top and bottom rows)
      canSkip = canSkip && generateBlockCol(fractal, firstCol, firstRow+1, height-2);
      canSkip = canSkip && generateBlockCol(fractal, lastCol, firstRow+1, height-2);

      if (canSkip && fractal->pixelScores[firstRow][firstCol] != 0) {
         // pruning case

         // this block is entirely bordered by the same score, which isn't 0
         // fill the rest in with this score
         for (row = firstRow+1; row != firstRow+1+height-2; ++row) {
            for (col = firstCol+1; col != firstCol+1+width-2; ++col) {
               fractal->pixelScores[row][col] = fractal->pixelScores[firstRow][firstCol];
            }
         }
      } else {
         // recursive case
         newWidth = width/2;
         newHeight = height/2;

         // split and generate 4 quadrants
         generateDivideAndConquer(fractal, startX, startY, newWidth, newHeight);
         generateDivideAndConquer(fractal, startX+newWidth, startY, width-newWidth, newHeight);
         generateDivideAndConquer(fractal, startX, startY+newHeight, newWidth, height-newHeight);
         generateDivideAndConquer(fractal, startX+newWidth, startY+newHeight, width-newWidth, height-newHeight);
      }
   }
}

static inline bool generateBlockRow(MandelbrotSet fractal, int row, int colStart, int width) {
   bool isSameColor = true;
   int col = colStart;
   while (isSameColor && col != colStart+width) {
      generateSetPixel(fractal, row, col);
      if (col > 0 && fractal->pixelScores[row][col] != fractal->pixelScores[row][col-1]) {
         isSameColor = false;
      }
      col++;
   }

   return isSameColor;
}

static inline bool generateBlockCol(MandelbrotSet fractal, int col, int rowStart, int height) {
   bool isSameColor = true;
   int row = rowStart;
   while (isSameColor && row != rowStart+height) {
      generateSetPixel(fractal, row, col);
      if (row > 0 && fractal->pixelScores[row][col] != fractal->pixelScores[row-1][col]) {
         isSameColor = false;
      }
      row++;
   }

   return isSameColor; 
}

static inline void generateSetPixel(MandelbrotSet fractal, int row, int col) {
   assert(fractal->pixelScores != NULL);

   mandelbrotCoord coord;
   real halfResolution = fractal->resolution/2.0;

   // generate coordinate, shift to the center of the pixel
   coord.x = fractal->left + (fractal->resolution * col + halfResolution);
   coord.y = fractal->top - (fractal->resolution * row + halfResolution);

   fractal->pixelScores[row][col] = escapeScore(fractal, coord);
}

static inline int escapeScore(MandelbrotSet fractal, mandelbrotCoord coord) {
   int score;
   real xSq, ySq;
   real tempX, x, y;

   // check for a known exit (using a polynomial)
   real xShifted = (coord.x - 0.25);
   real sqCoordY = (coord.y*coord.y);
   real q = xShifted*xShifted + sqCoordY;
   if ((q * (q + xShifted)) < (0.25 * sqCoordY)) {
      // confirmed inside the set
      score = fractal->maxIterations;
   } else {
      // unknown, calculate it slowly

      // TODO: add orbit (never escaping point) detection optimisation: http://mrob.com/pub/muency/orbitdetection.html
      // using Floyd's tortoise/hare cycle detection
      // to early-exit before reaching max iteration

      score = 0;
      x = 0;
      y = 0;
      xSq = 0;
      ySq = 0;
      while (xSq + ySq < ESCAPE_RADIUS_SQ && score != fractal->maxIterations) {
         tempX = xSq - ySq + coord.x;
         y = 2*x*y + coord.y;
         x = tempX;

         xSq = x*x;
         ySq = y*y;
         score++;
      } 
   }

   return score;
}


