module gates(
	input btn1,
	input btn2,
	output led1,
	output led2
);

assign led1 = btn1 | btn2;
assign led2 = btn1 & btn2;

endmodule
