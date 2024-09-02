#include <iostream>
#include <vector>
#include "ConfigureManager.hpp"

int main()
{
    Configuration<128> config;
    config.moduleName = "PLLFreConfigure";
    config.bits = 0x00000000000000000000000000000001;
    LDModule::PLLFreConfigure pll;
    pll.Configure(config);

    config.moduleName = "DUTClkConfigure";
    config.bits = 0x00000000000000000000000000000002;
    LDModule::DUTClkConfigure dut;
    dut.Configure(config);

    config.moduleName = "ModConfigure";
    config.bits = 10;
    LDModule::ModConfigure mod;
    mod.Configure(config);



    return 0;
}
