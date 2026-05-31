#!/bin/bash
set -e

cd kernel-5.10
 
echo "[+] Downloading and adding hlcan (usb-can-2-module)..."
mkdir -p drivers/net/can/usb-can-2-module
wget -qO usb-can-2-module.zip https://github.com/V0lk3n/usb-can-2-module/archive/refs/heads/main.zip
unzip -q usb-can-2-module.zip
rm -rf drivers/net/can/usb-can-2-module/*
mv usb-can-2-module-main/* drivers/net/can/usb-can-2-module/
rm -rf usb-can-2-module-main usb-can-2-module.zip
echo 'source "drivers/net/can/usb-can-2-module/Kconfig"' >> drivers/net/can/Kconfig
echo 'obj-y += usb-can-2-module/' >> drivers/net/can/Makefile

#echo "[+] Downloading and adding CAN-ISOTP..."
#mkdir -p drivers/net/can/can-isotp
#wget -qO can-isotp.zip https://github.com/V0lk3n/can-isotp/archive/refs/heads/master.zip
#unzip -q can-isotp.zip
#rm -rf drivers/net/can/can-isotp/*
#mv can-isotp-master/* drivers/net/can/can-isotp/
#rm -rf can-isotp-master can-isotp.zip
#echo 'source "drivers/net/can/can-isotp/Kconfig"' >> drivers/net/can/Kconfig
#echo 'obj-y += can-isotp/' >> drivers/net/can/Makefile

echo "[+] Downloading and adding elmcan (ELM327) driver..."
mkdir -p drivers/net/can/elmcan
wget -qO elmcan.zip https://github.com/V0lk3n/elmcan/archive/refs/heads/master.zip
unzip -q elmcan.zip
rm -rf drivers/net/can/elmcan/*
mv elmcan-master/* drivers/net/can/elmcan/
rm -rf elmcan-master elmcan.zip
cp drivers/net/can/elmcan/can327.c drivers/net/can/ || true
echo 'obj-$(CONFIG_CAN_CAN327) += can327.o' >> drivers/net/can/Makefile

# Append Kconfig entry for ELM327
cat <<EOF >> drivers/net/can/Kconfig
config CAN_CAN327
	tristate "Serial / USB serial ELM327 based OBD-II Interfaces (can327)"
	depends on TTY
	select CAN_RX_OFFLOAD
	help
	  CAN driver for several 'low cost' OBD-II interfaces based on the
	  ELM327 OBD-II interpreter chip.

	  This is a best effort driver - the ELM327 interface was never
	  designed to be used as a standalone CAN interface. However, it can
	  still be used for simple request-response protocols (such as OBD II),
	  and to monitor broadcast messages on a bus (such as in a vehicle).

	  Please refer to the documentation for information on how to use it:
	  Documentation/networking/device_drivers/can/can327.rst

	  If this driver is built as a module, it will be called can327.
EOF

echo "[✓] CAN drivers added! Now enable these in 'menuconfig/defconfig' (Networking Support → CAN ...) and build your kernel."