
#include "lcd.h"
#include "spi.h"
#include "amp_application.h"
#include "delay.h"

static void lcd_config(uint8_t param);
static uint8_t lcd_transmit(uint8_t data);
static void lcd_write(dataType type, uint8_t data);
//static void lcd_strobe(uint8_t);
//static void lcd_high(uint8_t data);
//static void lcd_low(uint8_t data);
static void lcd_delay(volatile uint32_t us);

static uint8_t bright = LCD_BRIGHTNESS_OFF;
extern uint8_t MCU_FREQ_VALUE = 96;

void LCD_Init(void)
{   
  lcd_clrscr();
  lcd_config(DISPLAY_CONFIG_8bit_1L_5x8);
  lcd_config(DISPLAY_CONFIG_4bit_2L_5x8);
  lcd_setmode(VIEW_MODE_DispOn_BlkOff_CrsOff);
  lcd_clrscr();
  lcd_setmode(ENTRY_MODE_INC_NO_SHIFT);
  Delay_ms(200);
  //lcd_brightness(LCD_BRIGHTNESS_ON);
  
  //Hello screen
  lcd_clrscr();
  lcd_goto(LCD_1st_LINE, 1u);
  lcd_puts("  Park Audio  ");
  lcd_brightness(LCD_BRIGHTNESS_ON);
  
  //Test
 // uint8_t i;
 // for(i = 0; i < 200; i++)
 // {
 //   lcd_clrscr();
 //   lcd_goto(LCD_1st_LINE, 1u);
 //   lcd_puts("  Park Audio  ");
 //   lcd_goto(LCD_2nd_LINE, 1u);
 //   lcd_puts(&i);
 //   lcd_brightness(LCD_BRIGHTNESS_ON);
 //   Delay_ms(300);
 // }
}

static uint8_t lcd_transmit(uint8_t data)
{  
  uint8_t tmp = 0;
  
  CS_Enable(IC_LCD);
  tmp = SPI3_Transmit(data);
  CS_Disable(IC_LCD);
     
  return tmp;
}
void LCD_Pre_Init()
{
  lcd_transmit(0x30);
  //lcd_config(DISPLAY_CONFIG_8bit_1L_5x8);
  //lcd_config(DISPLAY_CONFIG_4bit_2L_5x8);
  //lcd_setmode(VIEW_MODE_DispOn_BlkOff_CrsOff);
  //lcd_clrscr();
  //lcd_setmode(ENTRY_MODE_INC_NO_SHIFT);
}

static void lcd_write(dataType type, uint8_t data)
{
  uint8_t tmp;
  
  tmp = (data & 0xF0);
  if(bright == LCD_BRIGHTNESS_OFF) CLR(tmp, LCD_LED1);
  if(bright == LCD_BRIGHTNESS_ON) SET(tmp, LCD_LED1);
  if(type == DAT) SET(tmp, LCD_RS);
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  SET(tmp, LCD_E);
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  CLR(tmp, LCD_E);
  lcd_transmit(tmp);
  
  lcd_delay(AC_UPDATE_TIME);
  
  tmp = (data & 0x0F) << 4;
  if(bright == LCD_BRIGHTNESS_OFF) CLR(tmp, LCD_LED1);
  if(bright == LCD_BRIGHTNESS_ON) SET(tmp, LCD_LED1);
  if(type == DAT) SET(tmp, LCD_RS);
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  SET(tmp, LCD_E);
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  CLR(tmp, LCD_E);
  lcd_transmit(tmp);
}

static void lcd_config(uint8_t param)
{
  uint8_t tmp;
  
  lcd_delay(STARTUP_TIME);
  /*
  lcd_transmit(DISPLAY_CONFIG_8bit_1L_5x8 & 0xF0);
  lcd_strobe(DISPLAY_CONFIG_8bit_1L_5x8 & 0xF0);
  lcd_delay(BUSY_CYCLE_TIME);
  
  lcd_transmit(DISPLAY_CONFIG_8bit_1L_5x8 & 0xF0);
  lcd_strobe(DISPLAY_CONFIG_8bit_1L_5x8 & 0xF0);
  lcd_delay(BUSY_CYCLE_TIME);*/
  
  tmp = (param & 0xF0);
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  SET(tmp, LCD_E);
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  CLR(tmp, LCD_E);
  lcd_transmit(tmp);
  
  lcd_delay(BUSY_CYCLE_TIME);
  
  tmp = (param & 0xF0);
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  SET(tmp, LCD_E);
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  CLR(tmp, LCD_E);
  lcd_transmit(tmp);
  
  lcd_delay(AC_UPDATE_TIME);
  
  tmp = (param & 0x0F) << 4;
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  SET(tmp, LCD_E);
  lcd_transmit(tmp);
  lcd_delay(DATA_SET_TIME);
  CLR(tmp, LCD_E);
  lcd_transmit(tmp);
  lcd_delay(AC_UPDATE_TIME);
}

void lcd_clrscr(void)
{
  lcd_write(CMD, 0x01u);
  lcd_delay(CLRSCR_CYCLE_TIME);
}

void lcd_return(void)
{
  lcd_write(CMD, 0x02u);
  lcd_delay(RETHOME_CYCLE_TIME);
}

void lcd_scroll(uint8_t direction)
{
  switch (direction)
  {
    case LEFT   : lcd_write(CMD, 0x18u); break;
    case RIGHT  : lcd_write(CMD, 0x1Cu); break;
    default: break;
  }
  lcd_delay(AC_UPDATE_TIME);
}

void cursor_shift(uint8_t direction)
{
  switch (direction)
  {
    case LEFT   : lcd_write(CMD, 0x10u); break;
    case RIGHT  : lcd_write(CMD, 0x14u); break;
    default: break;
  }
  lcd_delay(AC_UPDATE_TIME);
}

void lcd_goto(uint8_t line, uint8_t address)
{
  switch (line)
  {
    case LCD_1st_LINE: lcd_write(CMD, 0x80u | START_ADDRESS_1st_LINE | address); break;
    case LCD_2nd_LINE: lcd_write(CMD, 0x80u | START_ADDRESS_2nd_LINE | address); break;
    case CGRAM : lcd_write(CMD, 0x40u | address); break;
    default: break;
  }
  lcd_delay(AC_UPDATE_TIME);
}

void lcd_setmode(uint8_t param)
{
  lcd_write(CMD, param);
  lcd_delay(AC_UPDATE_TIME);
}

void lcd_putc(uint8_t data)
{
  lcd_write(DAT, data);
  lcd_delay(AC_UPDATE_TIME);
}

void lcd_puts(const uint8_t *str)
{
  while ('\0' != *str)
  {
    lcd_putc(*str);		
    str++;
  }
}

void lcd_brightness(uint8_t brightness)
{
  uint8_t tmp;
  
  switch(brightness)
  {
    case LCD_BRIGHTNESS_OFF :   bright = LCD_BRIGHTNESS_OFF; CLR(tmp, LCD_LED1); break;   
    case LCD_BRIGHTNESS_ON :   bright = LCD_BRIGHTNESS_ON; SET(tmp, LCD_LED1); break;
  }
  
  lcd_transmit(tmp);
}

static void lcd_delay(volatile uint32_t us)
{
  us *= MCU_FREQ_VALUE;
  while (us > 0u)
  {
    us--;
  }
}