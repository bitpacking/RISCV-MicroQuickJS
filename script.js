(function() {
    console.log('Hello from MicroQuickJS on RISC-V!')

    var led = new LED('B')
    var isLedOn = true
    while (true) {
        if (isLedOn)
            led.on()
        else
            led.off()

        var time = Date.now()
        while (Date.now() - time < 1000) {}

        isLedOn = !isLedOn
    }
})();
