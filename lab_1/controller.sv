module controller(
	input logic clk,
	input logic [3:0] KEY,
	input logic [7:0] dout,
	output logic [3:0] a,
	output logic [7:0] din,
	output logic we
);

	logic [3:0] addr;
	logic [7:0] data;
	logic write_en;

	// sequential logic backing a, addr
	always_ff @(posedge clk)
		if (~KEY[3]) // increment
			if (addr == 4'd15) addr <= 4'd0; // go to zero
			else addr <= addr + 4'd1; // increment address
		else if (~KEY[2]) // decrement
			if (addr == 4'd0) addr <= 4'd15; // go to top
			else addr <= addr - 4'd1; // decrement address

	// sequential logic backing din, data
	always_ff @(posedge clk)
		if (~KEY[1]) // increment
			if (data == 8'd127) data <= 8'd0; // go to zero
			else data <= dout + 8'd1; // increment data
		else if (~KEY[0]) // decrement
			if (data == 8'd0) data <= 8'd127; // go to top
			else data <= dout - 8'd1; //decrement data

	// sequential logic backing we
	// TODO: double check timing
	always_ff @(posedge clk)
		if (~KEY[1] || ~KEY[0])
			write_en <= 1'b1;
		else
			write_en <= 1'b0;

	// Replace these with your code
	assign a = addr;
	assign din = data;
	assign we = write_en;

endmodule