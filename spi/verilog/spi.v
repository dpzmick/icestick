module spi_slave(
    // fpga signals
    input clk,

    // spi signals
    input  SCK,
    input  SSEL,  // FIXME figure out if high or low is active
    input  MOSI,  // Master Out Slave In
    output MISO,  // Master In Slave Out

    // for the producer/consumer
    input  in_ready,
    input  [31:0] in, // FIXME make configurable
    output in_done,  // toggled when the user can write more data

    output out_ready,
    output [31:0] out  // FIXME make configurable
);

reg [1:0] _sck;
reg [1:0] _mosi;
reg [4:0] cnt; // five bits for count

always @(posedge clk) begin
    if (SSEL) begin // FIXME high or low?
        _sck  <= {_sck[0], SCK};
        _mosi <= {_mosi[0], MOSI}; // not sure if this needs to be sampled
    end else begin
        _sck      <= 2'b00;
        _mosi     <= 2'b00;
        cnt       <= 0; // reset to zero at just the right times?
        out_ready <= 0; // ???????
    end
end

always @(posedge clk) begin
    if (SSEL) begin // FIXME high or low?
        // rising edge
        if (_sck[1] && !_sck[0]) begin
            out[31:0] <= { out[30:0], _mosi[1] };
            if (cnt == 5'd31) begin
                out_ready <= 1;
                cnt       <= 0;
            end else begin
                out_ready <= 0;         // user only has a short moment to actually read the bit
                cnt       <= cnt + 1;
            end
            // what happens if a bit is started at the "wrong" time?
        end
    end
end

endmodule
