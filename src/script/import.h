/*===================================================================*/
/* Import High-level routines                                        */
/*===================================================================*/

#include "jinete/base.h"

#include "effect/colcurve.h"
#include "raster/frame.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"

/* sprite.lua */
void CloseSprite(void);

/* layer.lua */
const char *GetUniqueLayerName(void);

Layer *NewLayer(const char *name, int x, int y, int w, int h);
Layer *NewLayerSet(const char *name);
void RemoveLayer(void);

/* remfrm.lua */
void RemoveFrame(Layer *layer, Frame *frame);

/* mask.lua */
void MaskAll(void);
void DeselectMask(void);
void ReselectMask(void);
void InvertMask(void);
void StretchMaskBottom(void);

/* effect.lua */
void ConvolutionMatrix(const char *name, bool r, bool g, bool b, bool k, bool a, bool index);
void ConvolutionMatrixRGB(const char *name);
void ConvolutionMatrixRGBA(const char *name);
void ConvolutionMatrixGray(const char *name);
void ConvolutionMatrixGrayA(const char *name);
void ConvolutionMatrixIndex(const char *name);
void ConvolutionMatrixAlpha(const char *name);

void _ColorCurve(Curve *curve, bool r, bool g, bool b, bool k, bool a, bool index);
void _ColorCurveRGB(Curve *curve);
void _ColorCurveRGBA(Curve *curve);
void _ColorCurveGray(Curve *curve);
void _ColorCurveGrayA(Curve *curve);
void _ColorCurveIndex(Curve *curve);
void _ColorCurveAlpha(Curve *curve);

/* void ColorCurve(int array[], bool r, bool g, bool b, bool k, bool a, bool index); */
/* void ColorCurveRGB(int array[]); */
/* void ColorCurveRGBA(int array[]); */
/* void ColorCurveGray(int array[]); */
/* void ColorCurveGrayA(int array[]); */
/* void ColorCurveIndex(int array[]); */
/* void ColorCurveAlpha(int array[]); */
