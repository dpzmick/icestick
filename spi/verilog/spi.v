module spi_slave(
    // fpga signals
    input clk,

    // spi signals
    input  SCK,
    input  SSEL,
    input  MOSI,  // Master Out Slave In
    output MISO  // Master In Slave Out
);

// reg last_sck = 0; // default to high, since high is 'not sending'
// reg tmp2 = 0;

// // handle crossing clock domains
// always @(*) begin
//     if (SSEL) begin
//         // is this a falling edge?
//         if (!last_sck && SCK) begin
//             tmp2 = ~tmp2;
//         end
//     end else begin
//         tmp2 = 1;
//     end

//     if (MOSI) MISO = 1;
//     else      MISO = 0;
// end

// always @(posedge clk) begin // triggers when clk goes up
//     tmp2 <= ~tmp2;
// end

endmodule
