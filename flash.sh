#! /bin/bash
 
if [ -f nuttx.hex ]; then
    echo "Flashing firmware for $RTOS platform $PLATFORM"

    if lsusb -d 15BA:002a; then
    	PROGRAMMER=interface/ftdi/olimex-arm-usb-tiny-h.cfg
    elif lsusb -d 15BA:0003;then
    	PROGRAMMER=interface/ftdi/olimex-arm-usb-ocd.cfg
    elif lsusb -d 15BA:002b;then
    	PROGRAMMER=interface/ftdi/olimex-arm-usb-ocd-h.cfg
    elif lsusb -d 1366:0101;then
    	PROGRAMMER=interface/jlink.cfg
    elif lsusb -d 1366:0105;then
	PROGRAMMER=interface/jlink.cfg
    else
    	echo "Error. Unsuported OpenOCD USB programmer"
    	exit 1
    fi

    openocd -f $PROGRAMMER -f target/stm32f4x.cfg -c init -c "reset halt" -c "flash write_image erase nuttx.hex" -c "flash write_image erase nuttx_user.hex" -c "reset" -c "exit"

else
    echo "Nuttx/nuttx.bin not found: please compile before flashing."
fi

