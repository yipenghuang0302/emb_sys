/*
 * Ball display
 * Yipeng Huang, Richard Townsend
 * Columbia University
 */

module VGA_BALL(
 input logic 	    clk50, reset,
 input logic [9:0]  radius, //2^10 = 1024 > 640
 input logic [9:0]  hpos, //2^10 = 1024 > 640
 input logic [8:0]  vpos, //2^9 = 512 > 480
 output logic [7:0] VGA_R, VGA_G, VGA_B,
 output logic 	    VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n);

/*
 * 640 X 480 VGA timing for a 50 MHz clock: one pixel every other cycle
 * 
 *HCOUNT 1599 0             1279       1599 0
 *            _______________              ________
 * __________|    Video      |____________|  Video
 * 
 * 
 * |SYNC| BP |<-- HACTIVE -->|FP|SYNC| BP |<-- HACTIVE
 *       _______________________      _____________
 * |____|       VGA_HS          |____|
 */

   parameter HACTIVE      = 11'd 1280,
             HFRONT_PORCH = 11'd 32,
             HSYNC        = 11'd 192,
             HBACK_PORCH  = 11'd 96,   
             HTOTAL       = HACTIVE + HFRONT_PORCH + HSYNC + HBACK_PORCH; //1600

   parameter VACTIVE      = 10'd 480,
             VFRONT_PORCH = 10'd 10,
             VSYNC        = 10'd 2,
             VBACK_PORCH  = 10'd 33,
             VTOTAL       = VACTIVE + VFRONT_PORCH + VSYNC + VBACK_PORCH; //525

   logic [10:0]			     hcount; // Horizontal counter
   logic 			     endOfLine;
   
   always_ff @(posedge clk50 or posedge reset)
     if (reset)          hcount <= 0;
     else if (endOfLine) hcount <= 0;
     else  	         hcount <= hcount + 11'd 1;

   assign endOfLine = hcount == HTOTAL - 1;

   // Vertical counter
   logic [9:0] 			     vcount;
   logic 			     endOfField;
   
   always_ff @(posedge clk50 or posedge reset)
     if (reset)          vcount <= 0;
     else if (endOfLine)
       if (endOfField)   vcount <= 0;
       else              vcount <= vcount + 10'd 1;

   assign endOfField = vcount == VTOTAL - 1;

   // Horizontal sync: from 0x520 to 0x57F
   // 101 0010 0000 to 101 0111 1111
   assign VGA_HS = !( (hcount[10:7] == 4'b1010) & (hcount[6] | hcount[5]));
   assign VGA_VS = !( vcount[9:1] == (VACTIVE + VFRONT_PORCH) / 2);

   assign VGA_SYNC_n = 1; // For adding sync to video signals; not used for VGA
   
   // Horizontal active: 0 to 1279     Vertical active: 0 to 479
   // 101 0000 0000  1280	       01 1110 0000  480	       
   // 110 0011 1111  1599	       10 0000 1100  524        
   assign VGA_BLANK_n = !( hcount[10] & (hcount[9] | hcount[8]) ) &
			!( vcount[9] | (vcount[8:5] == 4'b1111) );   

	logic [9:0] vpixel;
	adder_v	vpixel_sub (
		.clock ( clk50 ),
		.dataa ( vcount ),
		.datab ( VFRONT_PORCH + VSYNC + VBACK_PORCH ),
		.result ( vpixel )
	);

	logic [9:0] vdelta;
	adder_v	vdelta_sub (
		.clock ( clk50 ),
		.dataa ( vpixel ),
		.datab ( {1'b0,vpos} ),
		.result ( vdelta )
	);

	logic [19:0] vsquare;
	mult_v	v_mult (
		.clock ( clk50 ),
		.dataa ( vdelta ),
		.result ( vsquare )
	);

	logic [10:0] hpixel;
	adder_h	hpixel_sub (
		.clock ( clk50 ),
		.dataa ( hcount ),
		.datab ( HFRONT_PORCH + HSYNC + HBACK_PORCH ),
		.result ( hpixel )
	);

	logic [10:0] hdelta;
	adder_h	hdelta_sub (
		.clock ( clk50 ),
		.dataa ( {1'b0,hpixel[10:1]} ), // divide hpixel by 2
		.datab ( {1'b0,hpos} ),
		.result ( hdelta )
	);

	logic [21:0] hsquare;
	mult_h	h_mult (
		.clock ( clk50 ),
		.dataa ( hdelta ),
		.result ( hsquare )
	);

	logic [21:0] square_sum;
	s_adder	s_adder_inst (
		.add_sub ( 1'b1 ), // add
		.clock ( clk50 ),
		.dataa ( {1'b0,1'b0,vsquare} ),
		.datab ( hsquare ),
		.result ( square_sum )
	);

	logic [21:0] rsquare;
	mult_h	r_mult (
		.clock ( clk50 ),
		.dataa ( radius ),
		.result ( rsquare )
	);

	logic [21:0] diff;
	s_adder	diff_addr (
		.add_sub ( 1'b0 ), // subtract
		.clock ( clk50 ),
		.dataa ( rsquare ),
		.datab ( square_sum ),
		.result ( diff )
	);

	// TODO: if diff > 0 then print

   assign VGA_CLK = hcount[0]; // 25 MHz clock: pixel latched on rising edge
   
   always_comb begin
      {VGA_R, VGA_G, VGA_B} = {8'h0, 8'h0, 8'h0}; // Black
	  {VGA_R, VGA_G, VGA_B} = {8'hff, 8'h00, 8'h00}; // Red
	  {VGA_R, VGA_G, VGA_B} = {8'h20, 8'h20, 8'h20}; // Dark Gray
   end  
   
endmodule // VGA_LED_Emulator