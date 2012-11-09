#define DEFAULT_MAX_ITERATIONS 255

typedef struct mandelbrotSetData *MandelbrotSet;

typedef long double real;

typedef struct {
   real x;
   real y;
} mandelbrotCoord;

MandelbrotSet createMandelbrotSet(int width, int height);

void freeMandelbrotSet(MandelbrotSet fractal);

void MandelbrotSet_setPosition(MandelbrotSet fractal, mandelbrotCoord center, int zoom);


void MandelbrotSet_generate(MandelbrotSet fractal);

void MandelbrotSet_fastGenerate(MandelbrotSet fractal);

// returns a borrowed reference (freed when fractal is freed)
int **MandelbrotSet_getScores(MandelbrotSet fractal);

void MandelbrotSet_setMaxIterations(MandelbrotSet fractal, int maxIterations);