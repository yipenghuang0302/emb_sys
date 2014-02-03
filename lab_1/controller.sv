module controller(
	input logic clk,
	input logic [3:0] KEY,
	input logic [7:0] dout,
	output logic [3:0] a,
	output logic [7:0] din,
	output logic we
);


	logic key_3_ff;
	logic key_2_ff;
	logic key_1_ff;
	logic key_0_ff;

	logic addr_inc_trig;
	logic addr_dec_trig;
	logic [3:0] addr;
	
	logic [7:0] data_inc;
	logic [7:0] data_dec;
	logic [7:0] next_data;
	
	logic write_en;
	
	//sequential logic defining key flip-flops
	always_ff @(posedge clk) begin
		key_3_ff <= ~KEY[3];
		key_2_ff <= ~KEY[2];
		key_1_ff <= ~KEY[1];
		key_0_ff <= ~KEY[0];
	end
	
	//sequential logic defining addr_inc_trig and addr_dec_trig
	always_ff @(posedge clk) begin
		addr_inc_trig <= key_3_ff && KEY[3];
		addr_dec_trig <= key_2_ff && KEY[2];
	end
	
	// sequential logic backing a, addr
	always_ff @(posedge clk)
		if (addr_inc_trig) // increment
			if (addr == 4'd15) addr <= 4'd0; // go to zero
			else addr <= addr + 4'd1; // increment address
		else if (addr_dec_trig) // decrement
			if (addr == 4'd0) addr <= 4'd15; // go to top
			else addr <= addr - 4'd1; // decrement address
		
	//combinational logic defining data_dec and data_inc
	always_comb begin
		data_dec = (dout == 8'd0) ? 8'd255 : dout - 8'd1;
		data_inc = (dout == 8'd255) ? 8'd0 : dout + 8'd1;
	end
	
	//sequential logic defining next_data
	always_ff @(posedge clk)
		next_data <= key_1_ff ? data_inc : data_dec;

	// sequential logic backing we
	always_ff @(posedge clk)
		if (key_1_ff && KEY[1] || key_0_ff && KEY[0])
			write_en <= 1'b1;
		else
			write_en <= 1'b0;

	// Replace these with your code
	assign a = addr;
	assign din = next_data;
	assign we = write_en;

endmodule