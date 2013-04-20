#include <OneWire.h>
// #include <SoftwareSerial.h>
#include <LiquidCrystal.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

OneWire  ds(2);  // on pin 10 (a 4.7K resistor is necessary)

//SoftwareSerial disp(11, 10);

LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

const byte CMD_CLEAR = 0x00;
const byte CMD_WRITE_RAW = 0x01;
const byte CMD_WRITE_PAGE = 0x02;

void setup(void) {
  Serial.begin(9600);

  lcd.begin(20, 4);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.write("  Initializing...   ");
  delay(200);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("Testscreen 0");
  lcd.setCursor(0, 1);
  lcd.write("Testscreen 1");
  lcd.setCursor(0, 2);
  lcd.write("Testscreen 2");
  lcd.setCursor(0, 3);
  lcd.write("Testscreen 3");
}

void process_command(byte cmd)
{
  switch (cmd) {
  case CMD_CLEAR: {
    lcd.clear();
    break;
  }
  case CMD_WRITE_RAW: {
    byte length = 0x00;
    Serial.readBytes((char*)&length, 1);
    byte ch;
    for (int i = 0; i < length; i++) {
      Serial.readBytes((char*)&ch, 1);
      lcd.write(ch);
    }
    break;
  }
  case CMD_WRITE_PAGE: {
    lcd.setCursor(0, 0);
    byte length = 0x00;
    Serial.readBytes((char*)&length, 1);
    byte ch;
    int line = -1;
    for (int i = 0; i < length; i++) {
      if (i % 20 == 0) {
        line += 1;
        if (line >= 4) {
          line = 0;
        }
        lcd.setCursor(0, line);
      }
      Serial.readBytes((char*)&ch, 1);
      lcd.write(ch);
    }

    break;
  }
  default:
    break;
  };
}

void process_io(void) {
  while (Serial.available() >= 1) {
    byte command = Serial.read();
    /*Serial.print("Received: ");
    Serial.println(command, DEC);*/
    process_command(command);
  }
}

void io_aware_delay(uint16_t ms) {
	uint16_t start = (uint16_t)micros();

	while (ms > 0) {
    process_io();
		if (((uint16_t)micros() - start) >= 1000) {
			ms--;
			start += 1000;
		}
	}

}

void loop(void) {
  byte i;
  //byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];

  byte sensor_no = 0;

  while (ds.search(addr)) {
    if (OneWire::crc8(addr, 7) != addr[7]) {
      process_io();
      continue;
    }

    // the first ROM byte indicates which chip
    switch (addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      continue;
    }

    ds.reset();
    process_io();
    ds.select(addr);
    process_io();
    ds.write(0x44, 1);
    io_aware_delay(1000);

    ds.reset();
    process_io();
    ds.select(addr);
    process_io();
    ds.write(0xBE);
    process_io();

    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
      process_io();
    }


    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } else {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }

    Serial.write(sensor_no);
    process_io();
    Serial.write(addr, sizeof(addr));
    process_io();
    Serial.write((const uint8_t*)&raw, sizeof(raw));
    process_io();

    sensor_no += 1;
    io_aware_delay(250);
  }
  process_io();
}
