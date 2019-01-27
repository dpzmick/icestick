module test;

    reg clk = 0;
    reg SW1 = 0;
    reg SW2 = 0;
    reg SW3 = 0;
    reg SW4 = 0;

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
        #20 SW1 = 0;

        #21 SW1 = 1;
        #31 SW1 = 0;

        #32 SW2 = 1;
        #42 SW2 = 0;

        #43 SW3 = 1;
        #53 SW3 = 0;

        #54 SW4 = 1;
        #65 SW4 = 0;

        #100 $finish;
    end

endmodule // test
