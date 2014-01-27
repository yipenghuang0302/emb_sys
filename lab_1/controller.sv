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
	// TODO: what if addr is already 0 or 15
	always_ff @(posedge clk)
		if (~KEY[3])
			addr <= addr + 4'd1; //increment address
		else if (~KEY[2])
			addr <= addr - 4'd1; //decrement address

	// sequential logic backing din, data
	// TODO: what if addr is already 0 or 127
	always_ff @(posedge clk)
		if (~KEY[1])
			data <= dout + 8'd1; //increment data
		else if (~KEY[0])
			data <= dout - 8'd1; //decrement data

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