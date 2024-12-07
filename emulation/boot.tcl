proc wait_until_boot {} {
	global speed
	set speed 100
}

set save_settings_on_exit off
set speed 9999
set fullspeedwhenloading on
debug set_watchpoint write_io 0x18
after time 20 wait_until_boot
