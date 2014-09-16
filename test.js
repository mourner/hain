
var Benchmark = require('benchmark');

var hain = require('./hain');

var points = [[661,112],[661,96],[666,96],[666,87],[743,87],[771,87],[771,114],[750,114],[750,113],[742,113],[742,106],[710,106],[710,113],[666,113],[666,112]].reverse();

// hain(points);


new Benchmark.Suite()
    .add('hain', function () {
        hain(points);
    })
    .on('error', function(event) {
        console.log(event.target.error);
    })
    .on('cycle', function(event) {
        console.log(String(event.target));
    })
    // .on('complete', function(event) {
    //  var benchmarks = this.sort(function (a, b) { return a.hz - b.hz; }).reverse();
    //  console.log(benchmarks[0].name + ' is ' + Math.round(100 * (benchmarks[0].hz / benchmarks[1].hz - 1)) +
    //      '% faster than ' + benchmarks[1].name + ' (second fastest)\n');
    // })
    .run();
