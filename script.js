(function() {
    console.log('Hello from MicroQuickJS on RISC-V!')
    var s, t, d, r = new LED('R')
    for (t = 1, d = Date.now;; t=!t) {
        t ? r.on() : r.off()
        s = d()
        while (d() - s < 999){}
    }
})();
