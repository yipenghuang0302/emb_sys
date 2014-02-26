/*
 * Avalon memory-mapped peripheral for the VGA BALL Emulator
 * Yipeng Huang, Richard Townsend
 * Columbia University
 */

module VGA_BALL (
	input logic clk, reset, write, chipselect,
	input logic [15:0] writedata,
	input logic /*[2:0]*/ address,
	output logic [7:0] VGA_R, VGA_G, VGA_B,
	output logic VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n
);

	logic [8:0] vpos; //2^9 = 512 > 480
	logic [9:0] hpos; //2^10 = 1024 > 640
	logic [9:0] radius; //2^10 = 1024 > 640
	VGA_BALL_Emulator ball_emulator(.clk50(clk), .*);

	always_ff @(posedge clk)
		if (reset) begin
			vpos <= 9'd240;
			hpos <= 10'd320;
		end else if (chipselect && write)
		case (address)
			1'b0 : vpos <= writedata[8:0];
			1'b1 : hpos <= writedata[9:0];
		endcase

	assign radius = 10'd10;

endmodule
