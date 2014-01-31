//Hex 7-segment Decoder

module hex7seg(input logic [3:0] a,
	       output logic [7:0] y);

	always_comb
		case (a)
			4'h0:			y = 8'b0011_1111;
			4'h1:			y = 8'b0000_0110;
			4'h2:			y = 8'b0101_1011;
			4'h3:			y = 8'b0100_1111;
			4'h4:			y = 8'b0110_0110;
			4'h5:			y = 8'b0110_1101;
			4'h6:			y = 8'b0111_1101;
			4'h7:			y = 8'b0000_0111;
			4'h8:			y = 8'b0111_1111;
			4'h9:			y = 8'b0110_1111;
			4'ha:			y = 8'b0111_0111;
			4'hb:			y = 8'b0111_1100;
			4'hc:			y = 8'b0011_1001;
			4'hd:			y = 8'b0101_1110;
			4'he:			y = 8'b0111_1001;
			4'hf:			y = 8'b0111_0001;
			default:	y = 8'b0000_0000;
		endcase
   
endmodule
