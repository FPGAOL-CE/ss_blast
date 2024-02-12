/**
 * File              : ss_pins.v
 * License           : GPL-3.0-or-later
 * Author            : Peter Gu <github.com/regymm>
 * Date              : 2024.02.10
 * Last Modified Date: 2024.02.12
 */

// single device selectMAP configuration
module ss_pins
(
	input [3:0]gpio_in,
	output [1:0]gpio_out,
	input [31:0]gpio_d,

	output cfg_cclk,
	output cfg_prog_b,
	input cfg_init_b,
	input cfg_done,
	output cfg_cs_b,
	output cfg_rdwr_b,
	output [31:0]cfg_d
);
	assign {cfg_cclk, cfg_prog_b, cfg_cs_b, cfg_rdwr_b} = gpio_in;
	assign gpio_out = {cfg_init_b, cfg_done};
	assign cfg_d = gpio_d;
endmodule
