// NOVA-8 high-level demo: variables, control flow, builtin calls
var x = 10;
var y = 20;
x = x + y * 2;

if (x > 40) {
    pal(1);
} else {
    pal(0);
}

var i = 0;
while (i < 4) {
    spr(0, i, i);
    poke(i, x + i);
    i = i + 1;
}
halt;
