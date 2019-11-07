
#include <FT_Platform.h>
#include "Ft_Esd_Utility.h"

#include "FT_Gpu_Hal.h"
#include "FT_Gpu.h"
#include "Ft_Esd.h"
#include "FT_Esd_Dl.h"
#include "Ft_Esd_GpuAlloc.h"
#include "Ft_Esd_TouchTag.h"
#include "Ft_Esd_CoCmd.h"
#include "Ft_Esd_Core.h"

extern EVE_HalContext *Ft_Esd_Host;
extern Ft_Esd_GpuAlloc *Ft_Esd_GAlloc;

#ifndef NDEBUG
static ft_uint32_t s_FlashErrorLast = ~0;
#endif

//A function to enable spinner when frame is rendered.
ESD_FUNCTION(Ft_Esd_Spinner_Popup, DisplayName = "Pop-up Spinner", Category = EsdUtilities)
void Ft_Esd_Spinner_Popup()
{
	Esd_CurrentContext->SpinnerPopup = FT_TRUE;
}

#if defined(EVE_FLASH_AVAILABLE)
#ifdef ESD_SIMULATION
extern void Esd_SetFlashStatus__ESD(int status);
extern void Esd_SetFlashSize__ESD(int size);
#else
#define Esd_SetFlashStatus__ESD(status) \
	do                                  \
	{                                   \
	} while (false)
#define Esd_SetFlashSize__ESD(status) \
	do                                \
	{                                 \
	} while (false)
#endif

void Esd_AttachFlashFast()
{
	// Wait for flash status to move on from FLASH_STATUS_INIT
	uint32_t flashStatus;
	while (!(flashStatus = Ft_Gpu_Hal_Rd32(Ft_Esd_Host, REG_FLASH_STATUS)))
	{
#ifndef NDEBUG
		eve_printf_debug("Waiting for REG_FLASH_STATUS (%u)\n", flashStatus);
#endif
	}

	// No need to continue if flash is okay
	if (flashStatus < FLASH_STATUS_FULL)
	{
		ft_uint32_t error;

		Esd_SetFlashStatus__ESD(flashStatus);

		flashStatus = Ft_Gpu_CoCmd_FlashAttach(Ft_Esd_Host);
		Esd_SetFlashStatus__ESD(flashStatus);
		Esd_SetFlashSize__ESD(Ft_Gpu_Hal_Rd32(Ft_Esd_Host, REG_FLASH_SIZE));

		flashStatus = Ft_Gpu_CoCmd_FlashFast(Ft_Esd_Host, &error);
		Esd_SetFlashStatus__ESD(flashStatus);

#ifndef NDEBUG
		if (error != s_FlashErrorLast)
		{
			s_FlashErrorLast = error;
			switch (error)
			{
			case 0:
				eve_printf_debug("Flash OK\n");
				break;
			case FLASH_ERROR_NOTATTACHED: // 0xE001 :
				eve_printf_debug("Flash is not attached\n");
				break;
			case FLASH_ERROR_HEADERMISSING: // 0xE002:
				eve_printf_debug("No header detected in sector 0. Is flash blank?\n");
				break;
			case FLASH_ERROR_HEADERCRCFAILED: // 0xE003:
				eve_printf_debug("Sector 0 flash data failed integrity check\n");
				break;
			case FLASH_ERROR_FIRMWAREMISMATCH: // 0xE004:
				eve_printf_debug("Flash device/blob mismatch. Was correct blob loaded?\n");
				break;
			case FLASH_ERROR_FULLSPEEDFAILED: // 0xE005:
				eve_printf_debug("Failed full-speed flash test. Check board wiring\n");
				break;
			default:
				eve_printf_debug("Unknown flash error (%u)\n", error);
			}
		}
#endif
	}
}
#else
#define Esd_AttachFlashFast() eve_noop()
#endif

void Esd_BeginLogo()
{
	Ft_Esd_GpuAlloc_Reset(Ft_Esd_GAlloc);
	Ft_Esd_GpuAlloc_Alloc(Ft_Esd_GAlloc, RAM_G_SIZE, 0); // Block allocation
	Ft_Gpu_CoCmd_StartFrame(Ft_Esd_Host);
	Ft_Gpu_CoCmd_DlStart(Ft_Esd_Host);
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, CLEAR(1, 0, 0));
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, DISPLAY());
	Ft_Gpu_CoCmd_Swap(Ft_Esd_Host);
	Ft_Gpu_CoCmd_DlStart(Ft_Esd_Host);
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, CLEAR(1, 0, 0));
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, DISPLAY());
	Ft_Gpu_CoCmd_Swap(Ft_Esd_Host);
	Ft_Gpu_CoCmd_DlStart(Ft_Esd_Host);
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, CLEAR(1, 0, 0));
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, DISPLAY());
	// Ft_Gpu_CoCmd_MemSet(Ft_Esd_Host, 0, 0xFF, RAM_G_SIZE);
	Ft_Gpu_CoCmd_EndFrame(Ft_Esd_Host);
	Ft_Gpu_Hal_WaitCmdFifoEmpty(Ft_Esd_Host);
	Ft_Gpu_CoCmd_StartFrame(Ft_Esd_Host);
	Ft_Gpu_CoCmd_Logo(Ft_Esd_Host);
	Ft_Gpu_CoCmd_EndFrame(Ft_Esd_Host);
	Ft_Gpu_Hal_WaitLogo_Finish(Ft_Esd_Host);
	EVE_sleep(3000);
}

void Esd_EndLogo()
{
	Ft_Gpu_CoCmd_StartFrame(Ft_Esd_Host);
	Ft_Gpu_CoCmd_DlStart(Ft_Esd_Host);
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, CLEAR_COLOR_RGB(255, 255, 255));
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, CLEAR(1, 0, 0));
	Ft_Gpu_CoCmd_SendCmd(Ft_Esd_Host, DISPLAY());
	Ft_Gpu_CoCmd_Swap(Ft_Esd_Host);
	Ft_Gpu_CoCmd_EndFrame(Ft_Esd_Host);
	Ft_Gpu_Hal_WaitCmdFifoEmpty(Ft_Esd_Host);
	Ft_Esd_GpuAlloc_Reset(Ft_Esd_GAlloc);
}

void Esd_ShowLogo()
{
	Esd_CurrentContext->ShowLogo = FT_TRUE;
}

/// Run calibrate procedure
ft_bool_t Esd_Calibrate()
{
	EVE_HalContext *phost = Ft_Esd_Host;
	ft_uint32_t result;
	ft_uint32_t transMatrix[6];

#if defined(EVE_SCREEN_CAPACITIVE)
	Ft_Gpu_Hal_Wr8(phost, REG_CTOUCH_EXTENDED, CTOUCH_MODE_COMPATIBILITY);
#endif

	eve_printf_debug("App_CoPro_Widget_Calibrate: Start Frame\n");
	Ft_Gpu_CoCmd_StartFrame(phost);

	Ft_Gpu_CoCmd_DlStart(phost);
	Ft_Gpu_CoCmd_SendCmd(phost, CLEAR_COLOR_RGB(64, 64, 64));
	Ft_Gpu_CoCmd_SendCmd(phost, CLEAR(1, 1, 1));
	Ft_Gpu_CoCmd_SendCmd(phost, COLOR_RGB(0xff, 0xff, 0xff));

	// Ft_Gpu_CoCmd_Text(phost, (Ft_Esd_Host->Parameters.Display.Width / 2), (Ft_Esd_Host->Parameters.Display.Height / 2), 27, OPT_CENTER, "Please Tap on the dot");

	result = Ft_Gpu_CoCmd_Calibrate(phost);

	eve_printf_debug("App_CoPro_Widget_Calibrate: End Frame\n");
	Ft_Gpu_CoCmd_EndFrame(phost);

	// Print the configured values
	Ft_Gpu_Hal_RdMem(phost, REG_TOUCH_TRANSFORM_A, (ft_uint8_t *)transMatrix, 4 * 6); //read all the 6 coefficients
	eve_printf_debug("Touch screen transform values are A 0x%x,B 0x%x,C 0x%x,D 0x%x,E 0x%x, F 0x%x\n",
	    transMatrix[0], transMatrix[1], transMatrix[2], transMatrix[3], transMatrix[4], transMatrix[5]);

	return result != 0;
}

/* end of file */
