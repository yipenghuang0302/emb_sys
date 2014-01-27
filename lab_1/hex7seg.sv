//Hex 7-segment Decoder

module hex7seg(input logic [3:0] a,
	       output logic [6:0] y);

	always_comb
		case (a)
			4'h0:			y = 7'b111_1110;
			4'h1:			y = 7'b011_0000;
			4'h2:			y = 7'b110_1101;
			4'h3:			y = 7'b111_1001;
			4'h4:			y = 7'b011_0011;
			4'h5:			y = 7'b101_1011;
			4'h6:			y = 7'b101_1111;
			4'h7:			y = 7'b111_0000;
			4'h8:			y = 7'b111_1111;
			4'h9:			y = 7'b111_1011;
			4'ha:			y = 7'b111_0111;
			4'hb:			y = 7'b001_1111;
			4'hc:			y = 7'b100_1110;
			4'hd:			y = 7'b011_1101;
			4'he:			y = 7'b100_1111;
			4'hf:			y = 7'b100_0111;
			default:	y = 7'b000_0000;
		endcase
   
endmodule
