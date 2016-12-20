/**
 * Example of using the SSD1351 with a ESP32 module (should work with ESP8266 as well, just slower.
 * Mostly inspired by https://github.com/intel-iot-devkit/upm/tree/master/src/ssd1351 which in turn is
 * inspired by the adafruit library but the edison example is self contained and eaiser to follow.
 * Could probably fix the adafruit library to work but for my needs I don't need the full GFX lib.
 */

#include <SPI.h>

#define p_sclk 18
#define p_mosi 23
#define p_dc   17
#define p_cs   4
#define p_rst  2

#define SSD1351HEIGHT 128
#define SSD1351WIDTH 128

// Timing Delays
#define SSD1351_DELAYS_HWFILL      (3)
#define SSD1351_DELAYS_HWLINE       (1)

// SSD1351 Commands
#define SSD1351_CMD_SETCOLUMN     0x15
#define SSD1351_CMD_SETROW        0x75
#define SSD1351_CMD_WRITERAM      0x5C
#define SSD1351_CMD_READRAM       0x5D
#define SSD1351_CMD_SETREMAP    0xA0
#define SSD1351_CMD_STARTLINE     0xA1
#define SSD1351_CMD_DISPLAYOFFSET   0xA2
#define SSD1351_CMD_DISPLAYALLOFF   0xA4
#define SSD1351_CMD_DISPLAYALLON    0xA5
#define SSD1351_CMD_NORMALDISPLAY   0xA6
#define SSD1351_CMD_INVERTDISPLAY   0xA7
#define SSD1351_CMD_FUNCTIONSELECT  0xAB
#define SSD1351_CMD_DISPLAYOFF    0xAE
#define SSD1351_CMD_DISPLAYON       0xAF
#define SSD1351_CMD_PRECHARGE     0xB1
#define SSD1351_CMD_DISPLAYENHANCE  0xB2
#define SSD1351_CMD_CLOCKDIV    0xB3
#define SSD1351_CMD_SETVSL    0xB4
#define SSD1351_CMD_SETGPIO     0xB5
#define SSD1351_CMD_PRECHARGE2    0xB6
#define SSD1351_CMD_SETGRAY     0xB8
#define SSD1351_CMD_USELUT    0xB9
#define SSD1351_CMD_PRECHARGELEVEL  0xBB
#define SSD1351_CMD_VCOMH     0xBE
#define SSD1351_CMD_CONTRASTABC   0xC1
#define SSD1351_CMD_CONTRASTMASTER  0xC7
#define SSD1351_CMD_MUXRATIO            0xCA
#define SSD1351_CMD_COMMANDLOCK         0xFD
#define SSD1351_CMD_HORIZSCROLL         0x96
#define SSD1351_CMD_STOPSCROLL          0x9E
#define SSD1351_CMD_STARTSCROLL         0x9F

// Color definitions
#define  BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

void reset() {
  digitalWrite(p_rst, HIGH);
  delay(500);
  digitalWrite(p_rst, LOW);
  delay(500);
  digitalWrite(p_rst, HIGH);
  delay(500);
}

void writeCommand(uint8_t command) {
  digitalWrite(p_dc, LOW);
  SPI.write(command);
}

void writeData(uint8_t data) {
  digitalWrite(p_dc, HIGH);
  SPI.write(data);
}

void initialize() {
  SPI.setFrequency(8 * 1000000);
  SPI.setDataMode(SPI_MODE0);
  SPI.write(0x00); // Need to bring clk high before init
  
  digitalWrite(p_cs, LOW);
  reset();
  
  writeCommand(SSD1351_CMD_COMMANDLOCK);
  writeData(0x12);

  writeCommand(SSD1351_CMD_COMMANDLOCK);
  writeData(0xB1);

  writeCommand(SSD1351_CMD_DISPLAYOFF);

  writeCommand(SSD1351_CMD_CLOCKDIV);
  writeCommand(0xF1);

  writeCommand(SSD1351_CMD_MUXRATIO);
  writeData(127);

  writeCommand(SSD1351_CMD_SETREMAP);
  writeData(0x74);

  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(0x00);
  writeData(0x7F);

  writeCommand(SSD1351_CMD_SETROW);
  writeData(0x00);
  writeData(0x7F);

  writeCommand(SSD1351_CMD_STARTLINE);
  if (SSD1351HEIGHT == 96) {
      writeData(96);
  } else {
      writeData(0);
  }

  writeCommand(SSD1351_CMD_DISPLAYOFFSET);
  writeData(0x0);

  writeCommand(SSD1351_CMD_SETGPIO);
  writeData(0x00);

  writeCommand(SSD1351_CMD_FUNCTIONSELECT);
  writeData(0x01);

  writeCommand(SSD1351_CMD_PRECHARGE);
  writeCommand(0x32);

  writeCommand(SSD1351_CMD_VCOMH);
  writeCommand(0x05);

  writeCommand(SSD1351_CMD_NORMALDISPLAY);

  writeCommand(SSD1351_CMD_CONTRASTABC);
  writeData(0xC8);
  writeData(0x80);
  writeData(0xC8);

  writeCommand(SSD1351_CMD_CONTRASTMASTER);
  writeData(0x0F);

  writeCommand(SSD1351_CMD_SETVSL );
  writeData(0xA0);
  writeData(0xB5);
  writeData(0x55);

  writeCommand(SSD1351_CMD_PRECHARGE2);
  writeData(0x01);

  writeCommand(SSD1351_CMD_DISPLAYON);
}

void moveCursor(short x, short y) {
  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(x);
  writeData(SSD1351WIDTH-1);

  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(SSD1351HEIGHT-1);

  writeCommand(SSD1351_CMD_WRITERAM);  
}

void drawPixel(uint16_t x, uint16_t y, uint16_t color) {
  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(x);
  writeData(SSD1351WIDTH-1);
  
  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(SSD1351HEIGHT-1);
  
  writeCommand(SSD1351_CMD_WRITERAM);
  writeData(color >> 8);
  writeData(color);
}

void setup() {
  Serial.begin(9600);
  pinMode(p_sclk, OUTPUT);
  pinMode(p_mosi, OUTPUT);
  pinMode(p_dc, OUTPUT);
  pinMode(p_cs, OUTPUT);
  pinMode(p_rst, OUTPUT);
  
  SPI.begin(p_sclk, -1, p_mosi, p_cs);

  initialize();
}

const uint16_t buffer_size = SSD1351HEIGHT * SSD1351WIDTH * 2;
uint8_t buffer[buffer_size]; /** Screen buffer */ 
#define BLOCKS              8

void refreshFromBuffer() {
  writeCommand(SSD1351_CMD_WRITERAM);
  digitalWrite(p_dc, HIGH);
  const int blockSize = SSD1351HEIGHT * SSD1351WIDTH * 2 / BLOCKS;
  for (int block = 0; block < BLOCKS; block++) {
      SPI.writeBytes(&buffer[block * blockSize], blockSize);
  }
}

void SetAllBuffer(uint16_t color) {
  for (int i=0; i < buffer_size; i+=2) {
    buffer[i] = color >> 8;
    buffer[i+1] = color;
  }
}

bool white = true;
// Draw entire screen buffer each cycle, will flash black/white.
void loop_flash() {
  SetAllBuffer((white = !white) ? WHITE : BLACK);
  refreshFromBuffer();
}

short x = 0;
short y = 0;
// Draw one pixel each cycle, will sweep from black to white and back.
void loop_sweep() {
  drawPixel(x, y, white ? WHITE : BLACK);
  
  if (y % 128 == 0) {
    x = ++x % 128;
    if (x == 127) {
      white = !white;
    }
  }
  y = ++y % 128;
}

void loop() {
  // hard code whether you want to flash or sweep.
  bool flash = false;
  if (flash) {
    loop_flash();
  } else {
    loop_sweep();
  }
}

