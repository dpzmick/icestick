module test;

    reg clk;
    reg SW1;
    reg SW2;
    reg SW3;
    reg SW4;

    wire LED1;
    wire LED2;
    wire LED3;
    wire LED4;
    wire LED5;

    blink b(
        .clk(clk),
        .SW1(SW1),
        .SW2(SW2),
        .SW3(SW3),
        .SW4(SW4),
        .LED1(LED1),
        .LED2(LED2),
        .LED3(LED3),
        .LED4(LED4)
    );

    always #5 clk = !clk;

    initial begin
        clk = 0;
        $dumpfile("dump.vcd");
        $dumpvars;

        #10 SW1 = 1;
        #17 SW1 = 0;

        #100 $finish;
    end

endmodule // test
