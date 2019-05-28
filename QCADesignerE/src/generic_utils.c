//////////////////////////////////////////////////////////
// QCADesigner                                          //
// Copyright 2002 Konrad Walus                          //
// All Rights Reserved                                  //
// Author: Konrad Walus                                 //
// Email: qcadesigner@gmail.com                         //
// WEB: http://qcadesigner.ca/                          //
//////////////////////////////////////////////////////////
//******************************************************//
//*********** PLEASE DO NOT REFORMAT THIS CODE *********//
//******************************************************//
// If your editor wraps long lines disable it or don't  //
// save the core files that way. Any independent files  //
// you generate format as you wish.                     //
//////////////////////////////////////////////////////////
// Please use complete names in variables and fucntions //
// This will reduce ramp up time for new people trying  //
// to contribute to the project.                        //
//////////////////////////////////////////////////////////
// This file was contributed by Gabriel Schulhof        //
// (schulhof@atips.ca).                                 //
//////////////////////////////////////////////////////////
// Contents:                                            //
//                                                      //
// Stuff that wouldn't fit anywhere else.               //
//                                                      //
//////////////////////////////////////////////////////////

#include <math.h>
#include <stdlib.h>
#ifndef WIN32
  #include <unistd.h>
#endif /* ndef WIN32 */
#include <glib.h>
#include "exp_array.h"
#include "generic_utils.h"
#include "global_consts.h"

#ifdef GTK_GUI
typedef struct
  {
  char *pszCmdLine ;
  char *pszTmpFName ;
  } RUN_CMD_LINE_ASYNC_THREAD_PARAMS ;

static gpointer RunCmdLineAsyncThread (gpointer p) ;
#endif /* def GTK_GUI */

// Causes a rectangle of width (*pdRectWidth) and height (*pdRectHeight) to fit inside a rectangle of
// width dWidth and height dHeight.  Th resulting pair ((*px),(*py)) holds the coordinates of the
// upper left corner of the scaled rectangle wrt. the upper left corner of the given rectangle.
void fit_rect_inside_rect (double dWidth, double dHeight, double *px, double *py, double *pdRectWidth, double *pdRectHeight)
  {
  double dAspectRatio, dRectAspectRatio ;

  if (0 == dWidth || 0 == dHeight || 0 == *pdRectWidth || 0 == *pdRectHeight) return ;

  dAspectRatio = dWidth / dHeight ;
  dRectAspectRatio = *pdRectWidth / *pdRectHeight ;

  if (dRectAspectRatio > dAspectRatio)
    {
    *px = 0 ;
    *pdRectWidth = dWidth ;
    *pdRectHeight = *pdRectWidth / dRectAspectRatio ;
    *py = (dHeight - *pdRectHeight) / 2 ;
    }
  else
    {
    *py = 0 ;
    *pdRectHeight = dHeight ;
    *pdRectWidth = *pdRectHeight * dRectAspectRatio ;
    *px = (dWidth - *pdRectWidth) / 2 ;
    }
  }

// Convert a long long value to decimal, hexadecimal, or binary
char *strdup_convert_to_base (long long value, int base)
  {
  if (10 == base)
    return g_strdup_printf ("%llu", value) ;
  else
  if (16 == base)
    return g_strdup_printf ("%llX", value) ;
  else
  if (2 == base)
    {
    char *psz = NULL ;
    EXP_ARRAY *str = NULL ;

    if (0 == value)
      return g_strdup ("0") ;

    str = exp_array_new (sizeof (char), 1) ;

    while (value)
      {
      exp_array_insert_vals (str, NULL, 1, 1, -1) ;
      exp_array_index_1d (str, char, str->icUsed - 1) = (value & 0x1) ? '1' : '0' ;
      value = value >> 1 ;
      }

    exp_array_insert_vals (str, NULL, 1, 1, -1) ;
    exp_array_index_1d (str, char, str->icUsed - 1) = 0 ;
    psz = g_strreverse (g_strndup (str->data, str->icUsed)) ;
    exp_array_free (str) ;
    return psz ;
    }
  return NULL ;
  }

// Decide whether the slope difference between two line segments is significant enough for the three points
// demarcating the two line segments not to be considered collinear
//                     (dx2,dy2)
//                        /
//                       /
//(dx0,dy0)             /
//    _________________/
//                   (dx1,dy1)
gboolean LineSegmentCanBeSkipped (double dx0, double dy0, double dx1, double dy1, double dx2, double dy2, double dMaxSlopeDiff)
  {
  if (dx0 == dx1 && dx1 == dx2) return TRUE ;
  if (dy0 == dy1 && dy1 == dy2) return TRUE ;

  return (fabs ((dy1 - dy0) / (dx1 - dx0) - (dy2 - dy1) / (dx2 - dx1)) < dMaxSlopeDiff) ;
  }

#ifdef GTK_GUI
void RunCmdLineAsync (char *pszCmdLine, char *pszTmpFName)
  {
  RUN_CMD_LINE_ASYNC_THREAD_PARAMS *prclap = g_malloc0 (sizeof (RUN_CMD_LINE_ASYNC_THREAD_PARAMS)) ;

  prclap->pszCmdLine = g_strdup (pszCmdLine) ;
  prclap->pszTmpFName = (NULL == pszTmpFName ? NULL : g_strdup (pszTmpFName)) ;

  if (!g_thread_supported ()) g_thread_init (NULL) ;

  g_thread_create ((GThreadFunc)RunCmdLineAsyncThread, (gpointer)prclap, FALSE, NULL) ;
  }

static gpointer RunCmdLineAsyncThread (gpointer p)
  {
  RUN_CMD_LINE_ASYNC_THREAD_PARAMS *prclap = (RUN_CMD_LINE_ASYNC_THREAD_PARAMS *)p ;
#ifdef WIN32
  STARTUPINFO si ;
  PROCESS_INFORMATION pi ;

  memset (&si, 0, sizeof (si)) ;
  memset (&pi, 0, sizeof (pi)) ;
  si.cb = sizeof (STARTUPINFO) ;

  if (CreateProcess (NULL, prclap->pszCmdLine, NULL, NULL, FALSE, DETACHED_PROCESS,
    NULL, NULL, &si, &pi))
    {
    WaitForSingleObject (pi.hProcess, INFINITE) ;
    CloseHandle (pi.hProcess) ;
    CloseHandle (pi.hThread) ;
    }
#else
  system (prclap->pszCmdLine) ;
#endif
  g_free (prclap->pszCmdLine) ;
  if (NULL != prclap->pszTmpFName)
    {
#ifdef WIN32
    DeleteFile (prclap->pszTmpFName) ;
#else
    unlink (prclap->pszTmpFName) ;
#endif /* def WIN32 */
    g_free (prclap->pszTmpFName) ;
    }
  g_free (prclap) ;

  return NULL ;
  }
#endif /* def GTK_GUI */

char *get_enum_string_from_value (GType enum_type, int value)
  {
  GEnumClass *klass = g_type_class_ref (enum_type) ;
  GEnumValue *val = NULL ;

  if (NULL == klass) return g_strdup_printf ("%d", value) ;

  if (NULL == (val = g_enum_get_value (klass, value)))
    return g_strdup_printf ("%d", value) ;
  else
    return g_strdup (val->value_name) ;

  g_type_class_unref (klass) ;
  }

int get_enum_value_from_string (GType enum_type, char *psz)
  {
  GEnumClass *klass = g_type_class_peek (enum_type) ;
  GEnumValue *val = NULL ;

  if (NULL == klass) return g_ascii_strtod (psz, NULL) ;

  if (NULL == (val = g_enum_get_value_by_name (klass, psz)))
    return g_ascii_strtod (psz, NULL) ;
  else
    return val->value ;
  }


/* Global variables */
static int clocks = NUMBER_OF_CLOCKS_DEFAULT;

/**
 * Set the number of clocks.
 * @param nClocks Number of clocks
 */
void setNumberOfClocks(int nClocks) {
  clocks = nClocks;
}

/**
 * Gets the number of clocks
 * @return Number of clocks
 */
int getNumberOfClocks(void) {
  return clocks;
}

/**
 * Get the number of meta-clocks (shifted clocks)
 * @return Number of meta-clocks.
 */
int getNumberOfMetaClocks(void) {
  return 4;
}

/**
 * Get the number of clocks in total.
 * #clocks x #meta-clocks
 * @return Number of clocks in total
 */
int getNumberOfTotalClocks(void) {
  return getNumberOfClocks() * getNumberOfMetaClocks();
}

// -------------------------------------------------------------------------------------------------

static const double PARAMETER_B_03_CLOCKS[] = {
  -6.18540,
   0.00000,
   6.18540
};

static const double PARAMETER_B_04_CLOCKS[] = {
  -6.55761,
  -2.78716,
   2.78716,
   6.55761
};

static const double PARAMETER_B_05_CLOCKS[] = {
  -6.72463,
  -4.26100,
   0.00000,
   4.26100,
   6.72463
};

static const double PARAMETER_B_06_CLOCKS[] = {
  -6.81283,
  -5.10274,
  -1.88804,
   1.88804,
   5.10274,
   6.81283
};

static const double PARAMETER_B_07_CLOCKS[] = {
  -6.86477,
  -5.61943,
  -3.15723,
   0.00000,
   3.15723,
   5.61943,
   6.86477
};

static const double PARAMETER_B_08_CLOCKS[] = {
  -6.89784,
  -5.95578,
  -4.03129,
  -1.42394,
   1.42394,
   4.03129,
   5.95578,
   6.89784
};

static const double PARAMETER_B_09_CLOCKS[] = {
  -6.92015,
  -6.18540,
  -4.65102,
  -2.49251,
   0.00000,
   2.49251,
   4.65102,
   6.18540,
   6.92015
};

static const double PARAMETER_B_10_CLOCKS[] = {
  -6.93590,
  -6.34834,
  -5.10275,
  -3.30224,
  -1.14208,
   1.14208,
   5.10275,
   3.30224,
   6.34834,
   6.93590
};

static const double PARAMETER_B_11_CLOCKS[] = {
  -6.94742,
  -6.46773,
  -5.44029,
  -3.92456,
  -2.05470,
   0.00000,
   2.05470,
   3.92456,
   5.44029,
   6.46773,
   6.94742
};

static const double PARAMETER_B_12_CLOCKS[] = {
  -6.95610,
  -6.55761,
  -5.69808,
  -4.41009,
  -2.78716,
  -0.95306,
   0.95306,
   2.78716,
   4.41009,
   5.69808,
   6.55761,
   6.95610
};

static const double PARAMETER_B_13_CLOCKS[] = {
  -6.96280,
  -6.62683,
  -5.89877,
  -4.79440,
  -3.37956,
  -1.74609,
   0.00000,
   1.74609,
   3.37956,
   4.79440,
   5.89877,
   6.62683,
   6.96280
};

static const double PARAMETER_B_14_CLOCKS[] = {
  -6.96808,
  -6.68119,
  -6.05768,
  -5.10275,
  -3.86295,
  -2.40733,
  -0.81760,
   0.81760,
   2.40733,
   3.86295,
   5.10275,
   6.05768,
   6.68119,
   6.96808
};

static const double PARAMETER_B_15_CLOCKS[] = {
  -6.97231,
  -6.72463,
  -6.18540,
  -5.35322,
  -4.26100,
  -2.96115,
  -1.51737,
   0.00000,
   1.51737,
   2.96115,
   5.35322,
   6.18540,
   4.26100,
   6.72463,
   6.97231
};

static const double PARAMETER_B_16_CLOCKS[] = {
  -6.97575,
  -6.75984,
  -6.28942,
  -5.55901,
  -4.59170,
  -3.42760,
  -2.11687,
  -0.71579,
   0.71579,
   2.11687,
   3.42760,
   4.59170,
   5.55901,
   6.28942,
   6.75984,
   6.97575
};

static const double PARAMETER_B_17_CLOCKS[] = {
  -6.97859,
  -6.78878,
  -6.37515,
  -5.72982,
  -4.86876,
  -3.82284,
  -2.63187,
  -1.34127,
   0.00000,
   1.34127,
   2.63187,
   3.82284,
   4.86876,
   5.72982,
   6.37515,
   6.78878,
   6.97859
};

static const double PARAMETER_B_18_CLOCKS[] = {
  -6.98096,
  -6.81282,
  -6.44657,
  -5.87296,
  -5.10275,
  -4.15979,
  -3.07590,
  -1.88804,
  -0.63649,
   0.63649,
   1.88804,
   3.07590,
   5.10275,
   5.87296,
   4.15979,
   6.44657,
   6.81283,
   6.98096
};

static const double PARAMETER_B_19_CLOCKS[] = {
  -6.98296,
  -6.83302,
  -6.50664,
  -5.99393,
  -5.30183,
  -4.44876,
  -3.46035,
  -2.36684,
  -1.20150,
   0.00000,
   1.20150,
   2.36684,
   3.46035,
   4.44876,
   5.30183,
   5.99393,
   6.50664,
   6.83302,
   6.98296
};

static const double PARAMETER_B_20_CLOCKS[] = {
  -6.98465,
  -6.85014,
  -6.55761,
  -6.09698,
  -5.47238,
  -4.69804,
  -3.79466,
  -2.78716,
  -1.70335,
  -0.57300,
   0.57300,
   1.70335,
   2.78716,
   3.79466,
   4.69804,
   5.47238,
   6.09698,
   6.55761,
   6.85014,
   6.98465
};

static const double * PARAMETERS_B_03_TO_07_CLOCKS[] = {
  PARAMETER_B_03_CLOCKS,
  PARAMETER_B_04_CLOCKS,
  PARAMETER_B_05_CLOCKS,
  PARAMETER_B_06_CLOCKS,
  PARAMETER_B_07_CLOCKS,
  PARAMETER_B_08_CLOCKS,
  PARAMETER_B_09_CLOCKS,
  PARAMETER_B_10_CLOCKS,
  PARAMETER_B_11_CLOCKS,
  PARAMETER_B_12_CLOCKS,
  PARAMETER_B_13_CLOCKS,
  PARAMETER_B_14_CLOCKS,
  PARAMETER_B_15_CLOCKS,
  PARAMETER_B_16_CLOCKS,
  PARAMETER_B_17_CLOCKS,
  PARAMETER_B_18_CLOCKS,
  PARAMETER_B_19_CLOCKS,
  PARAMETER_B_20_CLOCKS
};

double getParameterB(int clock, int numberOfClocks) {
  return (PARAMETERS_B_03_TO_07_CLOCKS[numberOfClocks-3])[clock];
}

//------------------------------------------------------------------------------

int numberOfClocksUsed(DESIGN *design) {
    GList *llItr;
    GList *llItrObj;
    int maxClock;
    int clock;
  
    if(design == NULL) {
        return -1;
    }
  
    maxClock = 0;
    llItr = design->lstLayers;
    while(llItr != NULL) {  // Itera pela lista de layers.
        if(QCAD_LAYER(llItr->data)->type == LAYER_TYPE_CELLS) {  // Se é uma layer de células.
            llItrObj = QCAD_LAYER(llItr->data)->lstObjs;
            while(llItrObj != NULL) {  // Itera pela lista de células da layer.
                if(NULL != llItrObj->data) {
                    clock = (((QCADCell*)llItrObj->data)->cell_options.clock) % getNumberOfClocks();
                    if(clock > maxClock) {
                        maxClock = clock;
                    }
                }
                llItrObj = llItrObj->next;
            }
        }
        llItr = llItr->next;
    }
    // Retorna o maior clock encontrado dentre todas as células.
    return (maxClock + 1);
}

void updateNumberOfClocksUsed(DESIGN *design, int oldNumberOfClocks) {
    GList *llItr;
    GList *llItrObj;
    int maxClock;
    int clock;
    int clk, metaClk;
  
    if(design == NULL) {
        return;
    }
  
    maxClock = 0;
    llItr = design->lstLayers;
    while(llItr != NULL) {  // Itera pela lista de layers.
        if(QCAD_LAYER(llItr->data)->type == LAYER_TYPE_CELLS) {  // Se é uma layer de células.
            llItrObj = QCAD_LAYER(llItr->data)->lstObjs;
            while(llItrObj != NULL) {  // Itera pela lista de células da layer.
                if(NULL != llItrObj->data) {
                    clock = ((QCADCell*)llItrObj->data)->cell_options.clock;
                    clk = clock % oldNumberOfClocks;
                    metaClk = clock / oldNumberOfClocks;
                    ((QCADCell*)llItrObj->data)->cell_options.clock = getNumberOfClocks() * metaClk + clk;
                }
                llItrObj = llItrObj->next;
            }
        }
        llItr = llItr->next;
    }
}