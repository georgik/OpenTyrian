// Copyright 2016-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "SDL.h"
#include "esp_err.h"
#include "esp_check.h"


#if 1
// Wrover kit
// #define PIN_NUM_MISO -1
// #define PIN_NUM_MOSI 23
// #define PIN_NUM_CLK  19
// #define PIN_NUM_CS   22
// #define PIN_NUM_DC   21
// #define PIN_NUM_RST  18
// #define PIN_NUM_BCKL 5
#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI 6
#define PIN_NUM_CLK  7
#define PIN_NUM_CS   5
#define PIN_NUM_DC   4
#define PIN_NUM_RST  48
#define PIN_NUM_BCKL 47
#else
#define PIN_NUM_MOSI CONFIG_HW_LCD_MOSI_GPIO
#define PIN_NUM_MISO CONFIG_HW_LCD_MISO_GPIO
#define PIN_NUM_CLK  CONFIG_HW_LCD_CLK_GPIO
#define PIN_NUM_CS   CONFIG_HW_LCD_CS_GPIO
#define PIN_NUM_DC   CONFIG_HW_LCD_DC_GPIO
#define PIN_NUM_RST  CONFIG_HW_LCD_RESET_GPIO
#define PIN_NUM_BCKL CONFIG_HW_LCD_BL_GPIO
#endif

//You want this, especially at higher framerates. The 2nd buffer is allocated in iram anyway, so isn't really in the way.
#define DOUBLE_BUFFER
const int DUTY_MAX = 0x1fff;
bool isBackLightIntialized = false;
const int LCD_BACKLIGHT_ON_VALUE = 1;

/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} ili_init_cmd_t;

#if (CONFIG_HW_LCD_TYPE == 0)
#define TFT_CMD_SWRESET	0x01
#define TFT_CMD_SLEEP 0x10
#define TFT_CMD_DISPLAY_OFF 0x28

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_MH 0x04
#define TFT_RGB_BGR 0x08
#endif

static spi_device_handle_t spi;
short screen_boarder = 0;

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void ili_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}


void backlight_percentage_set(int value)
{
    // int duty = DUTY_MAX * (value * 0.01f);
    // ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 500);
    // ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}

#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp/touch.h"
//#include "esp_lcd_touch_ft5x06.h"
#include "esp_lcd_touch_tt21100.h"

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;
#define BSP_AXP2101_ADDR    0x34
#define BSP_AW9523_ADDR     0x58

static esp_err_t bsp_display_brightness_init(void)
{
    /* Initilize I2C */
    bsp_i2c_init();

    const uint8_t lcd_bl_en[] = { 0x90, 0xBF }; // AXP DLDO1 Enable
    i2c_master_write_to_device(BSP_I2C_NUM, BSP_AXP2101_ADDR, lcd_bl_en, sizeof(lcd_bl_en), 1000 / portTICK_PERIOD_MS);
    const uint8_t lcd_bl_val[] = { 0x99, 0b00011000 };  // AXP DLDO1 voltage
    i2c_master_write_to_device(BSP_I2C_NUM, BSP_AXP2101_ADDR, lcd_bl_val, sizeof(lcd_bl_val), 1000 / portTICK_PERIOD_MS);

    return ESP_OK;
}
// esp_err_t bsp_display_brightness_set(int brightness_percent)
// {
//     if (brightness_percent > 100) {
//         brightness_percent = 100;
//     }
//     if (brightness_percent < 0) {
//         brightness_percent = 0;
//     }

//     printf("Setting LCD backlight: %d%%", brightness_percent);
//     const uint8_t reg_val = 20 + ((8 * brightness_percent) / 100); // 0b00000 ~ 0b11100; under 20, it is too dark
//     const uint8_t lcd_bl_val[] = { 0x99, reg_val }; // AXP DLDO1 voltage
//     i2c_master_write_to_device(BSP_I2C_NUM, BSP_AXP2101_ADDR, lcd_bl_val, sizeof(lcd_bl_val), 1000 / portTICK_PERIOD_MS);

//     return ESP_OK;
// }

#define CONFIG_BSP_LCD_DRAW_BUF_HEIGHT 240
//Initialize the display
void ili_init()
{
    // Initialize ESP-BSP display
    bsp_display_config_t cfg = {
        // .buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        // .flags = {
        //     .buff_dma = true,
        //     .buff_spiram = false,
        // }
    };
    
    const bsp_display_config_t bsp_disp_cfg = {
        .max_transfer_sz = (BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT) * sizeof(uint16_t),
    };

    ESP_ERROR_CHECK(bsp_display_new(&bsp_disp_cfg, &panel_handle, &io_handle));
    printf("panel_handle: %p\n", panel_handle);

    // Initializa the touch
    bsp_touch_new(NULL, &touch_handle);
    printf("touch_handle: %p\n", touch_handle);

    bsp_display_brightness_init();
    
    // bsp_display_start();
    // bsp_display_backlight_on();
    // int cmd=0;
    // //Initialize non-SPI GPIOs
    // gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    // gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    // if(PIN_NUM_BCKL != -1)
    //     gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    // backlight_init();
    // //Reset the display
    // gpio_set_level(PIN_NUM_RST, 0);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    // gpio_set_level(PIN_NUM_RST, 1);
    // vTaskDelay(120);

    // //Send all the commands
    // while (ili_init_cmds[cmd].databytes!=0xff) {
    //     uint8_t dmdata[16];
    //     ili_cmd(spi, ili_init_cmds[cmd].cmd);
    //     //Need to copy from flash to DMA'able memory
    //     memcpy(dmdata, ili_init_cmds[cmd].data, 16);
    //     ili_data(spi, dmdata, ili_init_cmds[cmd].databytes&0x1F);
    //     if (ili_init_cmds[cmd].databytes&0x80) {
    //         vTaskDelay(140);
    //         printf("ili_init_cmds: delay\n");
    //     }
    //     cmd++;
    // }

    // ///Enable backlight
    // if(PIN_NUM_BCKL != -1)
    //     gpio_set_level(PIN_NUM_BCKL, 1);


    vTaskDelay(140);
}

static void IRAM_ATTR send_header_start(spi_device_handle_t spi, int xpos, int ypos, int w, int h)
{
}


void IRAM_ATTR send_header_cleanup(spi_device_handle_t spi)
{

}

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#ifndef DOUBLE_BUFFER
volatile static uint16_t *currFbPtr=NULL;
#else
//Warning: This gets squeezed into IRAM.
static uint32_t *currFbPtr=NULL;
#endif
SemaphoreHandle_t dispSem = NULL;
SemaphoreHandle_t dispDoneSem = NULL;

#define NO_SIM_TRANS 5 //Amount of SPI transfers to queue in parallel
#define MEM_PER_TRANS 320*2 //in 16-bit words

int16_t lcdpal[256];

void fillBufferWithRed(uint16_t *buffer, int width, int height) {
    uint16_t redColor = 0xF800;  // Full red in RGB565
    for (int i = 0; i < 50*50; i++) {
        buffer[i] = redColor;
    }
}

uint16_t *rgb565_buffer = NULL;

void IRAM_ATTR displayTask(void *arg) {
// 	int x, i;
// 	int idx=0;
// 	int inProgress=0;
// 	static uint16_t *dmamem[NO_SIM_TRANS];
// 	spi_transaction_t trans[NO_SIM_TRANS];
// 	spi_transaction_t *rtrans;

//     esp_err_t ret;
//     spi_bus_config_t buscfg;
//     memset(&buscfg, 0, sizeof(buscfg));
//     buscfg.miso_io_num=PIN_NUM_MISO;
//     buscfg.mosi_io_num=PIN_NUM_MOSI;
//     buscfg.sclk_io_num=PIN_NUM_CLK;
//     buscfg.quadwp_io_num=-1;
//     buscfg.quadhd_io_num=-1;
// //        .max_transfer_sz=(MEM_PER_TRANS*2)+16

//     spi_device_interface_config_t devcfg;
//     memset(&devcfg, 0, sizeof(devcfg));
//     devcfg.clock_speed_hz=40000000;               //Clock out at 26 MHz. Yes, that's heavily overclocked.
//     devcfg.mode=0;                                //SPI mode 0
//     devcfg.spics_io_num=PIN_NUM_CS;               //CS pin
//     devcfg.queue_size=NO_SIM_TRANS;               //We want to be able to queue this many transfers
//     devcfg.pre_cb=ili_spi_pre_transfer_callback;  //Specify pre-transfer callback to handle D/C line
//     devcfg.flags = SPI_DEVICE_NO_DUMMY;


	printf("*** Display task starting.\n");

    //heap_caps_print_heap_info(MALLOC_CAP_DMA);

    // SDL_LockDisplay();
    // Initialize the SPI bus
    // ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    // assert(ret == ESP_OK);

    // Add a device to the SPI bus
    // ret = spi_bus_add_device(SPI3_HOST, &devcfg, &spi);
    // assert(ret == ESP_OK);
    //Initialize the LCD
    bsp_i2c_init();


    ili_init();
    printf("Turn on backlight\n");
    bsp_display_brightness_init();
    bsp_display_backlight_on();
    bsp_display_brightness_set(80);
    isBackLightIntialized = true;

    esp_lcd_panel_disp_on_off(panel_handle, true);

    // SDL_UnlockDisplay();

	// We're going to do a fair few transfers in parallel. Set them all up.
	// for (x=0; x<NO_SIM_TRANS; x++) {
	// 	//dmamem[x]=pvPortMallocCaps(MEM_PER_TRANS*2, MALLOC_CAP_DMA);
    //     dmamem[x]=heap_caps_malloc(MEM_PER_TRANS*2, MALLOC_CAP_DMA);
	// 	assert(dmamem[x]);
	// 	memset(&trans[x], 0, sizeof(spi_transaction_t));
	// 	trans[x].length=MEM_PER_TRANS*2;
	// 	trans[x].user=(void*)1;
	// 	trans[x].tx_buffer=&dmamem[x];
	// }
	//xSemaphoreGive(dispDoneSem);

    int screen_x = 0;
    int screen_y = screen_boarder;
    int screen_width = 320;
    int screen_height = (240-screen_boarder*2);

    // uint32_t *rgb565_buffer = heap_caps_malloc(screen_width * screen_height * sizeof(uint16_t), MALLOC_CAP_32BIT);
    // if (rgb565_buffer == NULL) {
    //     printf("Failed to allocate rgb565 buffer\n");
    //     return;
    // }
    printf("Entering display loop.\n");
	while(1) {
		xSemaphoreTake(dispSem, portMAX_DELAY);


    // Drawing bitmap
    // printf("Drawing bitmap %d %d %d %d\n", screen_x, screen_y, screen_width, screen_height);
    // fillBufferWithRed(currFbPtr, screen_width, screen_height);

    // Upsample 8-bit graphics from currFbPtr to 16 bit RGB565 to rgb565_buffer and send to screen using esp_lcd_panel_draw_bitmap
    // for (int y=0; y<screen_height; y++) {
    //     for (int x=0; x<screen_width; x++) {
    //         uint32_t d=currFbPtr[x+y*screen_width];
    //         rgb565_buffer[x] = lcdpal[(d>>0)&0xff];
    //     }
    //     ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, screen_x, screen_y+y, screen_width, screen_y+y+1, rgb565_buffer));
    // }


    for (uint32_t y=0; y<screen_height; y++) {

			for (uint16_t i=0; i<320; i+=4) {
				uint32_t d=currFbPtr[(320*y+i)/4];
				rgb565_buffer[i+0]=lcdpal[(d>>0)&0xff];
				rgb565_buffer[i+1]=lcdpal[(d>>8)&0xff];
				rgb565_buffer[i+2]=lcdpal[(d>>16)&0xff];
				rgb565_buffer[i+3]=lcdpal[(d>>24)&0xff];
			}
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, y, screen_width, y+1, rgb565_buffer));
    }

    // uint32_t index = 0;
    // for (uint32_t y=0; y<10; y++) {
    //     for (uint32_t x=0; x<320; x++) {
    //         uint32_t d=currFbPtr[x+index];
    //         // rgb565_buffer[x] = lcdpal[(d>>0)&0xff];
    //         // rgb565_buffer[x+1] = lcdpal[(d>>8)&0xff];
    //     }
    //     index += 320;
    //     ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, y, screen_width, y+1, rgb565_buffer));
    // }
//         SDL_LockDisplay();
// 		send_header_start(spi, 0, screen_boarder, 320, 240-screen_boarder*2);
// 		send_header_cleanup(spi);

// 		for (x=0; x<320*(240-screen_boarder*2); x+=MEM_PER_TRANS) {
// #ifdef DOUBLE_BUFFER
// 			for (i=0; i<MEM_PER_TRANS; i+=4) {
// 				uint32_t d=currFbPtr[(x+i)/4];
// 				dmamem[idx][i+0]=lcdpal[(d>>0)&0xff];
// 				dmamem[idx][i+1]=lcdpal[(d>>8)&0xff];
// 				dmamem[idx][i+2]=lcdpal[(d>>16)&0xff];
// 				dmamem[idx][i+3]=lcdpal[(d>>24)&0xff];
// 			}
// #else
// 			for (i=0; i<MEM_PER_TRANS; i++) {
// 				dmamem[idx][i]=lcdpal[myData[i]];
// 			}
// 			myData+=MEM_PER_TRANS;
// #endif
// 			trans[idx].length=MEM_PER_TRANS*16;
// 			trans[idx].user=(void*)1;
// 			trans[idx].tx_buffer=dmamem[idx];

// 			ret=spi_device_queue_trans(spi, &trans[idx], portMAX_DELAY);
// 			assert(ret==ESP_OK);

// 			idx++;
// 			if (idx>=NO_SIM_TRANS) idx=0;

// 			if (inProgress==NO_SIM_TRANS-1) {
// 				ret=spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
// 				assert(ret==ESP_OK);
// 			} else {
// 				inProgress++;
// 			}
// 		}
// #ifndef DOUBLE_BUFFER
// 		//xSemaphoreGive(dispDoneSem);
// #endif
// 		while(inProgress) {
// 			ret=spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
// 			assert(ret==ESP_OK);
// 			inProgress--;
// 		}
//         SDL_UnlockDisplay();
	}
}

#include    <xtensa/config/core.h>
#include    <xtensa/corebits.h>
#include    <xtensa/config/system.h>
//#include    <xtensa/simcall.h>

void spi_lcd_wait_finish() {
#ifndef DOUBLE_BUFFER
	//xSemaphoreTake(dispDoneSem, portMAX_DELAY);
#endif
}

void spi_lcd_send(uint16_t *scr) {
#ifdef DOUBLE_BUFFER
	memcpy(currFbPtr, scr, 320*240);
	//Theoretically, also should double-buffer the lcdpal array... ahwell.
#else
	currFbPtr=scr;
#endif
	xSemaphoreGive(dispSem);
}

void IRAM_ATTR spi_lcd_send_boarder(uint16_t *scr, int boarder) {
#ifdef DOUBLE_BUFFER
	//memcpy(currFbPtr+(boarder*320/4), scr, 320*(240-boarder*2));
    screen_boarder = boarder;
	memcpy(currFbPtr, scr, 320*(240-boarder*2));
#else
	currFbPtr=scr;
#endif
	xSemaphoreGive(dispSem);
}

void spi_lcd_clear() {
#ifdef DOUBLE_BUFFER
	memset(currFbPtr,0,(320*240/sizeof(currFbPtr)));
#endif
	xSemaphoreGive(dispSem);
}

void spi_lcd_init() {
	printf("spi_lcd_init()\n");
    dispSem=xSemaphoreCreateBinary();
    //dispDoneSem=xSemaphoreCreateBinary();
#ifdef DOUBLE_BUFFER
    screen_boarder = 0;
    currFbPtr=heap_caps_malloc(320*240, MALLOC_CAP_32BIT);
    if (currFbPtr==NULL) {
        printf("Failed to allocate framebuffer\n");
        return;
    }
    rgb565_buffer = heap_caps_malloc(320 * sizeof(uint16_t), MALLOC_CAP_32BIT);
    assert(rgb565_buffer != NULL); // Ensure DMA buffer allocation succeeded

    memset(currFbPtr,0,(320*240));
#endif

	xTaskCreatePinnedToCore(&displayTask, "display", 10000, NULL, 6, NULL, 1);
}
