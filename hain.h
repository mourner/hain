/* © T. Hain, Nov 1997
 *
 * fill.h
 */
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

#define NUMB_WE 200
#define NUMB_WS 400
//#define FLATNESS .2
#define FLATNESS .0
//#define ALMOST_HORIZONTAL .1
#define ALMOST_HORIZONTAL .0
/* joinType bits */
#define DOWNTORIGHT 1
#define PEAK 2
#define POSCROSSPROD 4
#define UCUSP 8
#define DCUSP 16
#define WINDOW 32
#define ADDED_NODE 64
#define LAST_WE 4

typedef unsigned JoinType;

typedef struct ChainNodeTag {
    float x,y;
    float deltay; /* ynext - y */
    JoinType joinType;
    struct ChainNodeTag *prev, *next, *nextJoin, *prevJoin;
} ChainNode;

typedef struct {
    float x,y;
} Point;

typedef enum {WEDGE_SEQ, END} Opcode;

typedef enum {LEFT, RIGHT, BOTH} WedgeType; /* sign bit = 1 if last element */

typedef struct {
    WedgeType wedgeType; /* type of wedge: LEFT, RIGHT, BOTH */
    float height; /* height of wedge */
    float lCorr, rCorr; /* correction relative to last wedge */
    float lSlope, rSlope; /* (inverse) slopes (delta_x/delta_y) */
} WedgeElement;

typedef struct {
    int opcode; /* type of object */
    float x,y; /* top-left coord of bounding box for all wedge element */
    float pwidth, pht; /* bounding box */
    WedgeElement we[NUMB_WE];
} WedgeSequence;

int compareJoins(const void *pp, const void *qq);

void makeWedgeSequence(int wsNumb, ChainNode *topOfWindow);

void fillPoly(int n, Point v[], WedgeSequence ws[]);

WedgeSequence ws[NUMB_WS];
