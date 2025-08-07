-- vim:ft=lua
-- io configuration for zynqmp Xilinx UltraSCALE+ MPSoC
-- see https://www.xilinx.com/html_docs/registers/ug1087/ug1087-zynq-ultrascale-registers.html

-- Configure Ethernet Device
Io.Dt.add_children(Io.system_bus(), function()

  -- create a new hardware device called "GEM3"
  GEM3 = Io.Hw.Device(function()
    -- set the compatibility IDs for this device
    -- a client tries to match against these IDs and configures
    -- itself accordingly
    -- the list should be sorted from specific to less specific IDs
    compatible = {"cdns,zynqmp-gem", "cdns,gem"};

    -- set the 'hid' property of the device, the hid can also be used
    -- as a compatible ID when matching clients
    Property.hid  = "dev-eth,ethernet";

    -- note: names for resources are truncated to 4 letters and a client
    -- can determine the name from the ID field of a l4vbus_resource_t
    -- add two resources 'irq0' and 'reg0' to the device
    Resource.irq0 = Io.Res.irq(95); -- XPS_GEM3_INT_ID=(63U + 32U)
    Resource.reg0 = Io.Res.mmio(0xff0e0000, 0xff0effff);
  end);

  CRL_ARB = Io.Hw.Device(function()
	compatible = {"zynqmp-CRL_ARB", "CRL_ARB"};
        Property.hid  = "dev-CRL_ARB,CRL_ARB";
        Resource.reg0 = Io.Res.mmio(0xff5e0000, 0xff85ffff);
  end);

  GPIO = Io.Hw.Device(function()
	compatible = {"zynqmp-gpio", "gpio"};
        Property.hid  = "dev-gpio,gpio";
        Resource.reg0 = Io.Res.mmio(0xff0a0000, 0xff0a0fff);
  end);

  CSU = Io.Hw.Device(function()
	compatible = {"zynqmp-csu", "csu"};
        Property.hid  = "dev-csu,csu";
        Resource.reg0 = Io.Res.mmio(0xffca0000, 0xffca2000);
  end);

  rtc = Io.Hw.Device(function()
    compatible = { "arm,hwrtc" };
    Resource.reg0 = Io.Res.mmio(0xffa60000, 0xffa60fff);
    Resource.irq0 = Io.Res.irq(58, Io.Resource.Irq_type_level_high);   -- 32 for ARM_SPI_BASE
    Resource.irq1 = Io.Res.irq(59, Io.Resource.Irq_type_level_high);   -- 32 for ARM_SPI_BASE
   end);

  sdhci0 = Io.Hw.Device(function()
    compatible = {"xlnx,zynqmp-8.9a", "arasan,sdhci-8.9a","sdhci", "generic-sdhci"};
    
    -- Register space (0xff160000-0xff16ffff)
    Resource.reg0 = Io.Res.mmio(0xff160000, 0xff16ffff);
    
    -- Interrupt configuration (GIC SPI 48, level-high)
    Resource.irq0 = Io.Res.irq(48, 4);
    
        -- REQUIRED for emmc-drv detection
    Property.hid = "dev-sdhci,sdhci0";
    Property.flags = Io.Hw_device_DF_dma_supported;  -- device can perform DMA
    -- Property.Class = "sdhci";
    -- Class = "sdhci";
    -- status = "okay";  -- Explicitly enable device
    -- Property.enable = 1;       -- Alternative method
    -- enable = 1;       -- Alternative method
    -- enabled = 1;       -- Alternative method
    -- Property.device_type = "mmc";
    -- device_type = "mmc";
    -- Property.max_frequency = 200000000;  
    -- max_frequency = 200000000;  
  end);

  sdhci1 = Io.Hw.Device(function()
    compatible = {"xlnx,zynqmp-8.9a", "arasan,sdhci-8.9a","sdhci", "generic-sdhci"};
    Resource.reg0 = Io.Res.mmio(0xff170000, 0xff17ffff);
    
    -- Interrupt configuration (GIC SPI 49, level-high)
    Resource.irq0 = Io.Res.irq(49, 4);
    
        -- REQUIRED for emmc-drv detection
    Property.hid = "dev-sdhci,sdhci1";
    Property.flags = Io.Hw_device_DF_dma_supported;  -- device can perform DMA  
  end);

  usb0 = Io.Hw.Device(function()
    compatible = {"xlnx,zynqmp-dwc3", "generic-xhci"};
    Resource.reg0 = Io.Res.mmio(0xff9d0000, 0xff9d00ff);  -- USB@fe200000
    Resource.irq0 = Io.Res.irq(65, 4);                   -- GIC SPI 65
    Property.hid = "dev-usb,xhci";                        -- Required for USB driver
  end);
end);


Io.add_vbusses
{
  -- Create a virtual bus for a client and give access to GEM3
  vbus1 = Io.Vi.System_bus(function ()
    ethernet3 = wrap(Io.system_bus().GEM3);
    CRL_ARB   = wrap(Io.system_bus().CRL_ARB);
    rtc       = wrap(Io.system_bus().rtc);
  end);

  vbus2 = Io.Vi.System_bus(function ()
    gpio = wrap(Io.system_bus().GPIO);
  end);

  vbus3 = Io.Vi.System_bus(function ()
    csu = wrap(Io.system_bus().CSU);
  end);
  
  vbus_sd = Io.Vi.System_bus(function()
    sdhci0 = wrap(Io.system_bus():match("dev-sdhci,sdhci0"));
    sdhci1 = wrap(Io.system_bus():match("dev-sdhci,sdhci1"));
    usb0 = wrap(Io.system_bus():match("dev-usb,xhci"));
  end);
}
