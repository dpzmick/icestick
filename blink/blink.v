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

parameter START = 4'b0000;
parameter CODE1 = 4'b0001;
parameter CODE2 = 4'b0010;
parameter CODE3 = 4'b0011;
parameter CODE4 = 4'b0011;
parameter CODE5 = 4'b0111;
parameter SUCC  = 4'b1000;

reg last_sw1 = 0;
reg last_sw2 = 0;
reg last_sw3 = 0;
reg last_sw4 = 0;

wire sw1_toggle;
wire sw2_toggle;
wire sw3_toggle;
wire sw4_toggle;

reg [3:0] state = START;

assign sw1_toggle = last_sw1 != SW1 && last_sw1 == 1;
assign sw2_toggle = last_sw2 != SW2 && last_sw2 == 1;
assign sw3_toggle = last_sw3 != SW3 && last_sw3 == 1;
assign sw4_toggle = last_sw4 != SW4 && last_sw4 == 1;

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

    // outputs update based on previous view of the state
    if (state == SUCC) LED5 <= 1;
    else               LED5 <= 0;

    case (state)
        START : if (sw1_toggle) state <= CODE1;
                else            state <= START;

        CODE1 : if (sw1_toggle)                                  state <= CODE2;
                else if (sw2_toggle || sw3_toggle || sw4_toggle) state <= START;
                else                                             state <= CODE1;

        CODE2 : if (sw2_toggle)                                  state <= SUCC;
                else if (sw1_toggle || sw3_toggle || sw4_toggle) state <= START;
                else                                             state <= CODE2;

        SUCC : if (sw1_toggle || sw2_toggle || sw3_toggle || sw4_toggle) state <= START;
               else                                                      state <= SUCC;

        default : state <= START;
    endcase
end

endmodule
