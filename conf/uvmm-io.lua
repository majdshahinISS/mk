package.path = "rom/?.lua";

local L4 = require "L4";
local vmm = require "conf.vmm";

vmm.loader.log_fab = L4.default_loader:new_channel();

local function vm(id, vbus)
  vmm.start_vm({
    id = id,
    mem = 128,
    rd = "rom/ramdisk-armv8-64.cpio.gz",
    fdt = "rom/zynqmp-zcu104-revA.dtb",
    kernel = "rom/Image.gz",
    vbus = vbus,
    bootargs = "console=hvc0 earlyprintk=1 rdinit=/init",
  });
end

L4.default_loader:start(
   {
     log = L4.Env.log,
     caps = { cons = vmm.loader.log_fab:svr() }
   }, "rom/cons -a");

local io_busses = {
   vm_hw = 1,
}

vmm.start_io(io_busses, "rom/rtc.io -vvvvvv");
vm(1, io_busses.vm_hw);