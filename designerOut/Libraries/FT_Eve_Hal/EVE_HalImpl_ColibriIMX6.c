/**
* This source code ("the Software") is provided by Bridgetek Pte Ltd
* ("Bridgetek") subject to the licence terms set out
*   http://brtchip.com/BRTSourceCodeLicenseAgreement/ ("the Licence Terms").
* You must read the Licence Terms before downloading or using the Software.
* By installing or using the Software you agree to the Licence Terms. If you
* do not agree to the Licence Terms then do not download or use the Software.
*
* Without prejudice to the Licence Terms, here is a summary of some of the key
* terms of the Licence Terms (and in the event of any conflict between this
* summary and the Licence Terms then the text of the Licence Terms will
* prevail).
*
* The Software is provided "as is".
* There are no warranties (or similar) in relation to the quality of the
* Software. You use it at your own risk.
* The Software should not be used in, or for, any medical device, system or
* appliance. There are exclusions of Bridgetek liability for certain types of loss
* such as: special loss or damage; incidental loss or damage; indirect or
* consequential loss or damage; loss of income; loss of business; loss of
* profits; loss of revenue; loss of contracts; business interruption; loss of
* the use of money or anticipated savings; loss of information; loss of
* opportunity; loss of goodwill or reputation; and/or loss of, damage to or
* corruption of data.
* There is a monetary cap on Bridgetek's liability.
* The Software may have subsequently been amended by another user and then
* distributed by that other user ("Adapted Software").  If so that user may
* have additional licence terms that apply to those amendments. However, Bridgetek
* has no liability in relation to those amendments.
*/

#include "EVE_HalImpl.h"
#include "EVE_Platform.h"
#include "FT_DataTypes.h"
#if defined(ColibriiMX6_PLATFORM)

#include "spi.h"
void ticker();

#if defined(EVE_MODULE_PANL)
/* TODO: Pass these as parameters in HAL initialization, instead of here. */

uint8_t ucHeap[PANL_HEAP_SIZE];

/* uart_write((ft900_uart_regs_t*) p, (uint8_t) c); */
void tfp_putc(void *p, char c); /* Placeholder for user code */
void bacnet_notification_acked(uint8_t id); /* Placeholder for user code */
bool bacnet_msg_received(uint8_t src, const uint8_t *indata, const size_t inlen, uint8_t *outdata, size_t *outlen); /* Placeholder for user code */
void bacnet_unconf_msg_received(uint8_t src, const uint8_t *indata, const size_t inlen); /* Placeholder for user code */
#endif /* #if defined(EVE_MODULE_PANL) */

#if defined(__FT930__)
static const uint8_t s_SpimGpio[5] = { 30, 31, 32, 33, 29 };
static const pad_dir_t s_SpimFunc[5] = { pad_spim_ss0, pad_spim_ss1, pad_spim_ss2, pad_spim_ss3, pad29_spim_ss0 }
#else
static const uint8_t s_SpimGpio[4] = { 28, 33, 34, 35 };
//static const pad_dir_t s_SpimFunc[4] = { pad_spim_ss0, pad_spim_ss1, pad_spim_ss2, pad_spim_ss3 };
#endif

/*********
** INIT **
*********/

EVE_HalPlatform g_HalPlatform;

/* Initialize HAL platform */
void EVE_HalImpl_initialize()
{
	g_HalPlatform.TotalDevices = 1;

#if defined(PANL70) || defined(PANL70PLUS)
	gpio_function(GOODIXGPIO, pad_goodixgpio);
	gpio_dir(GOODIXGPIO, pad_dir_output);
	gpio_write(GOODIXGPIO, 1);
#endif
}

/* Release HAL platform */
void EVE_HalImpl_release()
{
//	spi_uninit(SPIM);
}

/* Get the default configuration parameters */
void EVE_HalImpl_defaults(EVE_HalParameters *parameters)
{
//	parameters->PowerDownPin = FT800_PD_N;
//	parameters->SpiCsPin = 0; // SS0
}



/* Opens a new HAL context using the specified parameters */
bool EVE_HalImpl_open(EVE_HalContext *phost, EVE_HalParameters *parameters)
{
    Spi_pd_connect();
    Spi_cs_connect();
//	write(pd_pin, HIGH, 1);
    Spi_pdSet(HIGH);
    Spi_init(SPI_PATH,1000000);
    uint8_t dummyTx = 0;
    uint8_t dummyRx = 0;
    Spi_transfer(&dummyTx, &dummyRx, 1);
//    write(pd_pin, LOW, 1);
    Spi_pdSet(LOW);
    usleep(300*1000);
//    write(pd_pin, HIGH, 1);
    Spi_pdSet(HIGH);
    usleep(300*1000);
    Spi_init(SPI_PATH, 1000000);
    /* Initialize the context valriables */
//    host->ft_cmd_fifo_wp = host->ft_dl_buff_wp = 0;
    phost->SpiDummyBytes = 1;//by default ft800/801/810/811 goes with single dummy byte for read
    phost->SpiChannels = 0;
//    host->status = FT_GPU_HAL_OPENED;
    phost->Status = EVE_STATUS_OPENED;

}

/* Close a HAL context */
void EVE_HalImpl_close(EVE_HalContext *phost)
{
//	phost->Status = EVE_STATUS_CLOSED;
//	--g_HalPlatform.OpenedDevices;
//	spi_close(SPIM, phost->Parameters.SpiCsPin);
}

/* Idle. Call regularly to update frequently changing internal state */
void EVE_HalImpl_idle(EVE_HalContext *phost)
{
#if defined(EVE_MODULE_PANL)
	panl_bacnet_task();
#endif
}

/*************
** TRANSFER **
*************/

void EVE_Hal_startTransfer(EVE_HalContext *phost, EVE_TRANSFER_T rw, uint32_t addr)
{
	eve_assert(phost->Status == EVE_STATUS_OPENED);

    Spi_csSet(LOW);
    if (rw == EVE_TRANSFER_READ)
    {
        ft_uint8_t tx[4];
        ft_uint8_t rx[4];
        /* Compose the read packet */
        // printf("addr: 0x%06x addr: %ld\n", addr, addr);
        tx[0] = (ft_uint8_t) (addr >> 16);
        tx[1] = (ft_uint8_t) (addr >> 8);
        tx[2] = (ft_uint8_t) (addr);

        tx[3] = 0; //Dummy Read byte

        // printf("t0: 0x%02x t1: 0x%02x t2: 0x%02x t3: 0x%02x\n", tx[0], tx[1],tx[2],tx[3]);

        Spi_transfer(tx, rx, sizeof(tx));
        phost->Status = FT_GPU_HAL_READING;
    }else{


        ft_uint8_t tx[3];
        ft_uint8_t rx[3];
        /* Compose the read packet */
        tx[0] = (0x80|(addr >> 16));
        tx[1] = addr >> 8;
        tx[2] = addr;

        Spi_transfer(tx, rx, sizeof(tx));
        phost->Status = FT_GPU_HAL_WRITING;
    }

}


void EVE_Hal_endTransfer(EVE_HalContext *phost)
{
    eve_assert(phost->Status == EVE_STATUS_READING || phost->Status == EVE_STATUS_WRITING);

//	spi_close(SPIM, phost->Parameters.SpiCsPin);
    Spi_csSet(HIGH);
    phost->Status = EVE_STATUS_OPENED;
}

void EVE_Hal_flush(EVE_HalContext *phost)
{
	eve_assert(phost->Status == EVE_STATUS_OPENED);
	/* no-op */
}

static inline void rdBuffer(EVE_HalContext *phost, uint8_t *buffer, uint32_t size)
{
//	spi_readn(SPIM, buffer, size);
    while (size--) {
       (*buffer) = EVE_Hal_transfer8(phost,0);
       buffer++;
    }
}

static inline void wrBuffer(EVE_HalContext *phost, const uint8_t *buffer, uint32_t size)
{
//	spi_writen(SPIM, buffer, size);
    while (size--) {
       EVE_Hal_transfer8(phost,*buffer);
       buffer++;
    }
}

static inline uint8_t transfer8(EVE_HalContext *phost, uint8_t value)
{
    ft_uint8_t tx = value;
//    printf("value: 0x%02x\n", value);
    ft_uint8_t rx = 0;

    if (phost->Status == EVE_STATUS_READING)
    {
        Spi_transfer(&tx, &rx, sizeof(tx));
//        printf("0x%02x\n", rx);
        return rx;
    }
    else
    {
//        printf("write 8: 0x%02x\n", tx);
        Spi_transfer(&tx, &rx, sizeof(tx));
        return 0;
    }

}

uint8_t EVE_Hal_transfer8(EVE_HalContext *phost, uint8_t value)
{
    return transfer8(phost, value);
}

uint16_t EVE_Hal_transfer16(EVE_HalContext *phost, uint16_t value)
{
    uint8_t buffer[2];
    if (phost->Status == EVE_STATUS_READING)
    {
//        printf("read 16\n");
        rdBuffer(phost, buffer, 2);
        return (uint16_t)buffer[0]
            | (uint16_t)buffer[1] << 8;
    }
    else
    {
//        printf("write 16\n");
        buffer[0] = value & 0xFF;
        buffer[1] = value >> 8;
        wrBuffer(phost, buffer, 2);
        return 0;
    }
}

uint32_t EVE_Hal_transfer32(EVE_HalContext *phost, uint32_t value)
{
    uint8_t buffer[4];
    if (phost->Status == EVE_STATUS_READING)
    {
        rdBuffer(phost, buffer, 4);
        return (uint32_t)buffer[0]
            | (uint32_t)buffer[1] << 8
            | (uint32_t)buffer[2] << 16
            | (uint32_t)buffer[3] << 24;
    }
    else
    {
        buffer[0] = value & 0xFF;
        buffer[1] = (value >> 8) & 0xFF;
        buffer[2] = (value >> 16) & 0xFF;
        buffer[3] = value >> 24;
        wrBuffer(phost, buffer, 4);
        return 0;
    }
}

void EVE_Hal_transferMem(EVE_HalContext *phost, uint8_t *result, const uint8_t *buffer, uint32_t size)
{
	if (!size)
		return;

	if (result && buffer)
	{
		/* not implemented */
		eve_debug_break();
	}
	else if (result)
	{
        rdBuffer(phost, result, size);
	}
	else if (buffer)
	{
        wrBuffer(phost, buffer, size);
	}
}

void EVE_Hal_transferProgmem(EVE_HalContext *phost, uint8_t *result, eve_progmem_const uint8_t *buffer, uint32_t size)
{
	if (!size)
		return;

	if (result && buffer)
	{
		/* not implemented */
		eve_debug_break();
	}
	else if (result)
	{
		/* not implemented */
		eve_debug_break();
	}
	else if (buffer)
	{
		eve_assert(!((uintptr_t)buffer & 0x3)); // must be 32-bit aligned
		eve_assert(!(size & 0x3)); // must be 32-bit aligned
		eve_progmem_const uint32_t *buf32 = (eve_progmem_const uint32_t *)(void eve_progmem_const *)buffer;
		size >>= 2;
		while (size--)
		{
			uint32_t value = *(buf32++);
			wrBuffer(phost, (uint8_t *)(&value), 4);
		}
	}
}

uint32_t EVE_Hal_transferString(EVE_HalContext *phost, const char *str, uint32_t index, uint32_t size, uint32_t padMask)
{
	if (!size)
	{
		/* TODO: Support different padding options */
		eve_assert(padMask == 0x3);
		EVE_Hal_transfer32(phost, 0);
		return 4;
	}

	eve_assert(size <= EVE_CMD_STRING_MAX);
	uint32_t transferred = 0;
	if (phost->Status == EVE_STATUS_WRITING)
	{
		transferred += (uint32_t)strnlen(str, size) + 1;
		eve_assert(str[transferred - 1] == '\0');
		wrBuffer(phost, str, transferred);
		if (transferred & padMask)
		{
			uint32_t pad = 4 - (transferred & padMask);
			uint8_t padding[4] = { 0 };
			wrBuffer(phost, padding, pad);
			transferred += pad;
			eve_assert(!(transferred & 0x3));
		}
	}
	else
	{
		/* not implemented */
		eve_debug_break();
	}
	return transferred;
}

/************
** UTILITY **
************/

void EVE_Hal_hostCommand(EVE_HalContext *phost, uint8_t cmd)
{
	eve_assert(phost->Status == EVE_STATUS_OPENED);

    uint8_t hcmd[4] = { 0 };
    hcmd[0] = cmd;
    hcmd[1] = 0;
    hcmd[2] = 0;
    hcmd[3] = 0;

//	spi_open(SPIM, phost->Parameters.SpiCsPin);
//	spi_writen(SPIM, hcmd, 3);
//	spi_close(SPIM, phost->Parameters.SpiCsPin);


    ft_uint8_t rx[3];
    Spi_csSet(LOW);
    Spi_transfer(hcmd, rx, sizeof(hcmd));
    Spi_csSet(HIGH);
}

void EVE_Hal_hostCommandExt3(EVE_HalContext *phost, uint32_t cmd)
{
	eve_assert(phost->Status == EVE_STATUS_OPENED);

	uint8_t hcmd[4] = { 0 };
	hcmd[0] = cmd & 0xff;
	hcmd[1] = (cmd >> 8) & 0xff;
	hcmd[2] = (cmd >> 16) & 0xff;
	hcmd[3] = 0;

//	spi_open(SPIM, phost->Parameters.SpiCsPin);
//	spi_writen(SPIM, hcmd, 3);
//	spi_close(SPIM, phost->Parameters.SpiCsPin);

    ft_uint8_t rx[3];
    Spi_csSet(LOW);
    Spi_transfer(hcmd, rx, sizeof(hcmd));
    Spi_csSet(HIGH);

}

void EVE_Hal_powerCycle(EVE_HalContext *phost, bool up)
{
    if (up)
    {
        Spi_pdSet(LOW);
        EVE_sleep(20);

        Spi_pdSet(HIGH);
        EVE_sleep(20);
    }else
    {
        Spi_pdSet(HIGH);
        EVE_sleep(20);

        Spi_pdSet(LOW);
        EVE_sleep(20);
    }
}

void EVE_Hal_setSPI(EVE_HalContext *phost, EVE_SPI_CHANNELS_T numchnls, uint8_t numdummy)
{
#if (EVE_MODEL < EVE_FT810)
    return;
#else

    phost->SpiChannels = numchnls;
    phost->SpiDummyBytes = numdummy;

    ft_uint8_t writebyte = 0;
    /* error check */
    if((numchnls > FT_GPU_SPI_QUAD_CHANNEL) || (numdummy > FT_GPU_SPI_TWODUMMY) || (numdummy < FT_GPU_SPI_ONEDUMMY))
    {
        return ;//error
    }

    phost->SpiChannels = numchnls;
    writebyte = phost->SpiChannels;
    phost->SpiDummyBytes = numdummy;

    if(FT_GPU_SPI_TWODUMMY == phost->SpiDummyBytes)
    {
        writebyte |= FT_SPI_TWO_DUMMY_BYTE;
    }
    Ft_Gpu_Hal_Wr8(phost,REG_SPI_WIDTH,writebyte);
    /* set the parameters in hal context and also set into ft81x */
    return ;
#endif
}

uint32_t EVE_Hal_currentFrequency(EVE_HalContext *phost)
{
//	uint32_t t0, t1;
//	int32_t r = 15625;

//	t0 = EVE_Hal_rd32(phost, REG_CLOCK); /* t0 read */

//	__asm__(
//	    "   move.l  $r0,%0               \n\t"
//	    "   mul.l   $r0,$r0,100          \n\t"
//	    "1:                              \n\t"
//	    "   sub.l   $r0,$r0,3            \n\t" /* Subtract the loop time = 4 cycles */
//	    "   cmp.l   $r0,0                \n\t" /* Check that the counter is equal to 0 */
//	    "   jmpc    gt, 1b               \n\t"
//	    : /* Outputs */
//	    : "r"(r) /* Inputs */
//	    : "$r0"); /* Using */

//	t1 = EVE_Hal_rd32(phost, REG_CLOCK); /* t1 read */
//	return ((t1 - t0) << 6); /* bitshift 6 places is the same as multiplying 64 */


    ft_uint32_t t0, t1;
    ft_uint32_t addr = REG_CLOCK;
    ft_uint8_t spidata[4];
    ft_int32_t r = 15625;

    t0 = Ft_Gpu_Hal_Rd32(phost,REG_CLOCK); /* t0 read */
    usleep(r);
    t1 = Ft_Gpu_Hal_Rd32(phost,REG_CLOCK); /* t1 read */
    return ((t1 - t0) * 64);
}

/*********
** MISC **
*********/

static void initSdHost()
{
//	sys_enable(sys_device_sd_card);
//	sdhost_sys_init();
//	sdhost_init();

//	gpio_function(GPIO_SD_CLK, pad_sd_clk);
//	gpio_pull(GPIO_SD_CLK, pad_pull_none); // pad_pull_none
//	gpio_function(GPIO_SD_CMD, pad_sd_cmd);
//	gpio_pull(GPIO_SD_CMD, pad_pull_pullup);
//	gpio_function(GPIO_SD_DAT3, pad_sd_data3);
//	gpio_pull(GPIO_SD_DAT3, pad_pull_pullup);
//	gpio_function(GPIO_SD_DAT2, pad_sd_data2);
//	gpio_pull(GPIO_SD_DAT2, pad_pull_pullup);
//	gpio_function(GPIO_SD_DAT1, pad_sd_data1);
//	gpio_pull(GPIO_SD_DAT1, pad_pull_pullup);
//	gpio_function(GPIO_SD_DAT0, pad_sd_data0);
//	gpio_pull(GPIO_SD_DAT0, pad_pull_pullup);
//	gpio_function(GPIO_SD_CD, pad_sd_cd);
//	gpio_pull(GPIO_SD_CD, pad_pull_pullup);
//	gpio_function(GPIO_SD_WP, pad_sd_wp);
//	gpio_pull(GPIO_SD_WP, pad_pull_pullup);
}

void EVE_Mcu_initialize()
{
#if (defined(EVE_MODULE_PANL))
#ifndef EVDEMO
	init_printf(UART0, tfp_putc);
#endif
	panl_board_cfg bcfg;
#if defined(PANL35)
	panl_board_cfg_set_defaults(PANL35_GENERIC_V2, &bcfg);
#elif defined(PANL70) || defined(PANL70PLUS)
	panl_board_cfg_set_defaults(PANL70_PLUS_V2, &bcfg);
#endif

	panl_result res = panl_init(&bcfg);
	panl_bacnet_cfg_s cfg = { .msg_rxed_cb = bacnet_msg_received, .notif_ack_cb = bacnet_notification_acked, .unconf_msg_rxed_cb = bacnet_unconf_msg_received, .mac = 13, .config = config_default };
	if (!panl_bacnet_init(&cfg))
	{
		eve_printf_debug("Error init bacnet!\n");
	}

	panl_led_init();

	uint8_t panlID = 0;
	res = panl_get_panl_id(&panlID);

	eve_printf_debug("\r\nPanL ID :  %04x\r\n", panlID);
#endif

//	interrupt_enable_globally();

//	sys_enable(sys_device_uart0);
//	gpio_function(GPIO_UART0_TX, pad_uart0_txd); /* UART0 TXD */
//	gpio_function(GPIO_UART0_RX, pad_uart0_rxd); /* UART0 RXD */
//	uart_open(UART0, /* Device */
//	    1, /* Prescaler = 1 */
//	    UART_DIVIDER_115200_BAUD, /* Divider = 1302 */
//	    uart_data_bits_8, /* No. Data Bits */
//	    uart_parity_none, /* Parity */
//	    uart_stop_bits_1); /* No. Stop Bits */

#if (defined(ENABLE_ILI9488_HVGA_PORTRAIT) || defined(ENABLE_KD2401_HVGA_PORTRAIT))
	/* asign all the respective pins to GPIO and set them to default values */
	gpio_function(GPIO_ILI9488_DCX, pad_ili9488_dcx);
	gpio_dir(GPIO_ILI9488_DCX, pad_dir_output);
	gpio_write(GPIO_ILI9488_DCX, 1);

	gpio_function(GPIO_SPIM_CLK, pad_spim_sck);
	gpio_dir(GPIO_SPIM_CLK, pad_dir_output);
	gpio_write(GPIO_SPIM_CLK, 1);

	gpio_function(GPIO_SPIM_MOSI, pad_spim_mosi);
	gpio_dir(GPIO_SPIM_MOSI, pad_dir_output);
	gpio_write(GPIO_SPIM_MOSI, 1);

	gpio_function(GPIO_ILI9488_CS1, pad_ili9488_cs1);
	gpio_dir(GPIO_ILI9488_CS1, pad_dir_output);
	gpio_write(GPIO_ILI9488_CS1, 1);

	gpio_function(GPIO_SPIM_MISO, pad_spim_miso);
	gpio_dir(GPIO_SPIM_MISO, pad_dir_output);
	gpio_write(GPIO_SPIM_MISO, 1);

	gpio_function(GPIO_SPIM_SS0, pad_spim_ss0);
	gpio_dir(GPIO_SPIM_SS0, pad_dir_output);
	gpio_write(GPIO_SPIM_SS0, 1);

	gpio_function(GPIO_PWD, pad_pwd);
	gpio_dir(GPIO_PWD, pad_dir_output);
	gpio_write(GPIO_PWD, 1);

	gpio_write(GPIO_ILI9488_DCX, 1);
	gpio_write(GPIO_SPIM_SS0, 1);
	gpio_write(GPIO_PWD, 1);
	gpio_write(GPIO_ILI9488_CS1, 1);

#endif

	initSdHost();
}

void EVE_Mcu_release()
{
	/* no-op */
}

/*********
** MISC **
*********/

/* Globals for interrupt implementation */
#if !defined(EVE_MODULE_PANL)
static uint32_t s_TotalMilliseconds = 0;
#endif

void EVE_Millis_initialize()
{
#if (defined(EVE_MODULE_PANL))
	panl_timer_register_ms_callback(ticker);
#else
	s_TotalMilliseconds = 0;
//	sys_enable(sys_device_timer_wdt);
//	timer_prescaler(FT900_TIMER_PRESCALE_VALUE);
//	timer_init(FT900_FT_MILLIS_TIMER, FT900_TIMER_OVERFLOW_VALUE, timer_direction_up, timer_prescaler_select_on, timer_mode_continuous);

//	interrupt_attach(interrupt_timers, 17, ticker);
//	/* enabling the interrupts for timer */
//	timer_enable_interrupt(FT900_FT_MILLIS_TIMER);

//	timer_start(FT900_FT_MILLIS_TIMER);
#endif
}

void EVE_Millis_release()
{
#if !defined(EVE_MODULE_PANL)
//    timer_stop(FT900_FT_MILLIS_TIMER);
//    timer_disable_interrupt(FT900_FT_MILLIS_TIMER);
#endif
}

/* Need to ensure that below api is called at least once in 6.5 seconds duration for FT900 platform as this module doesnt use timer for context update */
/* global counter to loopback after ~49.71 days */
uint32_t EVE_millis()
{
#if defined(EVE_MODULE_PANL)
	return panl_timer_get_time();
#else
	/* Interrupt implementation */
	return (s_TotalMilliseconds);
#endif
}

void ticker()
{
#if !defined(EVE_MODULE_PANL)
	s_TotalMilliseconds += 1;

//	timer_disable_interrupt(FT900_FT_MILLIS_TIMER);
//	/* Clear the interrupt and increment the counter */
//	timer_is_interrupted(FT900_FT_MILLIS_TIMER);
//	timer_enable_interrupt(FT900_FT_MILLIS_TIMER);
#endif
}

void EVE_sleep(uint32_t us)
{
    usleep(us * 1000);
}

/*********
** MISC **
*********/

bool EVE_UtilImpl_bootupDisplayGpio(EVE_HalContext *phost)
{
#if defined(PANL70) || defined(PANL70PLUS)
	gpio_function(GOODIXGPIO, pad_gpio33);
	gpio_dir(GOODIXGPIO, pad_dir_output);
	gpio_write(GOODIXGPIO, 0);
	EVE_sleep(1);
	Ft_Gpu_Hal_Wr8(phost, REG_CPURESET, 0);
	EVE_sleep(100);
	gpio_dir(GOODIXGPIO, pad_dir_input);
#endif

	return true;
}


//ft_uint16_t Ft_Gpu_Cmdfifo_Freespace(Ft_Gpu_Hal_Context_t *host)
//{
//    ft_uint16_t fullness,retval;

//    //host->ft_cmd_fifo_wp = Ft_Gpu_Hal_Rd16(host,REG_CMD_WRITE);

//    fullness = (host->ft_cmd_fifo_wp - Ft_Gpu_Hal_Rd16(host,REG_CMD_READ)) & 4095;
//    retval = (EVE_CMD_FIFO_SIZE - 4) - fullness;
//    return (retval);
//}

//ft_void_t Ft_Gpu_Hal_CheckCmdBuffer(Ft_Gpu_Hal_Context_t *host,ft_uint32_t count)
//{
//    ft_uint16_t getfreespace;
//    do{
//        getfreespace = Ft_Gpu_Cmdfifo_Freespace(host);
//    }while(getfreespace < count);
//}


#endif /* #if defined(FT9XX_PLATFORM) */

/* end of file */
