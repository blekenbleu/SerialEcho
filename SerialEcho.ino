#include <Arduino.h>
#include "USBCDC.h"

USBCDC Serial0;
#define TRYTWO 0
#if TRYTWO
USBCDC Serial3;
HardwareSerial SerHW(PA3, PA2);	// RX2, TX2 https://forum.arduino.cc/t/stm32f411ce-black-pill-serial-port-pin-mapping/907459
#define SetSer SerHW
#else
HardwareSerial Serial3(PA3, PA2);
#define SetSer Serial3
#endif

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
  Serial0.begin(19200);
  Serial3.begin(19200);
#if TRYTWO
  SerHW.begin(19200);
#endif
  SetSer.print("USB_CFGBUFFER_LEN = "); SetSer.println(USB_CFGBUFFER_LEN);
  USB_Begin();
  SetSer.println("USB_Begin()");
  LEDb4();   

  while (!USB_Running())
  {
    // until usb connected
    delay(80);
    LEDb4();
  }
  SetSer.println("USB_Running()");
  while (!Serial0)
    wait();
  Serial0.println("Serial0_Running()");
  SetSer.println("Serial0.begin(19200) running");
#if TRYTWO
  while (!Serial3)
    wait();
  Serial3.println("Serial3.begin(19200) running");
  SetSer.println("Serial3.begin(19200) running");
#endif
} // start()

void setup()
{
  // https://hackaday.com/2021/01/20/blue-pill-vs-black-pill-transitioning-from-stm32f103-to-stm32f411/
  pinMode(PC13, OUTPUT);    // LED
  LEDb4();
  start();
  SetSer.print("USB_EP_SIZE = "); SetSer.println(USB_EP_SIZE);
  now = then = millis();
}

int j = 0;
void loop()
{
  int l, m;

  LED();

  while (0 < (l = Serial0.available()) && millis() > 50 + now)
  {
    int i, a = Serial3.availableForWrite();

    timeout = false;
    if (a > USB_EP_SIZE)
      a = USB_EP_SIZE;
    if (l > a)
      l = a;
    if (0 >= l)
      Serial0.println("Serial3 unavail");
    else
    {
      for (i = 0; i < l; i++)
        buffer[i] = Serial0.read();
      buffer[i - 1] = '\0';
      Serial0.println((l <= Serial3.println(buffer)) ? "ok" : "fail");
      msec = 100;
    }
    now = millis();
  }

  while (0 < (l = Serial3.available()))
  {
    int a = Serial0.availableForWrite();

    if (a > USB_EP_SIZE)
      a = USB_EP_SIZE;
    if (l > a)
      l = a;
    if (0 >= l)
      Serial3.println("Serial0 unavail");
    else
    {
      for (int i; i < l; i++)
      {
        buffer2[j++] = Serial3.read();
        if (j >= (USB_EP_SIZE - 2) || 10 == buffer2[j-1])
        {
          buffer2[j] = '\0';
          Serial3.println((j == Serial0.write(buffer2, j)) ? "ok" : "fail");
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
    Serial3.println("waiting...");
    msec = 1250;
    timeout = true;
    now = millis();
  }
}
