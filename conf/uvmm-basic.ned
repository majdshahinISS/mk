local L4 = require "L4";

L4.default_loader:startv({
  log = L4.Env.log,
  caps = {
    ram  = L4.Env.user_factory:create(
      L4.Proto.Dataspace,
      128 * 1024 * 1024,                   -- size in MB
      L4.Mem_alloc_flags.Continuous |
        L4.Mem_alloc_flags.Pinned |
        L4.Mem_alloc_flags.Super_pages,
      21                                   -- alignment
    ):m("rws");
  }
}, "rom/uvmm", "--dtb", "rom/dtb/virt-arm_virt-64.dtb",
  "--ramdisk", "rom/ramdisk-armv8-64.rd", "--kernel", "rom/Image2.gz",
  "--cmdline", "console=hvc0 earlyprintk=1 rdinit=/bin/sh");

  -- -- $ ln -s  ../../l4/conf/uvmm-basic.ned   ../build-aarch64/assets/uvmm-basic.ned
