-- https://github.com/kernkonzept/manifest/wiki/HwPassThrough
local Res = Io.Res;
local Hw = Io.Hw;

Io.Dt.add_children(Io.system_bus(), function()
   rtc = Hw.Device(function()
    compatible = { "arm,pl031" };
    Resource.reg0 = Res.mmio(0xffa60000, 0xffa60fff);
    Resource.irq0 = Res.irq(58, Io.Resource.Irq_type_level_high);   -- 32 for ARM_SPI_BASE
    Resource.irq1 = Res.irq(59, Io.Resource.Irq_type_level_high);   -- 32 for ARM_SPI_BASE
   end)
 end)
