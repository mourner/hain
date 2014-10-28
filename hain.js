
if (typeof module !== 'undefined') module.exports = hain;

var FLATNESS = 0,
    ALMOST_HORIZONTAL = 0,

    DOWNTORIGHT = 1,
    PEAK = 2,
    POSCROSSPROD = 4,
    UCUSP = 8,
    DCUSP = 16,
    WINDOW = 32,
    ADDED_NODE = 64;

function hain(points) {

    var n = points.length,
        i, node, c, last;

    // put vertices in a circular doubly linked list

    c = last = makeNode(points[0]);

    for (i = 1; i < n; i++) {
        node = makeNode(points[i]);
        node.prev = last;
        last.next = node;
        last.dy = node.y - last.y;
        last = node;
    }

    c.prev = last;
    last.next = c;
    last.dy = c.y - last.y;


    // TODO make almost horizontal edges exactly horizontal


    // calculate convex/reflex status of vertices, removing almost-inline vertices

    var p = c.prev,
        q = c,
        r = c.next;

    i = n;
    while (i) {
        var crossprod = q.x * (r.y - p.y) - p.x * q.dy - r.x * p.dy;

        if (Math.abs(crossprod) <= FLATNESS) {
            p.next = r;
            r.prev = p;
            p.dy = r.y - p.y;

        } else {
            q.joinType = crossprod > 0 ? POSCROSSPROD : 0;
            p = q;
        }
        q = r;
        r = r.next;
        i--;
    }

    // Find beginning of next down-chain

    // 1. find first actual rising edge
    for (; c.dy <= 0; c = c.next) {}

    // 2. find end of this up-chain (i.e., beginning of down-chain.)
    for (c = c.next; c.dy >= 0; c = c.next) {}

    // Note that a horizontal edge at the top of a chain is considered to belong to the down-chain
    if ((c.prev.dy === 0) && !(c.joinType & POSCROSSPROD)) c = c.prev;

    // c now points to a peak join (left of horizontal edge)

    var qxmax, qxmin,
        numJoins = 0;

    q = c;
    do {
        // Find down-chain, and right-most node of down-chain
        for (p = qxmax = q, q = p.next; q.dy <= 0; q = q.next) {
            if (q.x >= qxmax.x) qxmax = q;
        }

        if (q.prev.dy === 0) {
            if (q.joinType & POSCROSSPROD) q = q.prev;

        } else if (q.x >= qxmax.x) qxmax = q;

        // vertical down-chains are default DOWNTORIGHT.
        // p now points to top of chain (left of horizontal edge.)
        // q now points to bottom of chain (left of horizontal edge),
        // qxmax points to node with rightmost vertex

        if ((qxmax !== p) && (qxmax !== q)) {
            // Create a DCUSP join, and set join links
            p.nextJoin = qxmax;
            qxmax.prevJoin = p;
            qxmax.nextJoin = q;
            q.prevJoin = qxmax;
            p.joinType |= DOWNTORIGHT;
            qxmax.joinType = DCUSP; // note that POSCROSSPROD is set to false
            numJoins++;

        } else {
            p.nextJoin = q;
            q.prevJoin = p;

            if (qxmax === q) {
                p.joinType |= DOWNTORIGHT;
                if (q.joinType & POSCROSSPROD) q.joinType |= DOWNTORIGHT;
                else q.joinType = DCUSP;

            } else if (!(p.joinType & POSCROSSPROD)) p.joinType = DCUSP; // qxmax === p
        }

        numJoins++;
        p.joinType |= PEAK;

        // handle up-chain
        numJoins++;
        p = q; // p is beginning of up-chain

        for (qxmin = p, q = q.next; q.dy >= 0; q = q.next) {
            if (q.x < qxmin.x) qxmin = q;
        }

        if (q.prev.dy === 0) {
            if (!(q.joinType & POSCROSSPROD)) q = q.prev;

        } else if (q.x < qxmin.x) qxmin = q;

        if ((qxmin !== p) && (qxmin !== q)) {
            p.nextJoin = qxmin;
            qxmin.prevJoin = p;
            qxmin.nextJoin = q;
            q.prevJoin = qxmin;
            qxmin.joinType = UCUSP;
            numJoins++;

        } else {
            p.nextJoin = q;
            q.prevJoin = p;
        }
    } while (q !== c);


    // sort joins
    var sortedJoins = [],
        spikes = 0;

    for (q = c, i = 0; i < numJoins; q = q.nextJoin, i++) {
        sortedJoins[i] = q;

        if (q.joinType === 4 || q.joinType === 7) spikes++;
    }
    // console.log((100 * sortedJoins.length / points.length).toFixed(2) + '% joins');
    // console.log((100 * spikes / points.length).toFixed(2) + '% spikes');

    sortedJoins.sort(compareJoins);


    var topOfWindow, botOfWindow, pIn, pOut, qIn, qOut, joinType, joinType2, xtmp, xmax;

    // process joins in x-order
    for (i = 0; i < numJoins; i++) {
        q = sortedJoins[i];
        joinType = q.joinType;

        // if at left of up-chain, mark top of chain as WINDOW
        if (joinType & PEAK) joinType = q.joinType |= WINDOW;
        else q.nextJoin.joinType |= WINDOW;

        if (joinType & POSCROSSPROD) {
            joinType2 = joinType & (PEAK|DOWNTORIGHT);

            if (joinType2 === PEAK) { // & !DOWNTORIGHT
                topOfWindow = q.nextJoin.nextJoin;

            } else if (joinType2 === DOWNTORIGHT) { // & !PEAK
                topOfWindow = q.prevJoin;

            } else { // PEAK same as DOWNTORIGHT.
                // Search for rightmost window on left of, and vertically spanning, current join.

                // --search (backward) for first window with top above q.y
                for (r = q.prevJoin; !(r.joinType & WINDOW) || (r.y <= q.y); r = r.prevJoin ) {}

                // --search (backward) for next window with bottom below q.y
                for ( r= r.prevJoin; (r.joinType & PEAK) || (r.y >= q.y); r = r.prevJoin ) {}

                // We are now at the bottom of first candidate window
                // (vertically spanning q.y, on left of current join)

                // -- find window intersection
                for (p = r.nextJoin.prev; p.y > q.y; p = p.prev) {}

                xtmp = p.x + (p.next.x - p.x) / p.dy * (q.y - p.y);
                if (xtmp < q.x) {
                    xmax = xtmp;
                    topOfWindow = r.nextJoin;
                } else xmax = -Infinity;

                while (true) {
                    // search (backward) for next window with top above q.y
                    for (r = r.prevJoin; (r !== q) && (r !== q.nextJoin) &&
                            (!(r.joinType & WINDOW) || (r.y <= q.y)); r = r.prevJoin) {}

                    if ((r === q) || (r === q.nextJoin)) break;

                    // search (bckwd) for next window with bottom below q.y
                    for (r = r.prevJoin; (r !== q) && (r !== q.nextJoin) &&
                            ((r.joinType & PEAK) || (r.y >= q.y)); r = r.prevJoin) {}

                    // We are now at the bottom of a candidate window
                    // (vertically spanning q.y, and potentially on left)

                    // -- find window intersection
                    for (p = r.nextJoin.prev; p.y > q.y; p = p.prev) {}

                    xtmp = p.x + (p.next.x - p.x) / p.dy * (q.y - p.y);
                    if ((xtmp < q.x) && (xtmp >= xmax)) {
                        xmax = xtmp;
                        topOfWindow = r.nextJoin;
                    }
                }
            }

            botOfWindow = topOfWindow.prevJoin;

            // find vertical position of current join in current window
            for (p = topOfWindow.prev; p.y > (q.y + ALMOST_HORIZONTAL); p = p.prev) {}

            // see if new node is needed (join does not align vertically with a window node
            if (joinType & PEAK) {
                // drawPoint([p.next.x, p.next.y], 'red');
                drawPoly([[q.x, q.y], [p.next.x, p.next.y]], 'red');
                // if (p.next.y < topOfWindow.y) topOfWindow = p.next;
            } else {
                // drawPoint([p.x, p.y], 'blue');
                drawPoly([[q.x, q.y], [p.x, p.y]], 'red');
            }
            // drawPoint([q.x, q.y], joinType & PEAK ? 'red' : 'blue');
            // }

            if (Math.abs(p.y - q.y) > ALMOST_HORIZONTAL) {
                // q lines up with edge
                pIn = makeNode();
                pOut = makeNode();

                pIn.joinType = pOut.joinType = ADDED_NODE;
                pIn.dy = 0;

                pIn.y = pOut.y = q.y;
                pIn.x = pOut.x = p.x + (p.next.x - p.x) / p.dy * (q.y - p.y);
                pOut.next = p.next;
                p.next.prev = pOut;
                pOut.dy = pOut.next.y - pOut.y;

                pOut.nextJoin = topOfWindow;
                pIn.prevJoin = botOfWindow;
                topOfWindow.prevJoin = pOut;
                p.next = pIn;
                pIn.prev = p;
                p.dy = pIn.y - p.y;

            } else {
                // q lines up with vertex
                if (p.prev.dy === 0) pIn = p.prev;
                else {
                    pIn = makeNode();
                    pIn.joinType = ADDED_NODE;
                    pIn.dy = 0;
                    pIn.prev = p.prev;
                    p.prev = p.prev.next = pIn;
                    pIn.dy = 0;
                    pIn.x = p.x;
                    pIn.y = p.y;
                }
                pIn.prevJoin = botOfWindow;
                pOut = null;
            }

            // relink to form two disjoint polygons

            if (joinType & PEAK) { // peak join
                if (q.prev.dy !== 0) {
                    qIn = makeNode();
                    qIn.joinType = ADDED_NODE;
                    qIn.dy = 0;
                    qIn.prev = q.prev;
                    q.prev.next = qIn;
                    qIn.y = q.y;
                    qIn.x = q.x;

                } else qIn = q.prev;

                pIn.next = q;
                q.prev = pIn;
                if (botOfWindow.prevJoin === q) botOfWindow.prevJoin = pIn;

                pIn.nextJoin = q.nextJoin;
                pIn.joinType |= WINDOW;
                q.prevJoin.nextJoin = topOfWindow;
                q.nextJoin.prevJoin = pIn;

                topOfWindow.prevJoin = q.prevJoin;
                botOfWindow.nextJoin = pIn;

                if (pOut) {
                    // peak join lines up horizontally with window edge
                    pOut.prev = qIn;
                    qIn.next = pOut;

                } else { // peak join lines up with window vertex
                    p.prev = qIn;
                    qIn.next = p;
                    q.dy = q.next.y - q.y;
                    qIn.prev.dy = qIn.y - qIn.prev.y;
                }

                // if at right side of down-chain, emit wedge sequence
                if (!(joinType & DOWNTORIGHT)) triangulateMonotone(pIn);

            } else { // valley join
                if (q.dy !== 0) {
                    qOut = makeNode();
                    qOut.joinType = ADDED_NODE;
                    qOut.next = q.next;
                    q.next.prev = qOut;
                    qOut.y = q.y;
                    qOut.x = q.x;

                } else qOut = q.next;

                q.nextJoin.prevJoin = botOfWindow;
                botOfWindow.nextJoin = q.nextJoin;
                qOut.prev = pIn;
                pIn.next = qOut;
                q.dy = 0;

                if (pOut) {
                    // valley join lines up horizontally with window edge
                    if (topOfWindow.nextJoin === q) topOfWindow.nextJoin = pOut;
                    else q.prevJoin.nextJoin = pOut;

                    pOut.prev = q;
                    q.next = pOut;
                    pOut.prevJoin = q.prevJoin;
                    topOfWindow.prevJoin = pOut;
                    pOut.nextJoin = topOfWindow;

                } else {
                    // valley join lines up horizontally with window vertex
                    if (topOfWindow.nextJoin === q) topOfWindow.nextJoin = p;
                    else q.prevJoin.nextJoin = p;

                    q.next = p;
                    p.prev = q;
                    p.prevJoin = q.prevJoin;

                    topOfWindow.prevJoin = p;
                    p.nextJoin = topOfWindow;
                    q.prev.dy = q.y - q.prev.y;
                }
                qOut.dy = qOut.next.y - qOut.y;
                // if at right side of down-chain, emit wedge sequence
                if (joinType & DOWNTORIGHT) triangulateMonotone(topOfWindow);
            }
        } else {
            if (joinType & DCUSP) { // Handle DCUSP case
                if (joinType & PEAK) triangulateMonotone(q);
                else triangulateMonotone(q.prevJoin);

            } else if (joinType & UCUSP) {// Handle UCUSP case
                q.nextJoin.prevJoin = q.prevJoin;
                q.prevJoin.nextJoin = q.nextJoin;

            } else if (joinType & PEAK) {
                if (!(joinType & DOWNTORIGHT)) triangulateMonotone(q);

            } else {
                if (joinType & DOWNTORIGHT) triangulateMonotone(q.nextJoin);
            }
        }
    }
}

function triangulateMonotone(p) {
    // drawMono(p);
    // drawPoint([p.x, p.y], 'blue');
    // drawPoint([p.next.x, p.next.y], 'red');
}

function compareJoins(p, q) {
    return p.x > q.x ?  1 :
           p.x < q.x ? -1 :
           q.y > p.y ?  1 : -1; // ties are done in decreasing y value
}

function makeNode(p) {
    var node = {
        x: null,
        y: null,
        dy: null,
        prev: null,
        next: null,
        prevJoin: null,
        nextJoin: null,
        joinType: null
    };
    if (p) {
        node.x = p[0];
        node.y = p[1];
    }
    return node;
}
