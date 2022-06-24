## V2.0.0

### Version

* SDK version: telink_b91m_driver_sdk V2.0.0.
* This version of the SDK supports B91 A0, A1 and A2 chips.

### BREAKING CHANGES

* **project**
    * Modified the folder structure, using virtual folders in the project and linking to local folders through relative paths.
* **pke**
    * B91 adds the cases where the curves are 160, 192 and 224.
    * pke_get_irq_status, pke_clr_irq_status, pke_set_irq_mask, pke_clr_irq_mask, pke_set_microcode, pke_set_exe_cfg, pke_opr_start, pke_check_rt_code, pke_set_operand_width_pre, pke_set_operand_width_pre_mont, pke_calc_pre_width and pke_mont_calc_pre
* **swire**
    * A new parameter is added to the swire_sync interface and swire_set_swm_en interface to set swm gpio.
* **uart**
    * The uart_set_dma_rx_timeout interface is changed to uart_set_rx_timeout.
* **dma**
    * dma_mask is turned on by default, and it is cleared in sys_init, and there is no need to clear it again in the upper application.
* **i2c**
    * Before the i2c_master_write_read function, there is no stop between sending data and receiving data by default. After modification, you can choose whether there is a stop by using the interface i2c_master_send_stop.
* **pwm**
    * Before pwm_start can only open one at the same time by default, redefine the enumeration type, after modification, you can open multiple at the same time.
* **flash**
    * In order to be compatible with different Flash models, the Flash driver structure has been adjusted, and some Flash interface calling methods have been modified.
    * In order to be compatible with different types of Flash, flash_read_status and flash_write_status are not provided for external use. If you need to use the corresponding interface, you need to select the interface under the flash directory according to the mid (for example: flash_write_status_midxxxxxx).
    * The first parameter flash_mid of the interface int flash_read_mid_uid_with_check( unsigned int *flash_mid ,unsigned char *flash_uid) requires 4 bytes of space, and the second parameter requires 16 bytes of space.
    * Interface flash_lock, flash_unlock, flash_read_otp, flash_write_otp, flash_erase_otp, flash_lock_otp are replaced by flash_lock_midxxxxxx, flash_unlock_midxxxxxx, flash_read_otp_midxxxxxx, flash_write_otp_midxxxxxx, flash_miderase_otxxxxxx, flash_miderase_otxxx, xxx
    * Delete unused function interfaces: flash_erase_page, flash_erase_32kblock, flash_erase_64kblock, flash_erase_chip, flash_deep_powerdown, flash_release_deep_powerdown.
* **s7816**
    * Modify the meaning of the s7816_set_clk parameter, and change the parameter passed in to represent the frequency division number.
* **rf**
    * The previous function interface rf_set_tx_settle_time, rf_set_rx_settle_time set the time value to the parameter + 1, now the set value is consistent with the parameter.
* **pm**
    * pm_set_suspend_power() add a argument to select power down or power on usb/npe/baseband module before suspend.if power down,th sleep current will decrease,but need to re-init the module after wake up.three module all power down default.

### Bug Fixes

* **freertos**
    * Fixed the problem that the system would crash after being interrupted by a task while reading and writing the flash. Because const variables also access flash.
* **swire**
    * Modify the driving time of the swire_sync function interface.
    * Modify the swire_set_slave_id interface to prevent writing 0 first and then assigning the value when configuring the register. When the configuration is 0, abnormal signals may be generated and some risks may be generated.
* **flash**
    * Solve the problem that the unlock interface of flash will reset the four-wire configuration.
* **aes**
    * Modify the pointer of the aes data address to the offset address based on I-RAM minus the offset address of embase to I-RAM.
* **clock**
    * Modify the value of the corresponding position in the sys_pll_clk_e enumeration to the correct value of 60.
* **audio**
    * In mono mode, fixed the problem that the analog input (amic or line-in) does not work when switching channels.
    * When the channel is switched, the adc channel (adc1, adc2) of the corresponding codec also needs to be switched.
* **rf**
    * Fix the definition error of FLD_RF_IRQ_ALL in the enumeration of rf interrupt in B91.
    * Fixed the problem that the function rf_set_access_code_len only takes effect when the access code length is set to 4.
    * Fixed the function rf_set_rffe_pin that may cause the set pin to be in an unstable state during the setting process.
* **uart**
    * Fixed the problem caused by shared variables when UART0 and UART1 are working at the same time.
    * Modify the uart_set_pin interface and adjust the configuration sequence of gpio multiplexing function to avoid the generation of a signal from high to low to high when the uart pin multiplexing function is configured by the old version of the code.
    * Fixed the problem of incorrect calculation of dma receiving length in uart dma mode.
    * Modify the uart_reset interface to solve the problem that when uart is reset, the tx_done mask has been opened, and the tx_done interrupt will continue to enter.
    * Modify the uart_set_rx_timeout/uart_init interface to prevent writing 0 first and then assigning values ​​when configuring certain registers. When the configuration is 0, abnormal signals may be generated and some risks may be generated.
* **i2c**
    * Modify the internal implementation mechanism of the i2c_slave_read function, add the fifo software pointer, record the current fifo position, and avoid the slave always fetching data from the first address of the fifo when reading data, which may cause the software pointer and the hardware pointer to not correspond.
    * Modify the i2c_set_pin interface and adjust the configuration sequence of gpio multiplexing functions to avoid the generation of a signal from high to low to high when the i2c pin multiplexing function is configured by the old version of the code.
    * Modify the implementation method of the i2c_rx_irq_trig_cnt interface to avoid writing 0 first and then assigning the value when configuring the register, because when the configuration is 0, an error interrupt will be generated.
* **gpio**
    * Solved the problem of false interruption caused by incorrect configuration sequence when the rising edge triggers the interrupt when combined with the pull-up resistor.
    * After configuring IO as peripheral functions (such as I2C / SPI, etc.),then call the gpio_shutdown function to make the GPIO enter the shutdown mode. At this time, the function does not work.
* **spi**
    * Fixed the problem that the mixed use of SPI DMA mode and non-DMA mode will cause abnormal communication in DMA mode.
    * Fixed the problem that the SPI slave was powered on first and the CSN was floating, which caused an end interrupt.
    * The operation of clearing 0 and then writing the corresponding value is modified to a write to avoid possible abnormalities caused by the state when the register is written to 0.
    * The hspi_set_pin_mux() interface and pspi_set_pin_mux() interface modified part of the code sequence.
* **S7816**
    * Modify the s7816_set_clk interface to prevent writing 0 first and then assigning the value when configuring the register. When the configuration is 0, abnormal signals may be generated and some risks may be generated.
* **charger**
    * Fixed the problem that the charger repeatedly switched on and off during the USB charging process.
* **stimer**
    * Solve the problem that the stimer_disable interface cannot stop the stimer because the auto/manual mode is not distinguished.
* **dma**
    * The operation of clearing the reg_dma_ctrl register to 0 and then writing the target value is modified as a write to avoid abnormalities caused by writing 0 to the register.
    * Fixed the problem that any dma channel cannot be written to the dma register again when the transmission has not finished. Need to disable dma before performing the corresponding operation.
* **pm**
    * Fixed the problem that the program freezes during the suspend wakeup process when cclk is configured to 96M and pclk is configured to cclk's quarter-frequency 24M.
    * In DCDC_1P4_DCDC_1P8 mode, set the correct voltage level to avoid the risk of low voltage level of 2.8V which was used in the previous version. ( the voltage of some chips may be lower than the 2.5V required by the codec, which will affect the performance of the codec)
* **pwm(demo)**
    * In app_pwm_ir_dma.c, in pwm_ir_fifo_dma mode, the array subscript set 0 for each interrupt,otherwise, multiple interrupts will result in buff out-of-bounds access.
 * **plic** 
    * fix the problem that the plic_set_threshold() may not take effect.  
* **spi**
	* Solved the problem that sometimes compilation optimization will store the table of switch case statement in rodata segment. Encoding options add -fno-jump-tables.
* **sws_usb**
	* fix the issue that dp dm use as GPIO may cause unexpected error.firmware will disable swire through usb function,and you need to call usb_set_pin_en() function to enable it.

### Refactoring

* **spi**
    * Move spi_tx_dma_en and spi_rx_dma_en operations from demo to interface.
* **plic**
    * To facilitate application, move all interrupt handling functions to vendor/common/plic_isr.c.
* **gpio**
    * Turn on the jtag function in the gpio_init() function.
* **project**
    * The suffix of Debug file is uniformly revised to elf.
* **rf**
    * Modify the keyword nordic to private.
    * Modify the internal implementation of rf_set_rx_timeout.
    * Separate the rx settle time setting from the rf_start_srx function and use it as the rf_set_rx_settle_time function.
* **audio**
    * Refactored audio_set_i2s_clock() function.
* **dma**
    * Refactored the function of setting DMA source address and destination address to make it compatible with addresses other than sram.
* **emi**
    * Delete mode3 in continue mode, which is debug mode. Does not affect the use, the function has been verified to be normal.
* **pke**
    * Adjust the structure of pke_demo (package the algorithm calculation part into a library), and adjust the structure of the files to be sealed into a library.
* **adc**
    * Modify the interface adc_set_ref_voltage and delete the repeated operation of setting the default reference voltage.
* **plic**
    * adjust core_interrupt_disable and core_restore_interrupt interfaces.
* **pwm(demo)**
    * In app_pwm_ir_dma.c, in pwm_chain_dma mode, modify the source addresses pointed to by nodes 1 and 2 so that the output waveform starts from buff0 and then buff1 alternately outputs.
* **license**
    * update license of related files from TELINK_BSD to TELINK_APACHE


### Features

* **emi**
    * Add test 0xaa and 0xf0 two data packets in tx burst mode, and control these two modes through bdt.
* **audio**
    * Added 192K sampling rate.
* **rf**
    * Added setting rf tx/rx mi modulation interface: rf_set_tx_modulation_index and rf_set_rx_modulation_index.
* **plic_demo**
    * Add the hardware stack protection (HSP) demo in the PLIC demo.
* **spi**
    * Add interfaces pspi_csn_pin_def_e pspi_change_csn_pin() and hspi_csn_pin_def_e hspi_change_csn_pin() for switching hardware CSN in multi-slave mode.
    * Add interface spi_master_write_read_full_duplex(), which is used for SPI full-duplex communication in MCU data transmission mode.
    * Add interface spi_master_read() for master read-only data.
    * Add macro definition SPI_SLAVE_READY in Demo. 1 means the slave_ready bit is used. The master will first ask if the slave is ready, and the slave will set this bit to 1 during initialization.
    * Add the spi_master_get_slave_ready() interface in the Demo, and the master sends instructions to read the ready status of the slave.
* **flash**
    * Add a four-wire read interface, a two-wire read interface, and a four-wire write interface.
* **adc**
    * Add the interface adc_get_gpio_calib_vref to obtain the reference voltage for single-point calibration.
    * Increase the value ADC_SAMPLE_FREQ_192K of the enumeration variable adc_sample_freq_e to select the ADC sampling rate of 192Khz.
    * Add interface adc_set_gpio_two_point_calib_offset to set the offset of two-point calibration.
* **vendor/common**
    * Added interface user_read_flash_value_calib for chip calibration (including ADC calibration and RF frequency offset calibration).
* **EMI_BQB_Demo**
    * Added the macro CLOSE_INTERNAL_CAP_EN to choose whether to close the internal capacitor.
    * Removed some unnecessary codes: Code for writing frequency offset value in EMI initialization;Temporary PA setting Code.
* **spi(demo)**
    * Added macro definition SPI_SLAVE_READY_TEST to test slave_ready function.

### Performance Improvements
* **IDE**
     * Add the compile command make -j${NUMBER_OF_PROCESSORS} to speed up the compilation.
* **plic**
     * Add the always_inline modification of the function plic_isr to reduce the size of the compiled bin file.
<hr style="border-bottom:2.5px solid rgb(146, 240, 161)">


### 版本

* SDK版本: telink_b91m_driver_sdk V2.0.0。
* 此版本SDK支持B91 A0、A1和A2芯片。

### BREAKING CHANGES

* **project**
    * 修改了文件夹结构，工程中采用虚拟文件夹，并通过相对路径链接到本地文件夹。
* **pke**
    * B91增加了曲线为160，192和224的情况。
    * pke_get_irq_status、pke_clr_irq_status、pke_set_irq_mask、pke_clr_irq_mask、pke_set_microcode、pke_set_exe_cfg、pke_opr_start、pke_check_rt_code、pke_set_operand_width和pke_calc_pre_mont、pke_pre_calc_mont等接口删除。
* **swire**
    * swire_sync接口、swire_set_swm_en接口新添加一个参数，设置swm gpio。
* **uart**
    * uart_set_dma_rx_timeout接口改为uart_set_rx_timeout。
* **dma**
    * dma_mask默认是打开的，在sys_init中统一进行了清除，在上层的应用中不需要再次清除。
* **i2c**
    * i2c_master_write_read函数之前默认发送数据与接收数据中间是没有stop，修改之后，通过使用接口i2c_master_send_stop，可以选择是否有stop。
* **pwm**
    * pwm_start之前默认只能同时打开一个，重新定义枚举类型，修改之后，可以同时打开多个。
* **flash**
    * 为兼容不同的Flash型号，Flash驱动结构做了调整，修改了部分Flash接口调用方式。
    * 为兼容不同型号的Flash,flash_read_status、flash_write_status不提供给外部使用，需要使用对应接口时，需要根据mid去选择flash目录下的接口(例如：flash_write_status_midxxxxxx)。
    * 接口int flash_read_mid_uid_with_check( unsigned int *flash_mid ,unsigned char *flash_uid)的第一个参数flash_mid需要4个字节空间，第二个参数需要16byte空间。
    * 接口flash_lock、flash_unlock、flash_read_otp、flash_write_otp、flash_erase_otp、flash_lock_otp由flash_lock_midxxxxxx、flash_unlock_midxxxxxx、flash_read_otp_midxxxxxx、flash_write_otp_midxxxxxx、flash_erase_otp_midxxxxxx、flash_lock_otp_midxxxxxx替代。
    * 删除不用函数接口：flash_erase_page、flash_erase_32kblock、flash_erase_64kblock、flash_erase_chip、flash_deep_powerdown、flash_release_deep_powerdown。
* **s7816**
    * 修改s7816_set_clk参数含义，改为传入的参数就代表分频数。
* **rf**
    * 之前函数接口rf_set_tx_settle_time，rf_set_rx_settle_time设置时间值为参数 + 1，现在改为设置值与参数保持一致。
* **pm**
    * pm_set_suspend_power()接口添加一个参数用于选择进入suspend前是power down还是power on usb/npe/baseband。关闭模块时睡眠电流会减小，但是唤醒后需要重新初始化对应模块。默认全关。

### Bug Fixes

* **freertos**
    * 修复了在读写flash，被任务中断后会死机的问题。因为const 变量也访问 flash。
* **swire**
    * 修改swire_sync函数接口的驱动时间。
    * 修改swire_set_slave_id接口，防止配置寄存器时先写0，后赋值，由于配置为0时，可能产生异常信号导致一些风险。
* **flash**
    * 解决了flash的解锁接口会把四线配置复位的问题。
* **aes**
    * 修改aes数据地址的指针为基于I-RAM的偏移地址减去embase对I-RAM的偏移地址。
* **clock**
    * 修改sys_pll_clk_e枚举中对应位置的值为正确的60。
* **audio**
    * mono模式下，修复了模拟输入（amic或line-in）切换通道时不起作用问题。
	* 通道切换时，对应的codec的adc通道（adc1,adc2）也需作切换。
* **rf**
    * 修复B91中rf中断的枚举中关于FLD_RF_IRQ_ALL的定义错误。
    * 修复了函数rf_set_access_code_len只有在设置access code长度为4的时候才生效的问题。
    * 修复了函数rf_set_rffe_pin可能会导致设置的pin脚在设置过程中处于不定态。
* **uart**
    * 修复了UART0和UART1同时工作时，由于共享变量造成的问题。
    * 修改uart_set_pin接口，调整gpio复用功能配置顺序,避免旧版本代码写法配置uart pin脚复用功能时，产生一个由高到低再到高的信号。
    * 修复了uart dma模式下，计算dma接收长度错误的问题。
    * 修改uart_reset接口，解决当reset uart时，tx_done mask已经打开，会一直进入tx_done中断的问题。
    * 修改uart_set_rx_timeout/uart_init接口，防止在配置某些寄存器时先写0，后赋值，由于配置为0时，可能产生异常信号导致一些风险。
* **i2c**
    * 修改i2c_slave_read函数的内部实现机制，添加fifo软件指针，记录当前fifo的位置，避免slave读取数据时，总是从fifo的首地址取数据，可能导致软件指针与硬件指针没有对应。
    * 修改i2c_set_pin接口，调整gpio复用功能配置顺序,避免旧版本代码写法配置i2c pin脚复用功能时，产生一个由高到低再到高的信号。
    * 修改i2c_rx_irq_trig_cnt接口的实现方法，避免配置寄存器时先写0，后赋值，由于配置为0时，产生一个误中断。
* **gpio**
    * 解决了当配合上拉电阻，上升沿触发中断，由于配置顺序不对，导致误中断的问题。
    * 将IO配置为外设功能（比如I2c/SPI等）后，再调用gpio_shutdown函数让该GPIO进入shutdown模式，此时该函数不起作用。
* **spi**
    * 修复了SPI DMA模式和非DMA模式混用会造成DMA模式通信异常的问题。
    * 修复了SPI slave先上电，CSN浮空导致产生end中断的问题。
    * 先清0再写相应值的操作，修改为一次写入，避免寄存器写0时状态可能造成异常。
    * hspi_set_pin_mux()接口和pspi_set_pin_mux()接口修改了部分代码顺序。
* **S7816**
    * 修改s7816_set_clk接口，防止配置寄存器时先写0，后赋值，由于配置为0时，可能产生异常信号导致一些风险。
* **charger**
    * 修复了USB在充电过程中，charger反复开关的问题。
* **stimer**
    * 解决因为没有区分auto/manual模式而导致stimer_disable接口不能停止stimer的问题。
* **dma**
    * reg_dma_ctrl寄存器先清0再写目标值的操作，修改成一次写入，避免寄存器写0造成异常。
    * 修复了任一dma通道在传输还未结束时，不能再次写入dma寄存器的问题，需要先disable dma再进行相应的操作。
* **pm**
    * 修复了cclk配置为96M，pclk配置为cclk的四分频24M时，程序在suspend唤醒过程中，程序卡死的问题。
    * DCDC_1P4_DCDC_1P8模式下，设置正确的电压档位，规避之前版本使用电压低档位，可能造成2.8V电压偏低的风险。（某些芯片该电压可能会低于codec要求的2.5V，会影响codec性能）
* **pwm(demo)**
    * 在app_pwm_ir_dma.c中，模式PWM_IR_FIFO_DMA下，每次进中断需将数组下标归0，否则多次中断，导致buff越界访问;
* **plic** 
  * 修复 plic_set_threshold() 可能不生效的问题。
* **spi** 
  * 解决了有时编译优化会将switch case语句的表存放在rodata段的问题。编码选项添加-fno-jump-tables。
* **sws_usb** 
  * 解决当dp dm做GPIO使用时可能会导致异常错误的问题。firmware默认会disable swire through usb功能，当需要使用该功能时需要调用usb_set_pin_en接口。

### Refactoring
* **spi**
    * 将spi_tx_dma_en和spi_rx_dma_en操作从demo中移到了接口中。
* **plic**
    * 为了方便应用，将所有的中断处理函数移动到vendor/common/plic_isr.c。
* **gpio**
    * 在gpio_init()函数中打开jtag功能。
* **project**
    * Debug文件后缀统一修改为elf。
* **rf**
    * 将关键字nordic修改为private。
    * 修改rf_set_rx_timeout内部实现方式。
    * 将rx settle时间设置从rf_start_srx函数中分离出来作为 rf_set_rx_settle_time函数使用。
* **audio**
    * 重构audio_set_i2s_clock()函数。
* **dma**
    * 重构了设置dma源地址、目的地址函数，使其可以兼容除sram以外的其他地址。
* **emi**
    * 删除continue模式下的mode3，该模式为debug模式。不影响使用，已验证功能正常。
* **pke**
    * 将pke_demo结构进行调整（算法运算部分封装成库），对要封成库的文件进行结构调整。
* **adc**
    * 修改接口adc_set_ref_voltage，删除设置默认参考电压的重复操作。
* **plic**
    * 调整core_interrupt_disable和core_restore_interrupt接口。
* **pwm(demo)**
    * 在app_pwm_ir_dma.c中，模式PWM_CHAIN_DMA下，修改节点1和节点2指向的源地址，使得输出的波形从buff0开始再到buff1交替输出;
* **license**
    * 将相关文件的许可证从 TELINK_BSD 更新为 TELINK_APACHE


### Features
* **emi**
    * tx burst模式下添加测试0xaa和0xf0两种数据包，通过bdt控制这两种模式。
* **audio**
    * 增加了192K采样率。
* **rf**
    * 新增设置rf tx/rx mi调制接口：rf_set_tx_modulation_index和rf_set_rx_modulation_index。
* **plic_demo**
    * 在PLIC demo里添加硬件栈保护（HSP）demo。
* **spi**
    * 添加接口pspi_csn_pin_def_e pspi_change_csn_pin()和hspi_csn_pin_def_e hspi_change_csn_pin()，用于多slave模式下切换硬件CSN。
    * 添加接口spi_master_write_read_full_duplex(),用于MCU传输数据方式的SPI全双工通信。
    * 添加接口spi_master_read(),用于master只读数据。
    * 在Demo中添加宏定义SPI_SLAVE_READY。1表示使用slave_ready位，master会先询问slave是否准备好，slave会在初始化时将该位设置为1。
    * 在Demo中添加spi_master_get_slave_ready()接口，master发送指令读取slave的ready状态。
* **flash**
    * 增加四线读接口，增加两线读接口，增加四线写接口。
* **adc**
    * 增加接口adc_get_gpio_calib_vref，获取单点校准的参考电压。
    * 增加枚举变量adc_sample_freq_e的值ADC_SAMPLE_FREQ_192K，用于选择ADC采样率192Khz。
    * 增加接口adc_set_gpio_two_point_calib_offset，设置两点校准的offset。
* **vendor/common**
    * 增加接口user_read_flash_value_calib，用于芯片校准(包括ADC校准和RF频偏校准)。
* **EMI_BQB_Demo**
    * 增加了宏CLOSE_INTERNAL_CAP_EN，用来选择是否关闭内部电容。
    * 去除了一些不必要的代码：EMI初始化中写频偏值的代码；用于PA设置的临时代码。
* **spi(demo)**
    * 增加宏定义SPI_SLAVE_READY_TEST，用于测试slave_ready功能。
	
### Performance Improvements
* **IDE**
    * 添加编译命令make -j${NUMBER_OF_PROCESSORS}，增快编译速度。
* **plic**
    * 添加函数plic_isr的always_inline修饰，减少编译的bin文件大小。
	
---
## V1.5.0-Beta

### Version

* SDK version : telink_b91_driver_sdk v1.5.0-Beta.
* This version sdk support B91 A0，A1 and A2 chip.

### Bug Fixes

* **usb**
  * Fix the problem that usb_spk could not work when only USB_SPEAKER_ENABLE is turned on in USB_SPK Demo.
* **audio**
  * Corrct spelling errors in "audio_buff" in AUDIO Demo.
  * Correct the data type definition error in the app_sin_data.h file in AUDIO_Demo.
* **spi**
  * Interface hspi_change_csn_pin()/pspi_change_csn_pin() have logic error, would configure the unused csn pin, the error has been fixed.And hspi_change_csn_pin()/pspi_change_csn_pin() has been deleted from the driver and moved to SPI_Demo.
  * Interface spi_slave_set_pin() have a pin configuration bug, the bug has been fixed.
  * Fixed the comment error of the hspi address length of the hspi_set_addr_len function.
* **rf**
  * Fixed the following problem: When using the function rf_clr_irq_status () to clear a certain rf interrupt status, other rf interrupts may  be cleared by mistake.
* **uart** 
  * add parameter of uart_num in function "uart_get_dma_rev_data_len() to distinguish UART0 and UART1".

### Features

* **freertos**
  * add freertos Demo

### Known issues

* N/A

### BREAKING CHANGES

* **flash**
  * In order to be compatible with other sdk, the flash_read_mid function used to pass in a buf pointer, and instead returns an unsigned int variable.
  * Mid is changed from 3byte to 4byte, and the highest byte is used to distinguish flashes with the same mid but different functions.

<hr style="border-bottom:2.5px solid rgb(146, 240, 161)">


### 版本

* SDK版本: telink_b91_driver_sdk v1.5.0-Beta。
* 此版本SDK支持B91 A0、A1和A2芯片。

### Bug Fixes

* **usb**
  * 修复在USB_SPK Demo中仅打开USB_SPEAKER_ENABLE时，usb_spk无法工作的问题。
* **audio**
  * 更正在AUDIO_DEMO中“audio_buff”中的拼写错误。
  * 更正在AUDIO_DEMO中app_sin_data.h文件中数据类型定义不符的错误。
* **spi**
  * Hspi_change_csn_pin()/Pspi_change_csn_pin() 接口有逻辑错误，会配置没有用到的csn引脚，该问题已经修复。并且Hspi_change_csn_pin()/Pspi_change_csn_pin()接口已经从Drvier中删除并移到了SPI_Demo中。
  * spi_slave_set_pin()接口引脚配置有错误，该问题已经修复。
  * 修复了hspi_set_addr_len函数的hspi地址长度的注释错误。
* **rf**
  * 修复了如下问题：在使用函数rf_clr_irq_status ()清除rf某个中断状态时，有可能会误清除其他rf中断。
* **uart**
  * 函数uart_get_dma_rev_data_len()增加参数uart_num区分UART0和UART1。

### Features

* **freertos**
  * 增加 freertos Demo

### Known issues

* N/A

### BREAKING CHANGES

* **flash**
  * 为了兼容其他sdk，flash_read_mid函数以前是传入一个buf指针，改为返回unsigned int型变量。
  * mid由3byte改为4byte,最高byte用于区分mid相同但是功能存在差异的flash。

---
## V1.4.0-Beta

### Version
* SDK version : telink_b91_driver_sdk v1.4.0-Beta.
* This version sdk support B91 A0，A1 and A2 chip.

### Bug Fixes

* **pm**
  * solve the problem that pragrem can not run when VBAT power supply is greater than 4.1V,and sys_int configer power mode as DCDC mode.
  * During the stability test, the program freezes after running for a period of time.
* **spi** 
  * spi enum SPI_MODE1、SPI_MODE2 definition as error value, fixed as correct value.
* **uart**
  * when close the irq in dma mode,the receive buffer maybe overflow if dma receive data continuous.

### Features

* **pwm**
  * add function interface "pwm_32k_chn_dis".
* **audio** 
  * Add the interface for selecting DAC output channel (audio_set_output_chn).
* **usb**
  * Add software to handle set_configuration control transmission.
  * Support usb cdc to send wMaxPacketSize (64bytes) length data.
* **uart** 
  * Add function uart_get_dma_rev_data_len() in DMA mode.
    

### Refactoring

* **spi** 
  * adjusted the initial process of LED in SPI_Demo.
* **audio**
  * Adjust the calls of i2c (control codec) and pwm (provide external clock) interfaces in audio applications.
* **usb/audio**
  * set the sampling rate of the DAC to 16k by default,In the mic_spk demo.

### Known issues

* N/A
### BREAKING CHANGES
* **audio**
  * Modify the audio_i2s_config function parameters, add audio_i2s_invert_config structure parameters to select i2s_lr_clk_invert and i2s_data_invert.

<hr style="border-bottom:2.5px solid rgb(146, 240, 161)">

### 版本

* SDK版本: telink_b91_driver_sdk v1.4.0-Beta。
* 此版本SDK支持B91 A0、A1和A2芯片。

### Bug Fixes

* **pm**
  * 解决VBAT供电高于4.1V且sys_int配置供电模式为DCDC时程序不能运行的问题。
  * 稳定性测试时，运行一段时间程序卡死的问题。
* **spi**
  * spi 的SPI_MODE1、SPI_MODE2枚举值定义错了，修复为了正确值。
* **uart**
  * DMA模式下关闭总中断时，如果UART仍在接收数据，有可能导致接收数组溢出。

### Features

* **pwm**
  * 添加函数接口pwm_32k_chn_dis。
* **audio**
  * 增加invert I2S CLK接口（audio_invert_i2s_lr_clk）。
* **usb**
  * 添加软件处理set_configuration控制传输。
  * 支持USB CDC发送wMaxPacketSize（64bytes）长度数据。
* **uart**
  * DMA模式增加函数uart_get_dma_rev_data_len()。


### Refactoring

* **spi** 
  * 调整了SPI_Demo中LED的初始化过程。
* **audio**
  * 调整AUDIO应用中i2c（控制codec）和pwm(提供外部时钟)接口的调用。
* **usb/audio**
  * 在mic_spk demo中将DAC的采样率默认设为16k。

### Known issues

* N/A

### BREAKING CHANGES

* **audio**
  * 修改audio_i2s_config function函数参数，新增audio_i2s_invert_config 结构体形参，设置i2s_lr_clk_invert 和i2s_data_invert。


---
## V1.3.1-Beta

### Version
* SDK version : telink_b91_driver_sdk V1.3.1-Beta.
* This version sdk support B91 A0，A1 and A2 chip.

### Bug Fixes
* **pm**:

 * Use digital register way to get 32k tick may read error tick,cause the wakeup time is incorrect with the setting time,the sleep time will very little or very big,will not wakeup ontime.

  + The previous configuration will cause the voltage on the VDD1V8 pin to be generally high during normal operation, about 3.0V~3.1V (the reason is that the previous configuration will cause the two power supplies to be powered at the same time), which may cause abnormal phenomena. The version is modified to the correct configuration. In the default configuration, the voltage on the VDD1V8 pin will be about 2.8V.

* **USB**:
  + The header file "stdDescriptors.h" is not case sensitive when included, so it is uniformly changed to "StdDescriptors.h"

* **AUDIO**:
      + In the function audio_set_dmic_pin(), fix the problem of possible errors in setting the dmic pin

### Known issues
* N/A
### BREAKING CHANGES
* **pwm**
   + Detailed description: add 32k clock calibration,the interface function(pwm_32k_chn_en) changes" =" to |=，to prevent reset other channels.

<hr style="border-bottom:2.5px solid rgb(146, 240, 161)">

### 版本

* SDK版本: telink_b91_driver_sdk V1.3.1-Beta。
* 此版本SDK支持B91 A0、A1和A2芯片。

### Bug Fixes

* **pm**:
  * 使用数字方式获取32k tick出错会导致设置的唤醒tick与预期不符，从而导致唤醒时间与设置的不相符，睡眠时间可能过小或者过大，可能会表现为不唤醒。
 + 之前的配置会导致正常工作时，VDD1V8引脚上的电压会普遍偏高，大概为3.0V~3.1V（原因是之前的配置会导致两个电源会同时供电），可能会造成异常现象，此版本修改为正确配置，默认配置下，VDD1V8引脚上的电压会是2.8V左右。

* **USB**:
    + 头文件"stdDescriptors.h",include时没有区分大小写，统一改成"StdDescriptors.h"


* **AUDIO**:
      + 在函数audio_set_dmic_pin()中，修复设置dmic pin可能出错问题

### Known issues
* N/A

### BREAKING CHANGES
* **pwm**
 + 添加32k时钟源校准，接口函数pwm_32k_chn_en修改，=改为|=，防止重置其他通道.

---

### Version



## V1.3.0-Beta

### Version
* SDK version : telink_b91_driver_sdk V1.3.0-Beta.
* This version sdk support B91 A0，A1 and A2 chip.

### Bug Fixes

* **pm**:
  * Solved the problem that the reboot state cannot be read after the reboot comes back, and the deep state is read only. The solution is to add a status "MCU_STATUS_REBOOT_DEEP_BACK" to the enumeration type "pm_mcu_status".
  * Solved the problem that the timer could not wake up after waking up several times when the function clock_cal_32k_rc() was not called.
  * Solved the problem that when the flash is configured with four wires, it cannot work normally after waking up from suspend mode.
* **clock**: 
  * Solved the problem of invalid kick with PD0 port when 32k xtal starts to vibrate.
* **adc**: 
  * Fix the sort bug of function adc_sort_and_get_average_code() in ADC_Demo.
  * Fix the problem that the adc_get_temperature()/adc_get_voltage() function in ADC_Demo may not get data for the first time.

### Features

* **swire**
  * add swire driver and demo.
* **cproject**: 
  * delete B91 keyword.
* **adc**
  * Added the 1/4 pre_scale parameter enum of the adc_set_scale_factor() function, and deleted the 1, 1/2, 1/8 pre_scale parameter enum.
* **pm** 
  * change the variable name g_pm_multi_addr ->g_pm_mspi_cfg.
    
  * Improved the problem of incorrect LED configuration during the suspend sleep current test in pm_demo.
* **audio** 
  * add audio_set_codec_wnf api.
  * add input_mode_select at line_in.
  * set all gain to 0db default in audio.c.
* **flash** 
  * add flash_set_xip_config function.
* **usb** 
  * add host speaker stop playback detection function in spk demo.
* **pwm**
  * add function interface pwm_32k_chn_update_duty_cycle.


### Known issues
* N/A
### BREAKING CHANGES
* **s7816**
  * modify the s7816_set_time(),delete the atr wait time in the function of coldreset and warmreset.

<hr style="border-bottom:2.5px solid rgb(146, 240, 161)">

### 版本

* SDK版本: telink_b91_driver_sdk V1.3.0-Beta。
* 此版本SDK支持B91 A0、A1和A2芯片。

### Bug Fixes

* **pm**:
  * 解决了reboot回来后不能读到reboot状态，只读到deep状态的问题。解决方案是在枚举类型"pm_mcu_status"中新增一种状态"MCU_STATUS_REBOOT_DEEP_BACK"。 
  * 解决了不调用函数clock_cal_32k_rc()时，timer唤醒几次后不能再唤醒的问题。
  * 解决了设置flash四线时，suspend唤醒后flash不能正常工作的问题。
* **clock**:
  * 解决了32k xtal起振时用PD0口kick无效的问题。
* **adc**:
  * 修复了ADC_Demo中的adc_sort_and_get_average_code()函数的排序问题。
  * 修复了ADC_Demo中的adc_get_temperature()/adc_get_voltage()函数第一次可能会采不到数据的问题。

### Features

* **swire**
  * 增加swire的驱动和demo。
* **cproject**:
  * 删除关键字B91。
* **adc**
  * 添加了adc_set_scale_factor（）函数的1/4 pre_scale参数枚举，并删除了1、1 / 2、1 / 8 pre_scale参数枚举。


* **pm**

  * 更改变量名称g_pm_multi_addr-> g_pm_mspi_cfg。
  * 改进了在pm_demo中的suspend睡眠电流测试过程中，指示灯配置错误的问题。

* **audio** 
  * 添加接口audio_set_codec_wnf。
  * 在line_in添加input_mode_select。
  * 在audio.c中将所有增益设置为0db默认值。

* **flash** 

  * 添加函数flash_set_xip_config 。

* **usb** 

  * 在spk_demo中添加主机扬声器停止播放的检测功能。

* **pwm**

  * 添加功能接口pwm_32k_chn_update_duty_cycle。




### Known issues
* N/A

### BREAKING CHANGES
* **s7816**

  * 修改函数s7816_set_time()，在coldreset和warmreset函数中删除atr等待时间。


---
## V1.2.0-Beta

### Version
* SDK version : telink_b91_driver_sdk V1.2.0-Beta.
* This version sdk support B91 A0 and A1 chip.

### Bug Fixes

* **uart** 

  * Fix the problem that the configuration function uart\_receive_dma cannot continue to receive data after UART suspend or reset UART

### Features
* **sys**
  * Modify the member name of the parameter enumeration vbat\_type\_e variable in the sys\_init function
* **watchdog**
	* Simplify the parameters of the wd\_set\_interval\_ms function, and no longer retain the parameter tick\_per\_ms
* **timer**
   * In the timer\_set\_mode function, the initial tick and capture tick are no longer configured and are encapsulated into two interfaces (timer\_set\_init\_tick, timer\_set\_cap_tick)
* **gpio** 
  * Modify default macro definition in gpio_default.h 
  * The corresponding enable function gpio\_irq\_en,gpio\_gpio2risc0\_irq\_en,gpio\_gpio2risc1\_irq\_en is no longer called in the gpio\_set\_irq,gpio\_set\_gpio2risc0\_irq,gpio\_set\_gpio2risc1\_irq function, which is flexibly called in the application layer
* **plic**
  * Delete the parameter of the function plic\_interrupt\_claim
* **other**  
  * Optimize the include structure of the header file, add Include paths in the project configuration options

### Known issues
* **32k xtal** 
  - 32k xtal can't be used on the A1 version chip
* **gpio** 
  - GPIO_PD0 can't be used on the A1 version chip
* **other**
  - Note (A1 version chip): Use 2.8V voltage to power the chip. When the power supply voltage is higher than 2.8V, the program may not run.
### BREAKING CHANGES
* N/A

<hr style="border-bottom:2.5px solid rgb(146, 240, 161)">

### Bug Fixes

* **uart** 

  * 修复UART suspend起来或reset UART后，配置函数uart\_receive\_dma不能继续接收数据问题

### 版本
* SDK版本: telink_b91_driver_sdk v1.2.0-Beta.
* 此版本SDK支持B91 A0和A1芯片.
### Features
* **sys**
  * 在sys\_init函数中修改枚举vbat\_type\_e类型的成员名称。
* **watchdog**
	* 简化wd\_set\_interval_ms函数的参数,参数period\_ms为间隔时间，不再保留参数tick\_per\_ms
* **timer**
  * 在timer\_set\_mode函数中,不再配置初始tick和capture tick，分别封装成两个接口（timer\_set\_init_tick,timer\_set\_cap\_tick）
* **gpio** 
  * 修改gpio_default.h默认宏定义
  * 在gpio\_set\_irq,gpio\_set\_gpio2risc0_irq/gpio\_set\_gpio2risc1\_irq函数中不再调用对应使能函数gpio\_irq\_en,gpio\_gpio2risc0\_irq\_en,gpio\_gpio2risc1\_irq\_en,在应用层里灵活调用
* **plic**
  * 删去函数plic\_interrupt\_claim的参数
* **other**  
  * 优化头文件包含结构，在工程配置选项里添加Include paths
### Known issues
* **32k xtal** 
   - 32k xtal 在A1版本芯片上不能使用。
* **gpio** 
  - GPIO_PD0在A1版本芯片上不能使用
* **其他**
  - 注意事项（A1版本芯片）：使用2.8V电压给芯片供电，供电电压高于2.8V时，程序有可能会跑不起来。

### BREAKING CHANGES
* N/A

---
## V1.1.0-Beta
### Version

* SDK version : telink_b91_driver_sdk v1.1.0-Beta.
* This version sdk support B91 A0 and A1 chip.

### Bug Fixes

* **pm**

  * Solved the problem of not being able to wake up when the sleep time is less than 950us.

  * Solved the problem that the sleep function cannot be awakened when the sleep function is called immediately within 500us after power on.

* **mdec**: 

  * Solved the problem of first interruption after initialization.

* **clock**: 

  * Solve the problem that the program sometimes stops in the clock initialization function (clock_init).

* **lpc**: 

  * Solved the problem of abnormal operation of lpc.

* **flash**: 

  * Remove the declaration of the function flash_read_mid_uid_with_check_ram. Added declaration of function flash_read_mid_uid_with_check.

  * Solved the problem of not being able to cross pages when writing flash across pages.

* **bqb:** 

  * fix the issue that could not send packets through instructions in initialization state.

* **rf:** 

  * Fixed an abnormal function of rf_set_rffe_pin function.

  * Modify the function rf_set_rx_dma and rf_set_tx_dma to adapt to sending and receiving long packets(up to 255 byte)

  * Modified the usage example of rx　interrupt. 

  * fix the issue that rf_set_pipe_access_code can not set 5-byte access code.

  * Fix the register setting error in the rf_set_preamble_len function.

  * Modify the access code of S2 and S8 mode to manual trigger.

* **emi** 

  * Fix the problem of zigbee mode 2480Mhz band edge can not pass the spec.
  * Fixed single carrier frequency offset in emi.

* **.s** 

  * Modified cstartup_b91_flash.S and cstartup_b91_ram.S file to reserve space. 

* **reboot&clock**

  * fix the issue that reboot will crash when hclk is half of cclk in pll_clk and pad24M_clk,need add a 

* **analog** 

  * fix a compile error in analog demo

* **spi** 

  * Simplify structure and macro definition naming, optimize SPI slave demo structure，fix compilation error

* **uart** 

  * Fix the issue that the mask might be overwritten by the set of bit width when the uart-init.

### Features
* **sys**

  * Add parameters vbat_v in the sys_init function.

* **sys_time**

  * Change CLOCK_16M_SYS_TIMER_CLK_XXX to SYSTEM_TIMER_TICK_XX.

* **pm** 

  * Increase the convergence calibration function of sleep time.

  * Increase USB wake-up function.

  * Increase the suspend sleep function.
  * add long sleep wakeup api.

* **bqb** 

  * add a macro to switch new/old setting ways to configure uart/cap/flash.

* **license** 

  * change the author to "Driver Group" and modify the license template to meet the requiements CI checking.

* **rf** 

  * Add ANT mode to rf demo.

  * update the function rf_set_chn and rf_set_ble_chn to support out-of-band frequency point setting

  * In order to facilitate the development of SDK, the DMA setting function related to RF is divided into several functions with single function, and added to the driver.

* **AES** 

  * optimize AES_set_mode apiuart:add Single line transceiver function

* **uart**

  * add UART-TXDONE/RXDONE interrupt in DMA mode in the chip version of A1 .

  * data of variable length is acceptable in uart-dma mode in the chip version of A1, the length of receive-date can be work out.

* **s7816**

  * add s7816 demo, can communicate with the equipment that support IS07816 protocol.

* **gpio**

  * add 30k-digital pull-up。

* **plic**

  * Optimized interrupt handling function structure in PLIC

* **i2c**

  * add function that the master sends a stop after detecting a NACK in the ID phase，then stops reading and writing。

  * Add tx_done interrupt in dma demo

* **audio**

  * In MONO mode, the input channel (left/right) can be selected

  * Add input channel acquisition mode selection (single-ended/differential)

  * Add configuration for ADC and DAC to work at different sampling rates（ADC=16K，DAC=48K）

* **spi**

  * Add 3line mode and master to multiple-slave function

* **adc**

  * Add ADC module  in the chip version of A1 which support DMA sampling mode and manual sampling mode, support Vbat\GPIO\Temperature sensor sampling

### Known issues
* **32k xtal** 
  - 32k xtal can't be used on the A1 version chip
* **gpio** 
  - GPIO_PD0 can't be used on the A1 version chip
* **other**
  - Note (A1 version chip): Use 2.8V voltage to power the chip. When the power supply voltage is higher than 2.8V, the program may not run.
### BREAKING CHANGES
* N/A

<hr style="border-bottom:2.5px solid rgb(146, 240, 161)">

### Bug Fixes

* **pm**  

  * 解决了睡眠时间小于950us时，不能唤醒的问题。 

  * 解决了上电500us内立即调用睡眠函数时无法唤醒的问题。

* **mdec**  

  * 解决了初始化后先进一次中断的问题。

* **clock**  

  * 解决了程序有时会停在时钟初始化函数（clock_init）中的问题。

* **lpc**  

  * 解决了lpc工作异常的问题。

* **flash**  

  * 去掉函数flash_read_mid_uid_with_check_ram的声明。新增函数flash_read_mid_uid_with_check的声明。

  * 解决了跨页写flash时，不能跨页的问题。

* **bqb** 

  * 修复了在初始化状态下，不能通过指令发包的问题。

* **rf** 

  * 修复了函数rf_set_rffe_pin功能异常 。

  * 修改函数参数rf_set_rx_dma ,rf_set_tx_dma以适应长包（最大长度255byte）

  * 修改rx interrupt使用示例

  * 修复了rf_set_pipe_access_code不能设置5byte长度的access code

  * 修复了rf_set_preamble_len函数中的寄存器设置错误

  *  修改S2,S8模式access code为手动触发

* **emi** 

  * 修复zigbee 2480Mhz边带不能过spec的问题
  * 修复单通频偏的问题

* **.s** 

  * 更新s文件以预留空间

* **reboot&clock** 

  * 修复当hclk等于1/2cclk时reboot出现宕机的问题。

* **analog** 

  * 修复analog_demo中的错误

* **spi** 

  * 精简结构体和宏定义命名，优化SPI slave demo结构，修复编译错误

* **uart** 

  * 修复UART初始化设置位宽时可能会覆盖掉mask的问题

### 版本
* SDK版本: telink_b91_driver_sdk v1.0.0-Beta.
* 此版本SDK支持B91 A0和A1芯片.
### Features
* **sys**

  * 在sys_init函数中添加参数vbat_v。

* **sys_time**

  * 将CLOCK_16M_SYS_TIMER_CLK_XXX更改为SYSTEM_TIMER_TICK_XX。

* **pm**

  * 增加睡眠时间的收敛校准功能。

  * 增加USB唤醒功能。 

  * 增加suspend睡眠功能。    
  * 新增长睡眠接口。

* **bqb** 

  * 增加了一个宏，用来选择新的或旧的方式来配置串口、CAP值以及适配的Flash大小。

* **license** 

  * 将License的作者统一改成了“Driver Group”以及统一调整了License的模板。    

* **rf** 

  * 添加ANT模式到rf demo中.

  * 更新设置channel的函数rf_set_chn和rf_set_ble_chn以支持带外频点设置

  * 为了方便SDK的开发将DMA设置函数拆分成几个功能单一的函数并添加到驱动中。

* **AES** 

  * 优化AES_set_mode接口uart:新增单线收发模式

* **uart**

  * A1版本芯片新增DMA模式UART-TXDONE/RXDONE中断。

  * A1版本芯片UART-DMA模式可以接收不定长的数据，并能计算出接收数据的长度

* **s7816**

  * 新增s7816模块，可与支持ISO7816协议的设备进行单线半双工通信。

* **gpio**

  * 新增30k数字上拉。

* **plic**

  * 在PLIC中优化了中断处理函数结构

* **i2c**

  * 增加了master在发ID阶段检测到NACK后发stop，并停止读写动作

  * 在dma demo中增加了tx_done中断。

* **audio**

  * MONO模式下，可选择输入通道（左/右)，默认只有左通道输出；

  * 增加了输入通道采集模式选择（单端/差分）

  * 新增ADC和DAC在不同采样率工作的配置（ADC=16K，DAC=48K）。

* **spi**

  * 新增3line模式和master对多slave功能。

* **adc**

  * A1版本芯片新增ADC模块，支持DMA采样模式和手动采样模式，支持Vbat\GPIO\Temperature sensor采样


### Known issues
* **32k xtal** 
   - 32k xtal 在A1版本芯片上不能使用。
* **gpio** 
  - GPIO_PD0在A1版本芯片上不能使用
* **其他**
  - 注意事项（A1版本芯片）：使用2.8V电压给芯片供电，供电电压高于2.8V时，程序有可能会跑不起来。

### BREAKING CHANGES
* N/A

---
## V1.0.0-Beta
### Version
* SDK version : telink_b91_driver_sdk v1.0.0-Beta.
* This version sdk support B91 A0 chip.
### Features
* AES/AUDIO/FLASH/LPC/MDEC/PKE/PM/PWM/RF/STIMER/TIMER/TRNG
* Interface:GPIO/I2C/SPI/UART/USB
### Known issues
* **pm** 
  - The suspend mode can't be used on the A0 version chip
  - In order to avoid the problem of abnormal overwrite sram in the A0 chip, a special treatment is added to the sys_init function 
    (if it is the A0 chip and it is the first power-on, it will enter deep, wake up after 100ms, and the program will be executed normally after wake-up)
* **gpio** 
  - GPIO_PD can't be used on the A0 version chip
* **uart** 
  - It is recommended to use the non-DMA mode. In the A0 version, the function of the DMA mode is defective. When receiving, the interrupt can be generated normally 
    only when the received data length is known.
* **clock**
  - The use of clock_init function needs to pay attention to two points:
   - CCLK_DIV2_TO_HCLK and HCLK_DIV4_TO_PCLK can’t be set at the same time
   - If the pm or reboot function is used, the parameter hclk_div cannot be set to CCLK_DIV2_TO_HCLK
* **other**
  - Precautions for the use of the development board (A0 version chip):
  If USB power supply, you need to unplug the USBVBUS-VBUS jumper (that is, you can't use +5V directly to the Vbus of the chip), otherwise it will cause USB/Audio 
  instability.
### BREAKING CHANGES
* N/A

<hr style="border-bottom:2.5px solid rgb(146, 240, 161)">

### 版本
* SDK版本: telink_b91_driver_sdk v1.0.0-Beta.
* 此版本SDK支持B91 A0芯片.
### Features
* AES/AUDIO/FLASH/LPC/MDEC/PKE/PM/PWM/RF/STIMER/TIMER/TRNG
* Interface:GPIO/I2C/SPI/UART/USB
### Known issues
* **pm** 
   - suspend模式在A0版本芯片上不可以使用
   - 为了规避A0芯片存在的异常改写sram问题，在sys_init函数中加了一段特殊处理（如果是A0芯片，并且是第一次上电，则会进入deep，100ms后唤醒，唤醒后会正常执行程序）
* **gpio** 
  - GPIO_PD在A0版本芯片上不能使用
* **uart** 
  - 建议使用非DMA模式，在A0版本，DMA模式的功能有缺陷，接收时，只有已知接收数据长度时，才能正常的产生中断。
* **clock**
  - clock_init函数的使用需要注意一下两点:
   - CCLK_DIV2_TO_HCLK和HCLK_DIV4_TO_PCLK不能同时设置 
   - 如果有使用pm或者reboot功能，则参数hclk_div不能设置为CCLK_DIV2_TO_HCLK
* **其他**
  - 开发板使用注意事项（A0版本芯片）：如果USB供电时，需要拔掉USBVBUS-VBUS跳帽（即不能用+5V直接给芯片的Vbus），否则会导致USB/Audio不稳定

### BREAKING CHANGES
* N/A