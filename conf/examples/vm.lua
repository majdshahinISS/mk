-- ln -s  ../../../../l4/conf/examples/vm.lua    ../$L4Re_BUILD/bin/arm64_armv8a/l4f/vm.lua
return {
  name = "ubuntu_vm",
  image = {
	kernel = "/path/to/linux/arch/arm64/boot/Image",
	ramdisk = "/path/to/ubuntu-rootfs.cpio.gz",
	cmdline = "console=hvc0 root=/dev/ram0 rw init=/sbin/init"
  },
  resources = {
	cpus = 4,          -- Use all 4 Cortex-A53 cores
	memory = 1024,      -- 1GB RAM
  },
  devices = {
	{ name = "uart", type = "pl011", base = 0xff000000, irq = 33 },  -- ZCU104 UART
	{ name = "gic", type = "arm-gic-v3" },  -- GICv3 interrupt controller
  },
}