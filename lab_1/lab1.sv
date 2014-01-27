// CSEE 4840 Lab1: Display and modify the contents of memory
//
// Spring 2014
//
// By: Yipeng Huang
// Uni: yh2315

module lab1(input logic       clk,
            input logic [3:0] KEY,
            output [7:0]      hex0, hex2, hex3);

   logic [3:0] 		      a;         // Address
   logic [7:0] 		      din, dout; // RAM data in and out
   logic 		      we;        // RAM write enable

   hex7seg h0( .a(a),         .y(hex0) ),
           h1( .a(dout[7:4]), .y(hex2) ),
           h2( .a(dout[3:0]), .y(hex3) );

   controller c( .* ); // Connect everything with matching names
   memory m( .* );
  
endmodule

// 16 X 8 synchronous RAM with old data read-during-write behavior
module memory(input logic        clk,
	      input logic [3:0]  a,
	      input logic [7:0]  din,
	      input logic 	 we,
	      output logic [7:0] dout);
   
   logic [7:0] 			 mem [15:0];

   always_ff @(posedge clk) begin
      if (we) mem[a] <= din;
      dout <= mem[a];
   end
        
endmodule

