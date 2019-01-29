module blink(
    input      clk,
    input      SW1,
    input      SW2,
    input      SW3,
    input      SW4,
    output reg LED1,
    output reg LED2,
    output reg LED3,
    output reg LED4,
    output reg LED5
);

// secret code is 11234

parameter START = 3'b000;
parameter CODE1 = 3'b001;
parameter CODE2 = 3'b010;
parameter CODE3 = 3'b011;
parameter CODE4 = 3'b110;
parameter SUCC  = 3'b111;

reg last_sw1 = 0;
reg last_sw2 = 0;
reg last_sw3 = 0;
reg last_sw4 = 0;

reg sw1_falling = 0;
reg sw2_falling = 0;
reg sw3_falling = 0;
reg sw4_falling = 0;

reg [2:0] state = START;

always @(posedge clk) begin
    LED1 <= SW1;
    LED2 <= SW2;
    LED3 <= SW3;
    LED4 <= SW4;

    // sample all of the buttons
    last_sw1 <= SW1;
    last_sw2 <= SW2;
    last_sw3 <= SW3;
    last_sw4 <= SW4;

    sw1_falling = last_sw1 == 1 && !SW1;
    sw2_falling = last_sw2 == 1 && !SW2;
    sw3_falling = last_sw3 == 1 && !SW3;
    sw4_falling = last_sw4 == 1 && !SW4;

    // outputs update based on previous view of the state
    if (state == SUCC) LED5 <= 1;
    else               LED5 <= 0;

    case (state)
        START : if (sw1_falling) state <= CODE1;
                else             state <= START;

        CODE1 : if (sw1_falling)                                    state <= CODE2;
                else if (sw2_falling || sw3_falling || sw4_falling) state <= START;
                else                                                state <= CODE1;

        CODE2 : if (sw2_falling)                                    state <= CODE3;
                else if (sw1_falling || sw3_falling || sw4_falling) state <= START;
                else                                                state <= CODE2;

        CODE3 : if (sw3_falling)                                    state <= CODE4;
                else if (sw1_falling || sw2_falling || sw4_falling) state <= START;
                else                                                state <= CODE3;

        CODE4 : if (sw4_falling)                                    state <= SUCC;
                else if (sw1_falling || sw2_falling || sw3_falling) state <= START;
                else                                                state <= CODE4;

        SUCC  : if (sw2_falling || sw3_falling || sw4_falling) state <= START;
                else if (sw1_falling)                          state <= CODE1;
                else                                           state <= SUCC;

        default : state <= START;
    endcase
end

endmodule
