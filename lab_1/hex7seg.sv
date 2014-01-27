//Hex 7-segment Decoder

module hex7seg(input logic [3:0] a,
	       output logic [7:0] y);

   assign y = {a,a}; // Replace this with your code
   
endmodule
