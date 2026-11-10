/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdint.h>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* ================= CAU HINH MAC DINH (sua duoc ngay tren LCD) ================= */

/* Muc kich relay: GPIO_PIN_SET = relay dong khi PA0 = 1 (jumper High).
 * Neu relay chay nguoc thi doi thanh GPIO_PIN_RESET */
#define RELAY_ON_LEVEL      GPIO_PIN_SET

/* An toan cho che do tu dong: bom lien tuc toi da bao lau, roi nghi bao lau
 * (de nuoc kip tham xuong dat, tranh ngap uong neu cam bien sai) */
#define AUTO_MAX_RUN_MS     60000UL     /* toi da 60 giay moi dot bom  */
#define AUTO_COOL_MS        300000UL    /* nghi 5 phut sau khi bi cat  */

/* Gia tri mac dinh lan dau (sau do luu vao flash) */
#define DEF_AUTO_EN         1
#define DEF_SOIL_START      30          /* dat < 30%  -> bat bom   */
#define DEF_SOIL_STOP       60          /* dat >= 60% -> tat bom   */
#define DEF_SCHED_EN        1
#define DEF_SCHED_H         7           /* hen gio 07:00           */
#define DEF_SCHED_M         0
#define DEF_PUMP_SEC        10          /* bom 10 giay             */
#define DEF_ADC_DRY         4000        /* cam bien dien tro ngoai khong khi ~4095 (se hieu chinh tren LCD) */
#define DEF_ADC_WET         1500        /* nhung nuoc/dat uot: thap hon nhieu (se hieu chinh tren LCD)       */

/* ---- Cam bien dat DIEN TRO (2 chan kim loai + module LM393) ----
 * Day AO -> PA2 (A2). Day VCC -> PB0 (khuyen dung) HOAC 3V3. GND -> GND. Day DO de trong.
 * Cap nguon qua PB0 de chi bat luc do: chan kim loai cam lien tuc se bi an mon (ri set) rat nhanh. */
#define SOIL_PWR_PORT       GPIOB
#define SOIL_PWR_PIN        GPIO_PIN_0
#define SOIL_SETTLE_MS      15          /* cho module on dinh sau khi cap nguon */
#define SOIL_READ_PERIOD_MS 500         /* doc cam bien moi 0.5 giay            */

/* Dia chi I2C */
#define RTC_ADDR            (0x68 << 1)
#define LCD_ADDR_1          (0x27 << 1)
#define LCD_ADDR_2          (0x3F << 1)

/* Bit dieu khien PCF8574 cua LCD */
#define LCD_BL              0x08
#define LCD_EN              0x04
#define LCD_RS              0x01

/* Luu cai dat vao trang flash cuoi (Blue Pill 64KB) */
#define CFG_ADDR            0x0800FC00UL
#define CFG_MAGIC           0xA5C30003UL

/* Chong treo LCD: moi 30 giay khoi dong lai bus I2C + LCD */
#define LCD_REINIT_MS       30000UL

/* Nut nhan (noi 1 chan nut xuong GND, chan con lai vao chan MCU) */
#define BTN_MENU_PIN        GPIO_PIN_12   /* PB12: chuyen trang menu        */
#define BTN_UP_PIN          GPIO_PIN_13   /* PB13: tang / bat               */
#define BTN_DOWN_PIN        GPIO_PIN_14   /* PB14: giam / tat               */
#define BTN_PUMP_PIN        GPIO_PIN_15   /* PB15: bom tay (10 giay)        */
#define BTN_MENU            0
#define BTN_UP              1
#define BTN_DOWN            2
#define BTN_PUMP            3

/* Cac trang menu */
#define P_HOME              0
#define P_AUTO              1
#define P_START             2
#define P_STOP              3
#define P_SCHED_EN          4
#define P_SCHED_H           5
#define P_SCHED_M           6
#define P_PUMP_S            7
#define P_CAL_DRY           8
#define P_CAL_WET           9
#define P_COUNT             10


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

typedef struct {
  uint8_t sec, min, hour, day, month, year;
} RtcTime_t;

typedef struct {
  uint32_t magic;
  uint8_t  auto_en, start, stop, sched_en;
  uint8_t  sched_h, sched_m, pump_s, rsv;
  uint16_t dry, wet;
  uint16_t rsv2, rsv3;
} Config_t;

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  uint8_t  raw, state;
  uint32_t t_change, t_next;
} Btn_t;

static Config_t  cfg;
static uint8_t   cfg_dirty = 0;
static uint16_t  lcd_addr = LCD_ADDR_1;
static uint8_t   lcd_err = 0;

static uint8_t   page = P_HOME;
static uint32_t  msg_until = 0;

/* Du lieu do duoc */
static uint8_t   g_temp = 0, g_hum = 0, g_dht_ok = 0;
static uint16_t  g_raw = 0;
static uint8_t   g_soil = 0;
static RtcTime_t g_now;
static uint8_t   g_rtc_ok = 0;

/* Bom */
static uint8_t   relay_state = 0;
static uint8_t   pump_timed = 0;
static uint32_t  pump_timed_end = 0;
static uint8_t   auto_run = 0;
static uint32_t  auto_run_start = 0;
static uint32_t  auto_cool_until = 0;
static uint32_t  sched_last_key = 0xFFFFFFFFUL;

static Btn_t btns[4] = {
  { GPIOB, BTN_MENU_PIN, 0, 0, 0, 0 },
  { GPIOB, BTN_UP_PIN,   0, 0, 0, 0 },
  { GPIOB, BTN_DOWN_PIN, 0, 0, 0, 0 },
  { GPIOB, BTN_PUMP_PIN, 0, 0, 0, 0 },
};


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ================= DELAY micro giay bang TIM2 (1 MHz) ================= */
static void delay_us(uint16_t us)
{
  __HAL_TIM_SET_COUNTER(&htim2, 0);
  while (__HAL_TIM_GET_COUNTER(&htim2) < us) { }
}

/* ============ PHUC HOI BUS I2C (khi bi nhieu lam treo SDA/SCL) ============ */
static void i2c_recover(void)
{
  GPIO_InitTypeDef g = {0};

  HAL_I2C_DeInit(&hi2c1);
  __HAL_RCC_I2C1_FORCE_RESET();
  __HAL_RCC_I2C1_RELEASE_RESET();

  /* PB6 = SCL, PB7 = SDA: tam dieu khien bang tay */
  g.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
  g.Mode  = GPIO_MODE_OUTPUT_OD;
  g.Pull  = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &g);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
  delay_us(10);

  /* Phat 9 xung SCL de thiet bi dang giu SDA nha bus ra */
  for (uint8_t i = 0; i < 9; i++)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); delay_us(10);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   delay_us(10);
  }
  /* Tao dieu kien STOP */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); delay_us(10);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   delay_us(10);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);   delay_us(10);

  MX_I2C1_Init();    /* cai lai I2C1 va tra chan ve che do I2C */
}

/* ======================= LCD 1602 I2C (PCF8574) ======================= */
static void lcd_write4(uint8_t nibble_with_flags)
{
  uint8_t b[2];
  b[0] = nibble_with_flags | LCD_EN;
  b[1] = nibble_with_flags;
  if (HAL_I2C_Master_Transmit(&hi2c1, lcd_addr, b, 2, 50) != HAL_OK) lcd_err = 1;
}

static void lcd_send(uint8_t data, uint8_t rs)
{
  uint8_t hi = data & 0xF0;
  uint8_t lo = (uint8_t)((data << 4) & 0xF0);
  uint8_t b[4];
  b[0] = hi | rs | LCD_BL | LCD_EN;
  b[1] = hi | rs | LCD_BL;
  b[2] = lo | rs | LCD_BL | LCD_EN;
  b[3] = lo | rs | LCD_BL;
  if (HAL_I2C_Master_Transmit(&hi2c1, lcd_addr, b, 4, 50) != HAL_OK) lcd_err = 1;
}

static void lcd_cmd(uint8_t cmd)
{
  lcd_send(cmd, 0);
  if (cmd == 0x01 || cmd == 0x02) HAL_Delay(2);
}

static void lcd_data(uint8_t d) { lcd_send(d, LCD_RS); }

static void lcd_set_cursor(uint8_t row, uint8_t col)
{
  lcd_cmd(0x80 | ((row ? 0x40 : 0x00) + col));
}

static void lcd_print(const char *s)
{
  while (*s) lcd_data((uint8_t)*s++);
}

/* In 1 dong 16 ky tu (tu dong chen khoang trang cho het dong) */
static void lcd_line(uint8_t row, const char *s)
{
  char b[20];
  snprintf(b, 17, "%-16s", s);
  lcd_set_cursor(row, 0);
  lcd_print(b);
}

/* Tra ve 1 neu tim thay LCD */
static uint8_t lcd_init(void)
{
  if (HAL_I2C_IsDeviceReady(&hi2c1, LCD_ADDR_1, 2, 50) == HAL_OK)      lcd_addr = LCD_ADDR_1;
  else if (HAL_I2C_IsDeviceReady(&hi2c1, LCD_ADDR_2, 2, 50) == HAL_OK) lcd_addr = LCD_ADDR_2;
  else return 0;

  HAL_Delay(50);
  lcd_write4(0x30 | LCD_BL); HAL_Delay(5);
  lcd_write4(0x30 | LCD_BL); HAL_Delay(1);
  lcd_write4(0x30 | LCD_BL); HAL_Delay(1);
  lcd_write4(0x20 | LCD_BL); HAL_Delay(1);   /* chuyen sang 4-bit */

  lcd_cmd(0x28);   /* 4-bit, 2 dong, font 5x8 */
  lcd_cmd(0x0C);   /* bat hien thi, tat con tro */
  lcd_cmd(0x06);   /* tu dong tang dia chi */
  lcd_cmd(0x01);   /* xoa man hinh */
  return 1;
}

/* Khoi dong lai bus I2C va LCD (chong treo man hinh) */
static void lcd_reinit(void)
{
  lcd_err = 0;
  i2c_recover();
  lcd_init();
}

/* ====================== RTC DS1307 / DS3231 (0x68) ===================== */
static uint8_t bcd2dec(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static uint8_t dec2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

static uint8_t rtc_read(RtcTime_t *t, uint8_t *halted)
{
  uint8_t b[7];
  if (HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, b, 7, 100) != HAL_OK)
    return 0;
  *halted = (b[0] & 0x80) ? 1 : 0;       /* bit CH cua DS1307 */
  t->sec   = bcd2dec(b[0] & 0x7F);
  t->min   = bcd2dec(b[1] & 0x7F);
  t->hour  = bcd2dec(b[2] & 0x3F);
  t->day   = bcd2dec(b[4] & 0x3F);
  t->month = bcd2dec(b[5] & 0x1F);
  t->year  = bcd2dec(b[6]);
  return 1;
}

static uint8_t rtc_write(const RtcTime_t *t)
{
  uint8_t b[7];
  b[0] = dec2bcd(t->sec) & 0x7F;         /* bit7 = 0 -> cho dong ho chay */
  b[1] = dec2bcd(t->min);
  b[2] = dec2bcd(t->hour);               /* che do 24h */
  b[3] = 1;
  b[4] = dec2bcd(t->day);
  b[5] = dec2bcd(t->month);
  b[6] = dec2bcd(t->year);
  return (HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, b, 7, 100) == HAL_OK);
}

static void rtc_get_compile_time(RtcTime_t *t)
{
  static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char m[4] = {0};
  int d = 1, y = 2026, hh = 0, mm = 0, ss = 0;
  sscanf(__DATE__, "%3s %d %d", m, &d, &y);
  sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);
  const char *p = strstr(months, m);
  t->month = p ? (uint8_t)((p - months) / 3 + 1) : 1;
  t->day   = (uint8_t)d;
  t->year  = (uint8_t)(y % 100);
  t->hour  = (uint8_t)hh;
  t->min   = (uint8_t)mm;
  t->sec   = (uint8_t)ss;
}

/* Neu RTC chua co gio dung -> nap gio luc build */
static void rtc_init_if_needed(void)
{
  RtcTime_t t; uint8_t halted = 0;
  uint8_t ok = rtc_read(&t, &halted);
  if (!ok || halted || t.year < 26 || t.month < 1 || t.month > 12 ||
      t.day < 1 || t.day > 31 || t.hour > 23 || t.min > 59 || t.sec > 59)
  {
    rtc_get_compile_time(&t);
    rtc_write(&t);
  }
}

/* ============================== DHT11 ================================== */
static void DHT11_SetOutput(void)
{
  GPIO_InitTypeDef g = {0};
  g.Pin = DHT11_Pin;
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DHT11_GPIO_Port, &g);
}

static void DHT11_SetInput(void)
{
  GPIO_InitTypeDef g = {0};
  g.Pin = DHT11_Pin;
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_GPIO_Port, &g);
}

static uint8_t DHT11_Wait(GPIO_PinState level, uint16_t timeout_us)
{
  __HAL_TIM_SET_COUNTER(&htim2, 0);
  while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) != level)
  {
    if (__HAL_TIM_GET_COUNTER(&htim2) > timeout_us) return 0;
  }
  return 1;
}

static uint8_t DHT11_Read(uint8_t *temp, uint8_t *hum)
{
  uint8_t data[5] = {0};
  uint8_t ok = 1;

  DHT11_SetOutput();
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);
  delay_us(30);
  DHT11_SetInput();

  __disable_irq();

  if (!DHT11_Wait(GPIO_PIN_RESET, 100)) ok = 0;
  if (ok && !DHT11_Wait(GPIO_PIN_SET, 100)) ok = 0;
  if (ok && !DHT11_Wait(GPIO_PIN_RESET, 100)) ok = 0;

  for (uint8_t i = 0; ok && i < 40; i++)
  {
    if (!DHT11_Wait(GPIO_PIN_SET, 100)) { ok = 0; break; }
    delay_us(40);
    if (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET)
      data[i / 8] |= (uint8_t)(1 << (7 - (i % 8)));
    if (!DHT11_Wait(GPIO_PIN_RESET, 100)) { ok = 0; break; }
  }

  __enable_irq();

  DHT11_SetOutput();
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);

  if (!ok) return 0;
  if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) return 0;

  *hum  = data[0];
  *temp = data[2];
  return 1;
}

/* ============================ CAM BIEN DAT ============================= */
static uint16_t soil_read_raw(void)
{
  uint32_t sum = 0;

  HAL_GPIO_WritePin(SOIL_PWR_PORT, SOIL_PWR_PIN, GPIO_PIN_SET);     /* bat nguon cam bien */
  HAL_Delay(SOIL_SETTLE_MS);

  for (uint8_t i = 0; i < 16; i++)
  {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    sum += HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
  }

  HAL_GPIO_WritePin(SOIL_PWR_PORT, SOIL_PWR_PIN, GPIO_PIN_RESET);   /* tat nguon -> chong an mon */
  return (uint16_t)(sum / 16);
}

/* Quy doi ra %: dry -> 0%, wet -> 100% (dung ca khi cam bien dao chieu) */
static uint8_t soil_percent(uint16_t raw)
{
  int32_t span = (int32_t)cfg.wet - (int32_t)cfg.dry;
  if (span == 0) return 0;
  int32_t p = ((int32_t)raw - (int32_t)cfg.dry) * 100 / span;
  if (p < 0)   p = 0;
  if (p > 100) p = 100;
  return (uint8_t)p;
}

/* =============================== RELAY ================================= */
static void relay_set(uint8_t on)
{
  relay_state = on;
  HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin,
                    on ? RELAY_ON_LEVEL : (RELAY_ON_LEVEL == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET));
}

static void pump_start_timed(void)
{
  pump_timed = 1;
  pump_timed_end = HAL_GetTick() + (uint32_t)cfg.pump_s * 1000UL;
}

/* ===================== LUU CAI DAT VAO FLASH ============================ */
static void cfg_default(void)
{
  memset(&cfg, 0, sizeof(cfg));
  cfg.magic    = CFG_MAGIC;
  cfg.auto_en  = DEF_AUTO_EN;
  cfg.start    = DEF_SOIL_START;
  cfg.stop     = DEF_SOIL_STOP;
  cfg.sched_en = DEF_SCHED_EN;
  cfg.sched_h  = DEF_SCHED_H;
  cfg.sched_m  = DEF_SCHED_M;
  cfg.pump_s   = DEF_PUMP_SEC;
  cfg.dry      = DEF_ADC_DRY;
  cfg.wet      = DEF_ADC_WET;
}

static void cfg_load(void)
{
  memcpy(&cfg, (const void *)CFG_ADDR, sizeof(cfg));
  if (cfg.magic != CFG_MAGIC || cfg.start > 95 || cfg.stop > 100 || cfg.start + 5 > cfg.stop ||
      cfg.sched_h > 23 || cfg.sched_m > 59 || cfg.pump_s < 1 || cfg.pump_s > 120 ||
      cfg.auto_en > 1 || cfg.sched_en > 1 || cfg.dry > 4095 || cfg.wet > 4095)
  {
    cfg_default();
  }
}

static void cfg_save(void)
{
  FLASH_EraseInitTypeDef e;
  uint32_t err = 0;
  const uint16_t *p = (const uint16_t *)&cfg;

  cfg.magic = CFG_MAGIC;
  HAL_FLASH_Unlock();
  e.TypeErase   = FLASH_TYPEERASE_PAGES;
  e.PageAddress = CFG_ADDR;
  e.NbPages     = 1;
  if (HAL_FLASHEx_Erase(&e, &err) == HAL_OK)
  {
    for (uint8_t i = 0; i < sizeof(cfg) / 2; i++)
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, CFG_ADDR + 2U * i, p[i]);
  }
  HAL_FLASH_Lock();
  cfg_dirty = 0;
}

/* ============================== NUT NHAN =============================== */
/* Tra ve 1 moi lan nhan (co chong doi 30ms). repeat=1: giu nut se tu lap */
static uint8_t btn_event(uint8_t i, uint32_t now, uint8_t repeat)
{
  Btn_t *b = &btns[i];
  uint8_t pressed = (HAL_GPIO_ReadPin(b->port, b->pin) == GPIO_PIN_RESET) ? 1 : 0;

  if (pressed != b->raw) { b->raw = pressed; b->t_change = now; return 0; }
  if (b->raw != b->state && (now - b->t_change) >= 30)
  {
    b->state = b->raw;
    if (b->state) { b->t_next = now + 600; return 1; }
    return 0;
  }
  if (b->state && repeat && now >= b->t_next) { b->t_next = now + 120; return 1; }
  return 0;
}

/* ============================ GIAO DIEN LCD ============================ */
static void ui_draw(void)
{
  char a[32], b[32], ps[16];
  uint32_t t = HAL_GetTick();
  a[0] = 0; b[0] = 0;

  switch (page)
  {
    case P_HOME:
    {
      uint8_t v = (uint8_t)((t / 3000UL) % 4UL);   /* doi man hinh moi 3 giay */
      if (v == 0)
      {
        if (g_dht_ok)
        {
          snprintf(a, sizeof a, "Nhiet do: %2d%cC", g_temp, 0xDF);
          snprintf(b, sizeof b, "Do am KK: %2d%%", g_hum);
        }
        else
        {
          snprintf(a, sizeof a, "Nhiet do: --");
          snprintf(b, sizeof b, "Do am KK: --");
        }
      }
      else if (v == 1)
      {
        if (g_rtc_ok) snprintf(a, sizeof a, "Gio: %02d:%02d:%02d", g_now.hour, g_now.min, g_now.sec);
        else          snprintf(a, sizeof a, "Gio: --:--:--");
        if (pump_timed)
        {
          int32_t rem = (int32_t)(pump_timed_end - t);
          if (rem < 0) rem = 0;
          snprintf(ps, sizeof ps, "%lus", (unsigned long)((rem + 999) / 1000));
        }
        else
          snprintf(ps, sizeof ps, "%s", relay_state ? "ON" : "OFF");
        snprintf(b, sizeof b, "Dat:%3d%% Bom:%-3s", g_soil, ps);
      }
      else if (v == 2)
      {
        snprintf(a, sizeof a, "Hen %02d:%02d %s", cfg.sched_h, cfg.sched_m, cfg.sched_en ? "ON" : "OFF");
        snprintf(b, sizeof b, "Auto %s %d-%d%%", cfg.auto_en ? "ON" : "OFF", cfg.start, cfg.stop);
      }
      else
      {
        snprintf(a, sizeof a, "ADC dat: %4d", g_raw);
        snprintf(b, sizeof b, "K:%4d U:%4d", cfg.dry, cfg.wet);
      }
      break;
    }
    case P_AUTO:
      snprintf(a, sizeof a, "Tu dong tuoi:");
      snprintf(b, sizeof b, "%s (UP/DOWN)", cfg.auto_en ? "ON" : "OFF");
      break;
    case P_START:
      snprintf(a, sizeof a, "Bat khi dat <");
      snprintf(b, sizeof b, "%d%% (UP/DOWN)", cfg.start);
      break;
    case P_STOP:
      snprintf(a, sizeof a, "Tat khi dat >=");
      snprintf(b, sizeof b, "%d%% (UP/DOWN)", cfg.stop);
      break;
    case P_SCHED_EN:
      snprintf(a, sizeof a, "Hen gio tuoi:");
      snprintf(b, sizeof b, "%s (UP/DOWN)", cfg.sched_en ? "ON" : "OFF");
      break;
    case P_SCHED_H:
      snprintf(a, sizeof a, "Chinh GIO tuoi");
      snprintf(b, sizeof b, "[%02d]:%02d", cfg.sched_h, cfg.sched_m);
      break;
    case P_SCHED_M:
      snprintf(a, sizeof a, "Chinh PHUT tuoi");
      snprintf(b, sizeof b, "%02d:[%02d]", cfg.sched_h, cfg.sched_m);
      break;
    case P_PUMP_S:
      snprintf(a, sizeof a, "Thoi gian bom:");
      snprintf(b, sizeof b, "%d giay", cfg.pump_s);
      break;
    case P_CAL_DRY:
      snprintf(a, sizeof a, "Calib KHO (kk)");
      if (t < msg_until) snprintf(b, sizeof b, "Da luu: %4d", g_raw);
      else               snprintf(b, sizeof b, "ADC:%4d UP=Luu", g_raw);
      break;
    case P_CAL_WET:
      snprintf(a, sizeof a, "Calib UOT (nuoc)");
      if (t < msg_until) snprintf(b, sizeof b, "Da luu: %4d", g_raw);
      else               snprintf(b, sizeof b, "ADC:%4d UP=Luu", g_raw);
      break;
    default:
      break;
  }
  lcd_line(0, a);
  lcd_line(1, b);
}

/* Chinh gia tri trong menu: d = +1 (UP) hoac -1 (DOWN) */
static void menu_adjust(int8_t d)
{
  int16_t v;
  switch (page)
  {
    case P_AUTO:     cfg.auto_en  ^= 1; break;
    case P_SCHED_EN: cfg.sched_en ^= 1; break;
    case P_START:
      v = (int16_t)cfg.start + d;
      if (v < 0) v = 0;
      if (v > (int16_t)cfg.stop - 5) v = (int16_t)cfg.stop - 5;
      cfg.start = (uint8_t)v;
      break;
    case P_STOP:
      v = (int16_t)cfg.stop + d;
      if (v < (int16_t)cfg.start + 5) v = (int16_t)cfg.start + 5;
      if (v > 100) v = 100;
      cfg.stop = (uint8_t)v;
      break;
    case P_SCHED_H:  cfg.sched_h = (uint8_t)((cfg.sched_h + 24 + d) % 24); break;
    case P_SCHED_M:  cfg.sched_m = (uint8_t)((cfg.sched_m + 60 + d) % 60); break;
    case P_PUMP_S:
      v = (int16_t)cfg.pump_s + d;
      if (v < 1)   v = 1;
      if (v > 120) v = 120;
      cfg.pump_s = (uint8_t)v;
      break;
    case P_CAL_DRY:
      if (d > 0) { cfg.dry = g_raw; msg_until = HAL_GetTick() + 1500; }
      else return;
      break;
    case P_CAL_WET:
      if (d > 0) { cfg.wet = g_raw; msg_until = HAL_GetTick() + 1500; }
      else return;
      break;
    default:
      return;
  }
  cfg_dirty = 1;
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start(&htim2);                 /* TIM2 chay 1 MHz cho DHT11 */
  HAL_ADCEx_Calibration_Start(&hadc1);        /* hieu chuan ADC */

  /* 4 nut nhan PB12..PB15: dau vao keo len, nhan = noi xuong GND */
  {
    GPIO_InitTypeDef gb = {0};
    gb.Pin  = BTN_MENU_PIN | BTN_UP_PIN | BTN_DOWN_PIN | BTN_PUMP_PIN;
    gb.Mode = GPIO_MODE_INPUT;
    gb.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gb);
  }

  /* Chan cap nguon cam bien dat (PB0): xuat, mac dinh TAT */
  {
    GPIO_InitTypeDef gp = {0};
    HAL_GPIO_WritePin(SOIL_PWR_PORT, SOIL_PWR_PIN, GPIO_PIN_RESET);
    gp.Pin   = SOIL_PWR_PIN;
    gp.Mode  = GPIO_MODE_OUTPUT_PP;
    gp.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SOIL_PWR_PORT, &gp);
  }

  cfg_load();                                 /* doc cai dat da luu (hoac mac dinh) */
  relay_set(0);                               /* luc khoi dong relay TAT */
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);

  if (!lcd_init())
  {
    /* Khong thay LCD tren I2C: nhay LED PC13 nhanh de bao loi */
    while (1)
    {
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      HAL_Delay(100);
    }
  }

  lcd_line(0, "  He thong tuoi");
  lcd_line(1, "  Dang khoi dong");
  HAL_Delay(1000);
  rtc_init_if_needed();
  lcd_cmd(0x01);

  uint8_t  halted = 0;
  uint32_t t_adc = 0, t_dht = 0, t_1s = 0;
  uint32_t last_act = HAL_GetTick();
  uint32_t t_lcd = HAL_GetTick(), lcd_reinit_at = 0;
  uint8_t  last_relay = 0;

  g_raw  = soil_read_raw();
  g_soil = soil_percent(g_raw);

  /* Watchdog doc lap (~4 giay): neu chuong trinh treo cung se tu reset */
  DBGMCU->CR |= DBGMCU_CR_DBG_IWDG_STOP;      /* dung watchdog khi dang debug */
  IWDG->KR  = 0xCCCC;                         /* bat watchdog */
  IWDG->KR  = 0x5555;                         /* cho phep ghi PR, RLR */
  IWDG->PR  = 4;                              /* chia 64 -> 625 Hz */
  IWDG->RLR = 2500;                           /* 2500 / 625 = 4 giay */
  for (uint32_t i = 0; IWDG->SR != 0 && i < 100000UL; i++) { }
  IWDG->KR  = 0xAAAA;


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    uint32_t tick = HAL_GetTick();
    uint8_t  redraw = 0;

    IWDG->KR = 0xAAAA;                           /* nap lai watchdog */

    /* ---------- NUT NHAN ---------- */
    if (btn_event(BTN_PUMP, tick, 0))            /* bom tay: bam lan nua de dung */
    {
      if (pump_timed) pump_timed = 0;
      else            pump_start_timed();
      redraw = 1;
    }

    if (btn_event(BTN_MENU, tick, 0))
    {
      last_act = tick;
      page++;
      if (page >= P_COUNT)
      {
        page = P_HOME;
        if (cfg_dirty) cfg_save();
      }
      redraw = 1;
    }

    if (btn_event(BTN_UP, tick, 1))
    {
      last_act = tick;
      if (page != P_HOME) { menu_adjust(+1); redraw = 1; }
    }
    if (btn_event(BTN_DOWN, tick, 1))
    {
      last_act = tick;
      if (page != P_HOME) { menu_adjust(-1); redraw = 1; }
    }

    /* Khong bam nut 30 giay -> tu thoat menu va luu */
    if (page != P_HOME && (tick - last_act) > 30000UL)
    {
      page = P_HOME;
      if (cfg_dirty) cfg_save();
      redraw = 1;
    }

    /* ---------- DOC CAM BIEN DAT: moi SOIL_READ_PERIOD_MS ---------- */
    if (tick - t_adc >= SOIL_READ_PERIOD_MS)
    {
      t_adc  = tick;
      g_raw  = soil_read_raw();
      g_soil = soil_percent(g_raw);
      if (page == P_CAL_DRY || page == P_CAL_WET) redraw = 1;
    }

    /* ---------- DHT11: moi 2 giay ---------- */
    if (tick - t_dht >= 2000)
    {
      t_dht = tick;
      uint8_t tt, hh;
      if (DHT11_Read(&tt, &hh)) { g_temp = tt; g_hum = hh; g_dht_ok = 1; }
      else                      { g_dht_ok = 0; }
    }

    /* ---------- LOGIC BOM: moi 1 giay ---------- */
    if (tick - t_1s >= 1000)
    {
      t_1s = tick;
      g_rtc_ok = rtc_read(&g_now, &halted);
      if (!g_rtc_ok) lcd_err = 1;                 /* bus I2C co van de -> se khoi dong lai */

      /* 1) Tu dong theo do am dat */
      if (cfg.auto_en)
      {
        if (!auto_run)
        {
          if (g_soil < cfg.start && (int32_t)(tick - auto_cool_until) >= 0)
          {
            auto_run = 1;
            auto_run_start = tick;
          }
        }
        else
        {
          if (g_soil >= cfg.stop)
          {
            auto_run = 0;                              /* du am -> dung */
          }
          else if ((tick - auto_run_start) >= AUTO_MAX_RUN_MS)
          {
            auto_run = 0;                              /* bom qua lau -> nghi */
            auto_cool_until = tick + AUTO_COOL_MS;
          }
        }
      }
      else
      {
        auto_run = 0;
      }

      /* 2) Hen gio: den gio thi bom pump_s giay (moi phut chi kich 1 lan) */
      if (g_rtc_ok && cfg.sched_en &&
          g_now.hour == cfg.sched_h && g_now.min == cfg.sched_m)
      {
        uint32_t key = (uint32_t)g_now.month * 100000UL + (uint32_t)g_now.day * 1440UL
                     + (uint32_t)g_now.hour * 60UL + g_now.min;
        if (key != sched_last_key)
        {
          sched_last_key = key;
          pump_start_timed();
        }
      }

      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);      /* LED nhay = dang chay */
      redraw = 1;
    }

    /* ---------- HET GIO BOM TAY / HEN GIO ---------- */
    if (pump_timed && (int32_t)(tick - pump_timed_end) >= 0) pump_timed = 0;

    /* ---------- DIEU KHIEN RELAY ---------- */
    relay_set((uint8_t)(pump_timed || auto_run));

    /* Chong treo man hinh: relay vua dong/ngat -> doi 150ms cho het nhieu
       roi khoi dong lai I2C + LCD. Cung khoi dong lai khi co loi hoac moi 30 giay */
    if (relay_state != last_relay)
    {
      last_relay = relay_state;
      lcd_reinit_at = tick + 150;
    }
    if ((lcd_reinit_at != 0 && (int32_t)(tick - lcd_reinit_at) >= 0) ||
        (lcd_err && (tick - t_lcd) >= 500) ||
        ((tick - t_lcd) >= LCD_REINIT_MS))
    {
      lcd_reinit_at = 0;
      t_lcd = tick;
      lcd_reinit();
      redraw = 1;
    }

    if (redraw) ui_draw();

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */
  /* Cam bien dien tro co tro khang ra lon -> lay mau 239.5 chu ky cho ket qua chinh xac */
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, RELAY_Pin|DHT11_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RELAY_Pin DHT11_Pin */
  GPIO_InitStruct.Pin = RELAY_Pin|DHT11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
