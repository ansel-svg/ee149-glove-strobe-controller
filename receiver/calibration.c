/*
	This will be used to handle the BOOT button long press, average the captured flex and
	IMU readings, check each finger's open and closed readings, and load/save the result
	to flash.

	Input: "glove_packet": packet [defined in glove_packet.h], "boot_state": bool

	Output: "calibration": per-finger open/closed readings,
	"enabled_mask", flat-hand baseline, to state.c and storage

	Interfaces with: state.c, calibration.h, receiver_config.h, light_map.c
*/
