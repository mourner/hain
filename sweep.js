
if (typeof module !== 'undefined') module.exports = sweep;

var VERTEX_START = 0,
    VERTEX_SPLIT = 1,
    VERTEX_END = 2,
    VERTEX_MERGE = 3,
    VERTEX_REGULAR = 4;

function sweep(points) {

    var n = points.length,
        queue = [],
        i, first, last, v, prev, next;

    // put vertices in a circular doubly linked list

    first = last = makeNode(points[0], 0);

    queue.push(first);

    for (i = 1; i < n; i++) {
        v = makeNode(points[i], i);
        v.prev = last;
        last.next = v;
        last.dy = v.y - last.y;
        last = v;
        queue.push(v);
    }

    first.prev = last;
    last.next = first;
    last.dy = first.y - last.y;

    // init edges list

    var edges = [];

    for (i = 0; i < n; i++) {
        v = queue[i];
        edges.push(new Edge(v, v.next, i));
    }

    // we'll iterate through vertices from top to bottom (sweep line)
    queue.sort(compareVertices);

    // determine vertex type
    for (i = 0; i < n; i++) {
        v = queue[i];
        prev = v.prev;
        next = v.next;

        v.type =
            below(prev, v) && below(next, v) ? (isConvex(next, prev, v) ? VERTEX_START : VERTEX_SPLIT) :
            below(v, prev) && below(v, next) ? (isConvex(next, prev, v) ? VERTEX_END : VERTEX_MERGE) : VERTEX_REGULAR;
    }

    var helpers = new Array(n),
        currentEdges = [],
        k, j, m;

    for (k = 0; k < n; k++) {
        v = queue[k];
        i = v.index;

        if (v.type === VERTEX_START) {
            insertEdge(currentEdges, edges[i]);
            helpers[i] = v;

        } else if (v.type === VERTEX_END) {
            m = v.prev.index;
            if (helpers[m].type === VERTEX_MERGE) addDiagonal(v, helpers[m]);
            removeEdge(currentEdges, edges[m]);

        } else if (v.type === VERTEX_SPLIT) {

            j = lowerEdge(currentEdges, v).index;
            addDiagonal(v, helpers[j]);
            helpers[j] = v;

            insertEdge(currentEdges, edges[i]);
            helpers[i] = v;

        } else if (v.type === VERTEX_MERGE) {

            m = v.prev.index;
            if (helpers[m].type === VERTEX_MERGE) addDiagonal(v, helpers[m]);
            removeEdge(currentEdges, edges[m]);

            j = lowerEdge(currentEdges, v).index;
            if (helpers[j].type === VERTEX_MERGE) addDiagonal(v, helpers[j]);
            helpers[j] = v;

        } else {

            if (below(v, v.prev)) {
                m = v.prev.index;
                if (helpers[m].type === VERTEX_MERGE) addDiagonal(v, helpers[m]);
                removeEdge(currentEdges, edges[m]);

                insertEdge(currentEdges, edges[i]);
                helpers[i] = v;

            } else {
                j = lowerEdge(currentEdges, v).index;
                if (helpers[j].type === VERTEX_MERGE) addDiagonal(v, helpers[j]);
                helpers[j] = v;
            }
        }
    }
}

function addDiagonal(v1, v2) {
    // debugger;
    // console.log('add!');
    drawPoly([[v1.x, v1.y], [v2.x, v2.y]], 'red');
}

function compareVertices(p1, p2) {
    return (p1.y - p2.y) || (p1.x - p2.x);
}

function below(p1, p2) {
    return ((p1.y - p2.y) || (p1.x - p2.x)) > 0;
}

function isConvex(p1, p2, p3) {
    return (p3.y - p1.y) * (p2.x - p1.x) - (p3.x - p1.x) * (p2.y - p1.y) > 0;
}

function makeNode(p, index) {
    var node = {
        x: p[0],
        y: p[1],
        index: index,
        prev: null,
        next: null,
        type: null
    };
    return node;
}


function insertEdge(edges, edge) {

    var i = 0,
        j = edges.length - 1;

    while (i <= j) {
        var mid = Math.floor((i + j) / 2);

        if (compareEdges(edge, edges[mid])) j = mid - 1;
        else i = mid + 1;
    }

    edges.splice(i, 0, edge);
}

function lowerEdge(edges, p) {

    var i = 0,
        j = edges.length - 1,
        edge = new Edge(p, p);

    while (i <= j) {
        var mid = Math.floor((i + j) / 2);

        if (compareEdges(edge, edges[mid])) j = mid - 1;
        else i = mid + 1;
    }

    return edges[i - 1];
}

function removeEdge(edges, edge) {

    var i = 0,
        j = edges.length - 1;

    while (i <= j) {
        var mid = Math.floor((i + j) / 2);

        if (edge === edges[mid]) {
            edges.splice(mid, 1);
            return;
        }

        if (compareEdges(edge, edges[mid])) j = mid - 1;
        else i = mid + 1;
    }
}

function compareEdges(e1, e2) {
    return e2.p1.y === e2.p2.y ? (e1.p1.y === e1.p2.y ? e1.p1.y < e2.p1.y : isConvex(e1.p1, e1.p2, e2.p1)) :
           e1.p1.y <= e1.p2.y ? !isConvex(e2.p1, e2.p2, e1.p1) : isConvex(e1.p1, e1.p2, e2.p1);
}

function Edge(p1, p2, index) {
    this.p1 = p1;
    this.p2 = p2;
    this.index = index;
}


// function findEdge(items, edge) {

//     var i = 0,
//         j = items.length - 1;

//     while (i <= j) {
//         var mid = Math.floor((i + j) / 2);
//         var c = compareEdges(edge, items[mid]);

//         if (c === 0) return mid;

//         if (c < 0) j = mid - 1;
//         else i = mid + 1;
//     }

//     return null;
// }

// function removeEdge(items, edge) {
//     var index = findEdge(items, edge);
//     if (index !== null) items.splice(index, 1);
// }

