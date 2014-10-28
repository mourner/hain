/* © T. Hain, Nov 1997
 *
 * Decompose a simple polygon into trapezoid sequences
 */

#include <limits.h>
#include "hain.h"

void fillPoly(int n, Point v[], WedgeSequence ws[]) {
/*----------------------------------------------
 * Assumptions:
 * 1. Vertices are given in clockwise order
 * 2. There are no crossing edges.
 *
 * Note:
 * "Up" and "down" assume the y-axis is in "upward" direction.
 */

    int i,j, /* general index */
        numJoins = 0, /* number of Joins */
        wsNumb = 0; /* number of wedge sequences */

    float crossprod, /* cross product at vertex */
          xmax, xtmp;

    ChainNode *c, /* start of vertex loop */
              *p, *q, *r, *keepq, /* p,q,r are general ChainNode pointers.*/
              *qxmax, *qxmin,
              **sortedJoins, /* array of pointers to Joins */
              *topOfWindow, *botOfWindow,
              *p_in, *p_out, *q_in, *q_out;

    JoinType joinType; /* current join type (reduces need for indirection) */

    /* Allocate space for initial chain nodes.
    * This vertices are sequentially stored in a circular list of doubly
    * linked chain nodes. */
    c = (ChainNode*) malloc(n * sizeof(ChainNode));

    /* Insert vertex coordinates, delta-y values and create doubly-linked circular list.
    * Note p->deltay = p->next->y - p->y */
    for (i = 0, p = c, q = c+1; i < n-1 ; i++, p++, q++) {
        p->next = q;
        q->prev = p;
        p->x = v[i].x;
        p->y = v[i].y;
        p->deltay = v[i+1].y - v[i].y;
    }

    /* close vertex loop */
    p->next = c;
    p->x = v[i].x;
    p->y = v[i].y;
    p->deltay = v[0].y - v[i].y;
    c->prev = p;

    /* Make almost horizontal edges exactly horizontal (This eliminates a trapexoid.) */
    q = c;
    r = c->next;
    do {
        if ((fabs(q->deltay) <= ALMOST_HORIZONTAL) && !(q->deltay == (float)0)) {
            /* force horizontal */
            r->y = q->y;
            r->deltay = r->next->y - r->y;
            q->deltay = (float)0;
        }
        q = r;
        r = r->next;
    } while (q != c);

    /* Remove center vertex of three (almost) in-line vertices.
     *
     * This is done by looking at the absolute value of the cross product
     * of the edges incident on the vertex to be (potentially) eliminated.
     * (This may not be the best way to measure "flatness".)
     */
    /* 1. Find any starting vertex (one that will not be eliminated) */
    for (p=c->prev, q=c, r=c->next; ; p = p->next, r = r->next) {
        crossprod = q->x * (r->y - p->y) - p->x * q->deltay - r->x * p->deltay;
        if (fabs(crossprod) > FLATNESS) {
            c = q;
            break;
        }
        q = r;
        if (q == c) { // This means all the points in the polygon are collinear.
            // Thus it boils down polygon with two vertices.
            ws[0].opcode = !WEDGE_SEQ;
            return;
        }
    }
    /* 2. Eliminate center vertex of all in-line triples */
    p=c->prev;
    q=c;
    r=c->next;
    i = 0;
    do {
        /* cross product of edges on either side of current point, q */
        crossprod = q->x * (r->y - p->y) - p->x * q->deltay - r->x * p->deltay;
        if (fabs(crossprod) <= FLATNESS) {
            p->next = r;
            r->prev = p;
            p->deltay = r->y - p->y;
        } else {
            q->joinType = (crossprod > 0)? POSCROSSPROD: 0;
            p = q;
        }
        q = r;
        r = r->next;
        i++;
    } while (q != c);

    /* Find beginning of next down-chain */

    /* 1. find first actual rising edge */
    for (q = c; q->deltay <= 0; q = q->next) {};

    /* 2. find end of this up-chain (i.e., beginning of down-chain.) */
    for (q = q->next; q->deltay >= 0; q = q->next) {};

    /* Note that a horizontal edge at the top of a chain is considered to belong to the down-chain */
    if ((q->prev->deltay == 0) && !(q->joinType & POSCROSSPROD)) q = q->prev;
    /* q now points to a peak join (left of horizontal edge) */

    /* Collect all chains and joins */
    keepq = q;
    do {
        /* Find down-chain, and right-most node of down-chain */
        for (p = qxmax = q, q = p->next; q->deltay <=0; q = q->next) {
            if (q->x >= qxmax->x) qxmax = q;
        }

        if (q->prev->deltay == 0) {
            if (q->joinType & POSCROSSPROD) q = q->prev;

        } else if (q->x >= qxmax->x) qxmax = q;

        /* vertical down-chains are default DOWNTORIGHT.
        * p now points to top of chain (left of horizontal edge.)
        * q now points to bottom of chain (left of horizontal edge),
        * qxmax points to node with rightmost vertex */

        if ( (qxmax != p) && (qxmax != q) ) {
            /* Create a DCUSP join, and set join links */
            p->nextJoin = qxmax;
            qxmax->prevJoin = p;
            qxmax->nextJoin = q;
            q->prevJoin = qxmax;
            p->joinType |= DOWNTORIGHT;
            numJoins++;
            qxmax->joinType = DCUSP; /* note that POSCROSSPROD is set to false*/

        } else {
            p->nextJoin = q;
            q->prevJoin = p;
            if (qxmax == q) {
                p->joinType |= DOWNTORIGHT;
                if (q->joinType & POSCROSSPROD)
                    q->joinType |= DOWNTORIGHT;
                else
                    q->joinType = DCUSP;/* note that POSCROSSPROD is set to false*/
            } else /* qxmax == p */
                if ( !(p->joinType & POSCROSSPROD) )
                    p->joinType = DCUSP;/* note that POSCROSSPROD is set to false*/
        }
        numJoins++;
        p->joinType |= PEAK;

        /* handle up-chain */
        numJoins++;
        p = q; /* p is beginning of up-chain */

        for (qxmin = p, q = q->next; q->deltay >= 0; q = q->next) {
            if (q->x < qxmin->x) qxmin = q;
        }
        if (q->prev->deltay == 0) {
            if (!(q->joinType & POSCROSSPROD)) q = q->prev;

        } else if (q->x < qxmin->x) qxmin = q;

        if ((qxmin != p) && (qxmin != q)) {
            p->nextJoin = qxmin;
            qxmin->prevJoin = p;
            qxmin->nextJoin = q;
            q->prevJoin = qxmin;
            qxmin->joinType = UCUSP;
            numJoins++;

        } else {
            p->nextJoin = q;
            q->prevJoin = p;
        }

    } while (q != keepq);

    /* sort joins */
    sortedJoins = (ChainNode**) malloc(numJoins * sizeof(ChainNode*));

    for (q = keepq, i = 0; i < numJoins; q = q->nextJoin, i++) sortedJoins[i] = q;
    qsort((void*)sortedJoins, (size_t)numJoins, sizeof(ChainNode*),
          compareJoins);

    /* process joins in x-order */
    for (i = 0; i < numJoins; i++) {
        q = sortedJoins[i];
        joinType = q->joinType;

        /* if at left of up-chain, mark top of chain as WINDOW */
        if (joinType & PEAK) joinType = q->joinType |= WINDOW;
        else q->nextJoin->joinType |= WINDOW;

        if (joinType & POSCROSSPROD) {
            switch (joinType & (PEAK|DOWNTORIGHT)) {

            case PEAK /* & !DOWNTORIGHT */:
                topOfWindow = q->nextJoin->nextJoin;
                break;
            case /* !PEAK & */ DOWNTORIGHT:
                topOfWindow = q->prevJoin;
                break;

            default: /* PEAK same as DOWNTORIGHT. */
                /* Search for rightmost window on left of, and
                * vertically spanning, current join.
                * --search (backward) for first window with top above q->y
                */
                for ( r= q->prevJoin; !(r->joinType & WINDOW) || (r->y <= q->y); r = r->prevJoin ) {}

                /* --search (backward) for next window with bottom below q->y*/
                for ( r= r->prevJoin; (r->joinType & PEAK) || (r->y >= q->y); r = r->prevJoin ) {}

                /* We are now at the bottom of first candidate window
                * (vertically spanning q->y, on left of current join)
                * -- find window intersection
                */
                for (p = r->nextJoin->prev; p->y > q->y; p = p->prev) {}

                xtmp = (p->x + (p->next->x - p->x)/p->deltay * (q->y - p->y));
                if (xtmp < q->x) {
                    xmax = xtmp;
                    topOfWindow = r->nextJoin;
                } else xmax = -FLT_MAX;

                for (;;) {
                    /* search (backward) for next window with top above q->y */
                    for (r = r->prevJoin; (r != q) && (r != q->nextJoin) &&
                            (!(r->joinType & WINDOW) || (r->y <= q->y)); r = r->prevJoin) {}

                    if ((r == q) || (r == q->nextJoin)) break;

                    /* search (bckwd) for next window with bottom below q->y */
                    for (r = r->prevJoin; (r != q) && (r != q->nextJoin) &&
                            ((r->joinType & PEAK) || (r->y >= q->y)); r = r->prevJoin) {}

                    /* We are now at the bottom of a candidate window
                    * (vertically spanning q->y, and potentially on left)
                    * -- find window intersection */
                    for (p = r->nextJoin->prev; p->y > q->y; p = p->prev) {}

                    xtmp = (p->x + (p->next->x - p->x) / p->deltay * (q->y - p->y));
                    if ((xtmp < q->x) && (xtmp >= xmax)) {
                        xmax = xtmp;
                        topOfWindow = r->nextJoin;
                    }
                }
            } /* switch */

            botOfWindow = topOfWindow->prevJoin;

            /* find vertical position of current join in current window */
            for (p = topOfWindow->prev; p->y > (q->y + ALMOST_HORIZONTAL); p = p->prev) {}

            /* see if new node is needed (join does not align vertically with a window node */
            if (fabs(p->y - q->y) > ALMOST_HORIZONTAL) {
                /* q lines up with edge */
                p_in = (ChainNode*) malloc(sizeof(ChainNode));
                p_out = (ChainNode*) malloc(sizeof(ChainNode));

                p_in->joinType = p_out->joinType = ADDED_NODE;
                p_in->deltay = (float)0;
                p_in->y = p_out->y = q->y;
                p_in->x = p_out->x = p->x
                                     + (p->next->x - p->x)/p->deltay * (q->y - p->y);
                p_out->next = p->next;
                p->next->prev = p_out;
                p_out->deltay = p_out->next->y - p_out->y;

                p_out->nextJoin = topOfWindow;
                p_in->prevJoin = botOfWindow;
                topOfWindow->prevJoin = p_out;
                p->next = p_in;
                p_in->prev = p;
                p->deltay = p_in->y - p->y;
            } else {
                /* q lines up with vertex */
                if (p->prev->deltay == (float)0) p_in = p->prev;
                else {
                    p_in = (ChainNode*) malloc(sizeof(ChainNode));
                    p_in->joinType = ADDED_NODE;
                    p_in->deltay = (float)0;
                    p_in->prev = p->prev;
                    p->prev = p->prev->next = p_in;
                    p_in->deltay = (float)0;
                    p_in->x = p->x;
                    p_in->y = p->y;
                }
                p_in->prevJoin = botOfWindow;
                p_out = NULL;
            }
            /* relink to form two disjoint polygons */
            if (joinType & PEAK) { /* peak join */
                if (q->prev->deltay != (float)0) {
                    q_in = (ChainNode*) malloc(sizeof(ChainNode));
                    q_in->joinType = ADDED_NODE;
                    q_in->deltay = (float)0;
                    q_in->prev = q->prev;
                    q->prev->next = q_in;
                    q_in->y = q->y;
                    q_in->x = q->x;

                } else q_in = q->prev;

                p_in->next = q;
                q->prev = p_in;
                if (botOfWindow->prevJoin == q) botOfWindow->prevJoin = p_in;

                p_in->nextJoin = q->nextJoin;
                p_in->joinType |= WINDOW;
                q->prevJoin->nextJoin = topOfWindow;
                q->nextJoin->prevJoin = p_in;

                topOfWindow->prevJoin = q->prevJoin;
                botOfWindow->nextJoin = p_in;

                if (p_out) {
                    /* peak join lines up horizontally with window edge */
                    p_out->prev = q_in;
                    q_in->next = p_out;

                } else { /* peak join lines up with window vertex */
                    p->prev = q_in;
                    q_in->next = p;
                    q->deltay = q->next->y - q->y;
                    q_in->prev->deltay = q_in->y - q_in->prev->y;
                }

                /* if at right side of down-chain, emit wedge sequence */
                if (!(joinType & DOWNTORIGHT)) makeWedgeSequence(wsNumb++, p_in);

            } else { /* valley join */
                if (q->deltay != (float)0) {
                    q_out = (ChainNode*) malloc(sizeof(ChainNode));
                    q_out->joinType = ADDED_NODE;
                    q_out->next = q->next;
                    q->next->prev = q_out;
                    q_out->y = q->y;
                    q_out->x = q->x;

                } else q_out = q->next;

                q->nextJoin->prevJoin = botOfWindow;
                botOfWindow->nextJoin = q->nextJoin;
                q_out->prev = p_in;
                p_in->next = q_out;
                q->deltay = (float)0;

                if (p_out) {
                    /* valley join lines up horizontally with window edge */
                    if (topOfWindow->nextJoin == q) topOfWindow->nextJoin = p_out;
                    else q->prevJoin->nextJoin = p_out;

                    p_out->prev = q;
                    q->next = p_out;
                    p_out->prevJoin = q->prevJoin;
                    topOfWindow->prevJoin = p_out;
                    p_out->nextJoin = topOfWindow;

                } else {
                    /* valley join lines up horizontally with window vertex */
                    if (topOfWindow->nextJoin == q) topOfWindow->nextJoin = p;
                    else q->prevJoin->nextJoin = p;

                    q->next = p;
                    p->prev = q;
                    p->prevJoin = q->prevJoin;

                    topOfWindow->prevJoin = p;
                    p->nextJoin = topOfWindow;
                    q->prev->deltay = q->y - q->prev->y;
                }
                q_out->deltay = q_out->next->y - q_out->y;
                /* if at right side of down-chain, emit wedge sequence */
                if (joinType & DOWNTORIGHT) makeWedgeSequence(wsNumb++, topOfWindow);
            }
        } else {
            if (joinType & DCUSP) { /* Handle DCUSP case */
                if (joinType & PEAK) makeWedgeSequence(wsNumb++, q);
                else makeWedgeSequence(wsNumb++, q->prevJoin);

            } else if (joinType &UCUSP) {/* Handle UCUSP case */
                q->nextJoin->prevJoin = q->prevJoin;
                q->prevJoin->nextJoin = q->nextJoin;

            } else if (joinType & PEAK) {
                if (!(joinType & DOWNTORIGHT)) makeWedgeSequence(wsNumb++, q);

            } else {
                if (joinType & DOWNTORIGHT) makeWedgeSequence(wsNumb++, q->nextJoin);
            }
        }
    }
    ws[wsNumb].opcode = END;
}
/*-------------------------------------------------------*/
int compareJoins( const void *pp, const void *qq)
{
    ChainNode *p = *((ChainNode**)pp), *q = *((ChainNode**)qq);
    if (q->x == p->x) return (q->y > p->y)? 1: -1; /* ties are done in decreasing y value */
    else return (p->x > q->x)? 1: -1;
}
/*-------------------------------------------------------*/
void makeWedgeSequence(int wsNumb, ChainNode *topOfWindow)
{
    float bb_xmin, bb_xmax; /* bounding box for wedge sequence */
    float prevy;
    int i = 0; /* wedge element index */
    ChainNode *botOfWindow = topOfWindow->prevJoin;
    ChainNode *p, *q;
    /* Preprocess chains to reduce number of wedges of left and right
    * vertices almost align
    */
    /* p = q = topOfWindow;
     do {
     if ((q->deltay == (float)0) || (p->y < q->y - ALMOST_HORIZONTAL))
     q = q->next;
     else if (p->y > q->y + ALMOST_HORIZONTAL
     p = p->prev;
     else {
     q->y = p->y;
     if (q->prev->deltay == (float)0) {
     q->prev->y = p->y;
     q->prev->prev->deltay = q->prev->y - q->prev->prev->y;
     } else
     q->prev->deltay = q->y - q->prev->y;
     q->deltay = q->next->y - q->y;
     q = q->next;
     p = p->prev;
     }
     } while (p != q);
    */

    ws[wsNumb].opcode = WEDGE_SEQ;
    ws[wsNumb].y = topOfWindow->y;
    ws[wsNumb].pht = topOfWindow->y - botOfWindow->y;
    /* find x-range of wedge sequence (for bounding box) */
    bb_xmin = botOfWindow->x;
    for (p = topOfWindow; p != topOfWindow->nextJoin; p = p->prev)
        if (p->x < bb_xmin)
            bb_xmin = p->x;
    bb_xmax = topOfWindow->nextJoin->x;
    for (q = topOfWindow; q != topOfWindow->nextJoin; q = q->next)
        if (q->x > bb_xmax)
            bb_xmax = q->x;
    ws[wsNumb].x = bb_xmin;
    ws[wsNumb].pwidth = bb_xmax - bb_xmin;
    p = q = topOfWindow;
    ws[wsNumb].we[0].wedgeType = BOTH;
    ws[wsNumb].we[0].lCorr = p->x - bb_xmin;
    if (p->deltay == (float)0) {
        q = q->next;
        ws[wsNumb].we[0].rCorr = q->x - bb_xmin;
    } else
        ws[wsNumb].we[0].rCorr = ws[wsNumb].we[0].lCorr;
    ws[wsNumb].we[0].lSlope = (p->x - p->prev->x)/p->prev->deltay;
    ws[wsNumb].we[0].rSlope = -(q->x - q->next->x)/q->deltay;
    for (prevy = topOfWindow->y, q = q->next, p = p->prev; p != q; ) {
        i++;
        if (p->y < (q->y - ALMOST_HORIZONTAL)) {
            ws[wsNumb].we[i].wedgeType = RIGHT;
            ws[wsNumb].we[i-1].height = prevy - q->y;
            prevy = q->y;
            if (q->deltay == (float)0) {
                q = q->next;
                ws[wsNumb].we[i].rCorr = q->x - q->prev->x;
            } else
                ws[wsNumb].we[i].rCorr = (float)0;
            ws[wsNumb].we[i].rSlope = -(q->x - q->next->x)/q->deltay;
            q = q->next;
        } else if (p->y > (q->y + ALMOST_HORIZONTAL)) {
            ws[wsNumb].we[i].wedgeType = LEFT;
            ws[wsNumb].we[i-1].height = prevy - p->y;
            prevy = p->y;
            if (p->prev->deltay == (float)0) {
                p = p->prev;
                ws[wsNumb].we[i].lCorr = p->x - p->next->x;
            } else
                ws[wsNumb].we[i].lCorr = (float)0;
            ws[wsNumb].we[i].lSlope = (p->x - p->prev->x)/p->prev->deltay;
            p = p->prev;
        } else {

            ws[wsNumb].we[i-1].height = prevy - p->y; /*pick left vertex height*/
            if (q->deltay == (float)0) {
                q = q->next;
                if (p == q) { /* if bottom of sequence is horizontal */
                    i--;
                    break;
                }
                ws[wsNumb].we[i].rCorr = q->x - q->prev->x;
            } else
                ws[wsNumb].we[i].rCorr = (float)0;
            ws[wsNumb].we[i].wedgeType = BOTH;
            prevy = p->y;
            ws[wsNumb].we[i].rSlope = -(q->x - q->next->x)/q->deltay;
            q = q->next;
            if (p->prev->deltay == (float)0) {
                p = p->prev;
                ws[wsNumb].we[i].lCorr = p->x - p->next->x;
            } else
                ws[wsNumb].we[i].lCorr = (float)0;
            ws[wsNumb].we[i].lSlope = (p->x - p->prev->x)/p->prev->deltay;
            p = p->prev;
        }
    }
    ws[wsNumb].we[i].height = prevy - botOfWindow->y;
    ws[wsNumb].we[i].wedgeType |= LAST_WE;
}
