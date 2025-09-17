#include <Arduino.h>
#include "USBCDC.h"

#define TRYTWO 1

USBCDC Serial0;

#if TRYTWO
USBCDC SerialC;
#define SetSer SerHW
#else
#define SetSer SerialC
#endif

// append to ARDUINO_:  grep board= AppData/Local/Arduino15/packages/STMicroelectronics/hardware/stm32/*/boards.txt | cut -f2 -d= | sort -u
// TX  RX  BLUEPILL_F103C6 BLUEPILL_F103C8 BLUEPILL_F103CB https://i.pinimg.com/originals/b8/46/45/b84645f6a15e07e893e3eac22613a57b.png
// B6  B7  Serial1
// A2  A3  Serial2
// B10 B11 Serial3
// https://miro.medium.com/v2/resize:fit:3760/1*ArK_z-CP3VN57ramxamAIg.png
// TX  RX  BLACKPILL_F103C8 BLACKPILL_F103CB BLACKPILL_F303CC BLACKPILL_F401CC BLACKPILL_F401CE BLACKPILL_F411CE
// A9  A10 Serial1
// A2  A3  Serial2	// same as Blue Pill

//#if defined(ARDUINO_BLACKPILL_F411CE) || defined(BLACKPILL_F401CE) || defined(BLACKPILL_F401CC) || defined(BLACKPILL_F303CC) || defined(BLACKPILL_F103CB) || defined(BLACKPILL_F103C8)
HardwareSerial SetSer(PA3, PA2);	// RX2, TX2 https://forum.arduino.cc/t/stm32f411ce-black-pill-serial-port-pin-mapping/907459
//#elif defined(BLUEPILL_F103C6)  || defined(BLUEPILL_F103C8)  || defined(BLUEPILL_F103CB)
//#endif

char buffer[USB_EP_SIZE + 1], buffer2[USB_EP_SIZE + 1];
unsigned long now, then, msec;
bool toggle = true, timeout = false;

void LEDb4()
{
  digitalWrite(PC13, toggle ? HIGH : LOW);
  toggle = !toggle;
}

void LED()
{
  if (millis() < then + msec)
    return;

  if (timeout)
    msec = 1250;
  else if (25 > Serial0.availableForWrite())
    msec = (toggle) ? 400 : 800;
  LEDb4();
  then = millis();
}

void wait()
{
  delay(500);    // syncopated blink
  LEDb4();
  delay(250);
  LEDb4();
  delay(750);
  LEDb4();
}

void start()
{
  LEDb4();   
  SetSer.begin(19200);
  SetSer.print("USB_CFGBUFFER_LEN = "); SetSer.println(USB_CFGBUFFER_LEN);
  SetSer.print("USB_EP_SIZE = "); SetSer.println(USB_EP_SIZE);
#ifdef ARDUINO_BLACKPILL_F411CE
  SetSer.println("ARDUINO_BLACKPILL_F411CE");
#elifdef ARDUINO_BLUEPILL_F103C6
  SetSer.println("ARDUINO_BLUEPILL_F103C6");	// 72 MHz, 32KB flash, 10KB SRAM
#elifdef ARDUINO_BLUEPILL_F103C8				// ST-Link reports 64KB flash
  SetSer.println("ARDUINO_BLUEPILL_F103C8");	// 72 MHz, 64KB flash, 20KB SRAM
#elifdef ARDUINO_BLUEPILL_F103CB
  SetSer.println("ARDUINO_BLUEPILL_F103CB");	// 72 MHz, 128KB flash, 20KB SRAM
#endif
  USB_Begin();
  SetSer.println("USB_Begin;\n\tnow waiting for USB_Running");
  while (!USB_Running())
  {
    // until usb connected
    delay(80);
    LEDb4();
  }
  SetSer.println("USB_Running() success");

  Serial0.begin(19200);
#if TRYTWO
  SetSer.println("Trying for composite USB");
  SerialC.begin(19200);
#endif
  while (!Serial0)
    wait();
  Serial0.println("Serial0_Running()");
  SetSer.println("Serial0.begin(19200) running");
#if TRYTWO
  while (!SerialC)
    wait();
  SerialC.println("SerialC.begin(19200) running");
  SetSer.println("SerialC.begin(19200) running");
#endif
} // start()

void setup()
{
  // https://hackaday.com/2021/01/20/blue-pill-vs-black-pill-transitioning-from-stm32f103-to-stm32f411/
  pinMode(PC13, OUTPUT);    // LED
  LEDb4();
  start();
  now = then = millis();
}

int j = 0;
void loop()
{
  int l, m;

  LED();

  while (0 < (l = Serial0.available()) && millis() > 50 + now)
  {
    int i, a = SerialC.availableForWrite();

    timeout = false;
    if (a > USB_EP_SIZE)
      a = USB_EP_SIZE;
    if (l > a)
      l = a;
    if (0 >= l)
      Serial0.println("SerialC unavail");
    else
    {
      for (i = 0; i < l; i++)
        buffer[i] = Serial0.read();
      buffer[i - 1] = '\0';
      Serial0.println((l <= SerialC.println(buffer)) ? "ok" : "fail");
      msec = 100;
    }
    now = millis();
  }

  while (0 < (l = SerialC.available()))
  {
    int a = Serial0.availableForWrite();

    if (a > USB_EP_SIZE)
      a = USB_EP_SIZE;
    if (l > a)
      l = a;
    if (0 >= l)
      SerialC.println("Serial0 unavail");
    else
    {
      for (int i; i < l; i++)
      {
        buffer2[j++] = SerialC.read();
        if (j >= (USB_EP_SIZE - 2) || 13 == buffer2[j-1]) // PuTTY Enter key default
        {
          buffer2[j] = '\0';
          SerialC.println((j == Serial0.write(buffer2, j)) ? "ok" : "fail");
          j = 0;
          msec = 200;
          timeout = false;
        }
      }
    }
    now = millis();
  }

  if (millis() > 10000 + now && ! timeout)    // suspiciously quiet
  {
    Serial0.println("waiting...");
    SerialC.println("waiting...");
    msec = 1250;
    timeout = true;
    now = millis();
  }
}
