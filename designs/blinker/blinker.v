module blinker(
	input clk,
	output led1,
	output led2,
	input btn1,
	input btn2
);

/* debounce */
wire ce;
reg [20:0] div;
always @(posedge clk)
	div <= div + 21'd1;
assign ce = div[20];

reg btn1_r;
reg btn2_r;
reg [4:0] increment;
always @(posedge clk) begin
	if(ce) begin
		btn1_r <= btn1;
		btn2_r <= btn2;
		if(btn1 & ~btn1_r)
			increment <= increment + 5'd1;
		else if(btn2 & ~btn2_r)
			increment <= increment - 5'd1;
	end
end

reg [26:0] count;
always @(posedge clk)
	count <= count + increment;

assign led1 = count[26];
assign led2 = count[25];

endmodule

