local L4 = require("L4");

local l = L4
local loader = L4.default_loader;
local uvmm = require("uvmm");

-- dynamically generate FDT using uvmm_dtg
local dt = L4.new_channel()
loader:start(
  { caps = { dt = dt:svr() } },
  "rom/uvmm_dtg dt"
);
print("Line 13");
-- define memory
local memcap = L4.new_channel()
print("Line 16");

-- start VM
uvmm.start_vm {
  name = "linux_vm",
  fdt = "dt",            -- use dynamically generated DTB
  kernel = "rom/linux",
  ramfs = "rom/ramdisk.cpio.gz",
  memory = { memcap, size = 1024 * 1024 * 1024 }, -- 1 GiB RAM
  vcpus = 4,
  gic = "gic",           -- optional: link to existing gic cap
  uart = "uart",         -- optional: connect guest UART
  bootargs = "console=ttyAMA0 earlycon=pl011,0x9000000 root=/dev/ram0 rw"
};
print("Line 30");


-- ln -s  ../../../../l4/conf/examples/vm-ubuntu-zcu104_dtg.lua    ../$L4Re_BUILD/bin/arm64_armv8a/l4f/vm-ubuntu-zcu104_dtg.lua
