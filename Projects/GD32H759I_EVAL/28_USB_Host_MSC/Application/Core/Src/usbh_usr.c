/*!
    \file    usbh_usr.c
    \brief   user application layer for USBFS host-mode MSC class operation

    \version 2024-07-31, V2.0.0, demo for GD32H7xx
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "usbh_usr.h"
#include "drv_usb_hw.h"
#include "usbh_msc_core.h"
#include "ff.h"

#include <string.h>

extern usb_core_driver msc_host_core;
extern usbh_host usb_host_msc;

FRESULT res;
FATFS fatfs;
FIL file;

__ALIGN_BEGIN char ReadTextBuff[100] __ALIGN_END;
__ALIGN_BEGIN char WriteTextBuff[] __ALIGN_END = "GD32 USB Host Demo application using FAT_FS   ";
uint16_t bytesWritten, bytesToWrite, bytesRead;

uint8_t line_idx;
uint8_t usbh_usr_application_state = USBH_USR_FS_INIT;

/* points to the usbh_user_cb structure */
usbh_user_cb usr_cb = {
    usbh_user_init,
    usbh_user_deinit,
    usbh_user_device_connected,
    usbh_user_device_reset,
    usbh_user_device_disconnected,
    usbh_user_over_current_detected,
    usbh_user_device_speed_detected,
    usbh_user_device_desc_available,
    usbh_user_device_address_assigned,
    usbh_user_configuration_descavailable,
    usbh_user_manufacturer_string,
    usbh_user_product_string,
    usbh_user_serialnum_string,
    usbh_user_enumeration_finish,
    usbh_user_userinput,
    usbh_usr_msc_application,
    usbh_user_device_not_supported,
    usbh_user_unrecovered_error
};

const uint8_t MSG_HOST_HEADER[] = "USBFS & USBHS MSC Host";
const uint8_t MSG_HOST_FOOTER[] = "USB Host Library v3.0.0";

/* local function prototypes ('static') */
static uint8_t explore_disk(char* path, uint8_t recu_level);
static void toggle_leds(void);

/*!
    \brief      user operation for host-mode initialization
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_init(void)
{
    static uint8_t startup = 0U;

    if(0U == startup) {
        startup = 1U;

        /* configure the LEDs and KEYs*/
#ifdef USE_ULPI_PHY
        gd_eval_led_init(LED1);
        gd_eval_led_init(LED2);
#endif /* USE_ULPI_PHY */

        gd_eval_key_init(KEY_TAMPER, KEY_MODE_GPIO);
        gd_eval_key_init(KEY_WAKEUP, KEY_MODE_GPIO);

#ifdef USE_LCD
        gd_eval_lcd_init();

        lcd_log_init();

        lcd_log_header_set((uint8_t *)MSG_HOST_HEADER, 50U);

        LCD_UsrLog("USB host library started\n");

        lcd_log_footer_set((uint8_t *)MSG_HOST_FOOTER, 40U);
#else
        gd_eval_com_init(EVAL_COM);

        LCD_UsrLog("\r\n++++USB host library started++++\r\n");
#endif
    }
}

/*!
    \brief      deinitialize user state and associated variables
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_deinit(void)
{
    usbh_usr_application_state = USBH_USR_FS_INIT;
}

/*!
    \brief      user operation for device attached
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_connected(void)
{
    LCD_UsrLog("> Device Attached.\r\n");
}

/*!
    \brief      user operation when unrecovered error happens
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_unrecovered_error(void)
{
    LCD_ErrLog("> Unrecovered error state .\r\n");
}

/*!
    \brief      user operation for device disconnect event
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_disconnected(void)
{
    LCD_UsrLog("> Device Disconnected.\r\n");
}

/*!
    \brief      user operation for reset USB Device
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_reset(void)
{
    LCD_UsrLog("> Reset the USB device.\r\n");
}

/*!
    \brief      user operation for detecting device speed
    \param[in]  device_speed: device speed
    \param[out] none
    \retval     none
*/
void usbh_user_device_speed_detected(uint32_t device_speed)
{
    if(PORT_SPEED_HIGH == device_speed) {
        LCD_UsrLog("> High speed device detected.\r\n");
    } else if(PORT_SPEED_FULL == device_speed) {
        LCD_UsrLog("> Full speed device detected.\r\n");
    } else if(PORT_SPEED_LOW == device_speed) {
        LCD_UsrLog("> Low speed device detected.\r\n");
    } else {
        LCD_ErrLog("> Device Fault.\r\n");
    }
}

/*!
    \brief      user operation when device descriptor is available
    \param[in]  device_desc: device descriptor
    \param[out] none
    \retval     none
*/
void usbh_user_device_desc_available(void *device_desc)
{
    usb_desc_dev *dev_str = device_desc;

    LCD_DevInformation("VID: %04Xh\r\n", (uint32_t)dev_str->idVendor);
    LCD_DevInformation("PID: %04Xh\r\n", (uint32_t)dev_str->idProduct);
}

/*!
    \brief      USB device is successfully assigned the address
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_address_assigned(void)
{
}

/*!
    \brief      user operation when configuration descriptor is available
    \param[in]  cfg_desc: pointer to configuration descriptor
    \param[in]  itf_desc: pointer to interface descriptor
    \param[in]  ep_desc: pointer to endpoint descriptor
    \param[out] none
    \retval     none
*/
void usbh_user_configuration_descavailable(usb_desc_config *cfg_desc, \
                                           usb_desc_itf *itf_desc, \
                                           usb_desc_ep *ep_desc)
{
    usb_desc_itf *id = itf_desc;

    if(0x08U == (*id).bInterfaceClass) {
        LCD_UsrLog("> Mass storage device connected.\r\n");
    } else if(0x03U == (*id).bInterfaceClass) {
        LCD_UsrLog("> HID device connected.\r\n");
    } else {
        LCD_UsrLog("> Unknown device connected.\r\n");
    }
}

/*!
    \brief      user operation when manufacturer string exists
    \param[in]  manufacturer_string: manufacturer string of usb device
    \param[out] none
    \retval     none
*/
void usbh_user_manufacturer_string(void *manufacturer_string)
{
    LCD_DevInformation("Manufacturer: %s\r\n", (char *)manufacturer_string);
}

/*!
    \brief      user operation when product string exists
    \param[in]  product_string: product string of usb device
    \param[out] none
    \retval     none
*/
void usbh_user_product_string(void *product_string)
{
    LCD_DevInformation("Product: %s\r\n", (char *)product_string);
}

/*!
    \brief      user operation when serial string exists
    \param[in]  serial_num_string: serialNum string of usb device
    \param[out] none
    \retval     none
*/
void usbh_user_serialnum_string(void *serial_num_string)
{
    LCD_DevInformation("Serial Number: %s\r\n", (char *)serial_num_string);
}

/*!
    \brief      user response request is displayed to ask for application jump to class
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_enumeration_finish(void)
{
    LCD_UsrLog("> Enumeration completed.\r\n");

#ifdef USE_LCD
    lcd_text_color_set(LCD_COLOR_RED);
    lcd_vertical_string_display(LCD_HINT_LINE0, 0, (uint8_t *)"---------------------------------------");
    lcd_text_color_set(LCD_COLOR_GREEN);
    lcd_vertical_string_display(LCD_HINT_LINE1, 0, (uint8_t *)"To see the disk information:  ");
    lcd_vertical_string_display(LCD_HINT_LINE2, 0, (uint8_t *)"Press Tamper Key...             ");
#else
    LCD_UsrLog(">To see the disk information: \r\n");
    LCD_UsrLog(">Press Tamper Key...\r\n");
#endif
}

/*!
    \brief      user operation when device is not supported
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_not_supported(void)
{
    LCD_ErrLog("> Device not supported.\r\n");
}

/*!
    \brief      user action for application state entry
    \param[in]  none
    \param[out] none
    \retval     user response for user key
*/
usbh_user_status usbh_user_userinput(void)
{
    usbh_user_status usbh_usr_status = USR_IN_NO_RESP;

#if USB_LOW_POWER
    gd_eval_key_init(KEY_WAKEUP, KEY_MODE_EXTI);

    if(usb_host_msc.suspend_flag) {
        LCD_UsrLog("> Host in suspend status.\n");
        LCD_UsrLog("> Pls press Wakeup key (General wakeup).\n");

        if(usb_host_msc.dev_supp_remote_wkup) {
            LCD_UsrLog("> Or operate device (Remote wakeup).\n");
        }
    } else {
        if(GENERAL_WAKEUP == usb_host_msc.wakeup_mode) {
            usb_host_msc.wakeup_mode = NORMAL_WORK;
            LCD_UsrLog("> General wakeup success.\n");
        } else if(REMOTE_WAKEUP == usb_host_msc.wakeup_mode) {
            usb_host_msc.wakeup_mode = NORMAL_WORK;
            LCD_UsrLog("> Remote wakeup success.\n");
        } else {
        }
    }
#endif /* USB_LOW_POWER */

    /*key User is in polling mode to detect user action */
    if(RESET == gd_eval_key_state_get(KEY_TAMPER)) {
        usbh_usr_status = USR_IN_RESP_OK;
    }

    return usbh_usr_status;
}

/*!
    \brief      user operation for device over current detection event
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_over_current_detected(void)
{
    LCD_ErrLog("> Overcurrent detected.\r\n");
}

/*!
    \brief      demo application for mass storage
    \param[in]  none
    \param[out] none
    \retval     status
*/
int usbh_usr_msc_application(void)
{
    msc_lun info;

    switch(usbh_usr_application_state) {
    case USBH_USR_FS_INIT:
        /* initializes the file system*/
        if(FR_OK != f_mount(&fatfs, "0:/", 0)) {
            LCD_ErrLog("> Cannot initialize File System.\r\n");

            return(-1);
        }

        LCD_UsrLog("> File System initialized.\r\n");

        if(USBH_OK == usbh_msc_lun_info_get(&usb_host_msc, 0, &info)) {
            LCD_UsrLog("> Disk capacity: %llu Bytes.\r\n", (uint64_t)info.capacity.block_nbr * info.capacity.block_size);
        }

        usbh_usr_application_state = USBH_USR_FS_READLIST;
        break;

    case USBH_USR_FS_READLIST:
        LCD_UsrLog("> Exploring disk flash ...\r\n");

#ifdef USE_LCD
        lcd_text_color_set(LCD_COLOR_GREEN);
        lcd_vertical_string_display(LCD_HINT_LINE1, 0, (uint8_t *)"To see the root content of disk");
        lcd_vertical_string_display(LCD_HINT_LINE2, 0, (uint8_t *)"Press Tamper Key...            ");
#else
        LCD_UsrLog(">>> To see the root content of disk \r\n");
        LCD_UsrLog(">>> Press Tamper Key...\r\n");
#endif

        /*Key TAMPER in polling*/
        while((msc_host_core.host.connect_status) && \
            (SET == gd_eval_key_state_get(KEY_TAMPER))) {
            toggle_leds();
        }

        explore_disk("0:/", 1);
        line_idx = 0;
        usbh_usr_application_state = USBH_USR_FS_WRITEFILE;
        break;

    case USBH_USR_FS_WRITEFILE:
        usb_mdelay(100);

#ifdef USE_LCD
        lcd_text_color_set(LCD_COLOR_GREEN);
        lcd_vertical_string_display(LCD_HINT_LINE1, 0, (uint8_t *)"                                  ");
        lcd_vertical_string_display(LCD_HINT_LINE2, 0, (uint8_t *)"Press Wakeup Key to write file");
#else
        LCD_UsrLog(">>> Press Wakeup Key to write file\r\n");
#endif

        /*key Wakeup in polling*/
        while ((msc_host_core.host.connect_status) && \
                (RESET == gd_eval_key_state_get(KEY_WAKEUP))) {
            toggle_leds();
        }

        LCD_UsrLog("> Writing File to disk flash ...\r\n");

        /* register work area for logical drives */
        f_mount(&fatfs, "0:/", 1);

        if(FR_OK == f_open(&file, "0:GD32.TXT", FA_CREATE_ALWAYS | FA_WRITE)) {
            LCD_UsrLog("> GD32.TXT be opened for write.\n");
            /* write buffer to file */
            bytesToWrite = strlen(WriteTextBuff);
            res = f_write(&file, WriteTextBuff, bytesToWrite, (void *)&bytesWritten);
            f_sync(&file);

            /* EOF or error */
            if((0U == bytesWritten) || (FR_OK != res)) {
                LCD_ErrLog("> GD32.TXT CANNOT be written.\r\n");
            } else {
                if(FR_OK == f_open(&file, "0:GD32.TXT", FA_READ)) {
                    res = f_read(&file, ReadTextBuff, sizeof(ReadTextBuff), (void *)&bytesRead);
                    /* EOF or error */
                    if((bytesRead == 0) || (res != FR_OK)) {
                        LCD_ErrLog("> GD32.TXT CANNOT be read.\r\n");
                    } else {
                        /* compare file content */
                       if((bytesRead == bytesWritten) && (0 == strncmp(ReadTextBuff, WriteTextBuff, bytesRead))) {
                            LCD_UsrLog("> File content compare: SUCCESS.\r\n");
                        } else {
                            LCD_ErrLog("> File content compare: ERROR.\r\n");
                        }
                    }
                } else {
                    LCD_ErrLog("> GD32.TXT CANNOT be opened for read.\r\n");
                }
            }

            /* close file */
            f_close(&file);
        } else {
            LCD_ErrLog("> GD32.TXT CANNOT be opened.\r\n");
        }

        /* unmount file system */
        f_mount(NULL, "0:/", 1);

        usbh_usr_application_state = USBH_USR_FS_DEMOEND;
        LCD_UsrLog("> The MSC host demo is end.\r\n");
        break;

    case USBH_USR_FS_DEMOEND:
        break;

    default:
        break;
    }

    return(0);
}

/*!
    \brief      displays disk content
    \param[in]  path: pointer to root path
    \param[in]  recu_level: recursive level
    \param[out] none
    \retval     status
*/
static uint8_t explore_disk(char* path, uint8_t recu_level)
{
    FILINFO fno;
    DIR dir;
    char *fn;

    res = f_opendir(&dir, path);

    if(res == FR_OK) {
        while((msc_host_core.host.connect_status)) {
            res = f_readdir(&dir, &fno);
            if(FR_OK != res || 0U == fno.fname[0]) {
                break;
            }

            if('.' == fno.fname[0]) {
                continue;
            }

            fn = fno.fname;

            line_idx++;

            if(line_idx > 4) {
                line_idx = 0;

#ifdef USE_LCD
                lcd_text_color_set(LCD_COLOR_GREEN);
                lcd_vertical_string_display(LCD_HINT_LINE1, 0, (uint8_t *)"                                ");
                lcd_vertical_string_display(LCD_HINT_LINE2, 0, (uint8_t *)"Press Tamper Key to continue");
#else
                LCD_UsrLog(">>> Press Tamper Key to continue\r\n");
#endif

                /*key User in polling*/
                while((msc_host_core.host.connect_status) && \
                    (SET == gd_eval_key_state_get(KEY_TAMPER))) {
                    toggle_leds();
                }
            }

            if(1U == recu_level) {
                LCD_UsrLog("   |__");
            } else if(2U == recu_level) {
                LCD_UsrLog("   |   |__");
            }

            if(AM_DIR == fno.fattrib) {
                LCD_UsrLog("%s\r\n", fno.fname);
            } else {
                LCD_UsrLog("%s\r\n", fno.fname);
            }

            if((AM_DIR == fno.fattrib) && (1U == recu_level)) {
                explore_disk(fn, 2);
            }
        }
    }

    return res;
}

/*!
    \brief      toggle leds to shows user input state
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void toggle_leds(void)
{
#ifdef USE_ULPI_PHY
    static uint32_t i;

    if(0x10000U == i++) {
        gd_eval_led_toggle(LED1);
        gd_eval_led_toggle(LED2);
        i = 0;
    }
#endif /* USE_ULPI_PHY */
}

#ifndef USE_LCD

#ifdef __GNUC__
/* retarget the C library printf function to the USART, in Eclipse GCC environment */
int __io_putchar(int ch)
{
    usart_data_transmit(EVAL_COM, (uint8_t) ch );
    while(RESET == usart_flag_get(EVAL_COM, USART_FLAG_TC));
    return ch;
}
#else
/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(EVAL_COM, (uint8_t)ch);
    while(RESET == usart_flag_get(EVAL_COM, USART_FLAG_TC));

    return ch;
}
#endif /* __GNUC__ */

#endif