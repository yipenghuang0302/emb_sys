module controller(input logic        clk,
		  input logic [3:0]  KEY,
		  input logic [7:0]  dout,
		  output logic [3:0] a,
		  output logic [7:0] din,
		  output logic 	     we);

   // Replace these with your code
   assign a = KEY;
   assign din = {KEY, ~KEY};
   assign we = 1'b1;
   
endmodule
