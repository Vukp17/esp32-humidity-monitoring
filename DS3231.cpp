#include "DS3231.h"
#include <Wire.h>

const char DaysOfTheWeek[NUMBER_OF_DAYS][10] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// all of the numbers in the DS3231 registers are stored as binary coded decimals (BCD) so we need convertion between the two

unsigned char BCDtoInt(unsigned char BCD) {
  unsigned char a, b;

  a = BCD & 0x0F;
  b = BCD >> 4;

  return b * 10 + a;
}

unsigned char InttoBCD(unsigned char Int) {
  unsigned char a, b;

  a = Int % 10;
  b = (Int / 10) % 10;

  return (b << 4) + a;
}
//   pressureTemperaturePair DS3231::readBoth() {
//    int* result = doRead(7, true, 0x2E);
//     int temp_low_bit = result[0];
//     int temp_high_bit = result[1];
//     int pres_low_bit = result[2];
//     int pres_high_bit = result[3];

//     pressureTemperaturePair pair = {
//        // calculatePressure(pres_low_bit, pres_high_bit),
//         calculateTemperature(temp_low_bit, temp_high_bit)
//     };

//     return pair;
// }
// void DS3231::writer(commandSequence seq) {
//     // this->multiplex_switch();
//     Wire.beginTransmission(SM9333_UNPROTECTED);
//     for (int i = 0; i < seq.length; i++) {
//         int val = seq.sequence[i];
//         // Wire.write((byte)val);
//         Wire.write(val);
//     }
//     Wire.endTransmission();
// }
// int* DS3231::doRead(int numBits, bool crcProtected, int location) {
//     // this->multiplex_switch();
//     int crcLoc = crcProtected ? SM9333_CRC_PROTECTED : SM9333_UNPROTECTED;
//     int command[3] = {location, 0x5B, 0xDB};
//     commandSequence seq = {command, 3};
//     writer(seq);
//     Wire.requestFrom(crcLoc, numBits);

//     // int result[numBits];
//     int* result;
//     for (int i = 0; i < numBits; i++) {
//         *(result+i) = Wire.read();
//     }
//     return result;
// }

void I2C_SendDataToSlave(unsigned char Address, unsigned char Register, unsigned char Data) {
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.write(Data);
  Wire.endTransmission();
}

float I2C_ReadDataFromSlave(unsigned char Address, unsigned char Register) {
  unsigned char lowByte, highByte;
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.endTransmission();

  Wire.requestFrom(Address, 2);

  if (Wire.available() == 2) {
    lowByte = Wire.read();
    highByte = Wire.read();

    int16_t data = lowByte | (highByte << 8);

    float temperature = static_cast<float>(data + 16881) / 397.2;

    return temperature;
  }

  return 0.0;
}
float I2C_ReadDataFromSlavePressure(unsigned char Address, unsigned char Register) {
  unsigned char lowByte, highByte;
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.endTransmission();

  Wire.requestFrom(Address, 3);

  if (Wire.available() == 3) {  // Changed condition to check for 3 bytes
    lowByte = Wire.read();
    highByte = Wire.read();
    int16_t data = (lowByte << 8) | (highByte << 16);

    double p_pc = 100.0 / (pow(2, 16) - 1) * double(data) + 100.0 * pow(2, 16) / (2.0 * (pow(2, 16) - 1));

    // Assuming P_MIN and P_MAX are defined somewhere in your code


    double p_p = (P_MIN - P_MAX) / 80.0 * p_pc - P_MIN - (P_MIN - P_MAX) / 8.0;

    return p_p;
  }
}

float I2C_ReadDataFromSlaveHumidity(unsigned char Address, unsigned char Register) {
  unsigned char lowByte, highByte;
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.endTransmission();

  delay(500);

  Wire.requestFrom(Address, 2);

  if (Wire.available() == 2) {
    lowByte = Wire.read();
    highByte = Wire.read();
    int16_t data = (highByte << 8) | lowByte;

    // Convert the data to humidity
    float humidity = static_cast<float>(-600 + 12500 * (data / 65536));  // Assuming 0.0 is the offset, adjust accordingly

    return humidity;
  }

  return 0.0;  // Return a default value if data is not available
}


// float  I2C_ReadDataFromSlavePresure(unsigned char Address, unsigned char Register)
// {
// 	unsigned char Data;
// 	Wire.beginTransmission (Address);
// 	Wire.write (Register);
// 	Wire.endTransmission ();

// 	Wire.requestFrom (Address, 7);
//     if (Wire.available() == 2) {
//         int16_t rawTemp = Wire.read() << 8 | Wire.read();
//        // float temperature = static_cast<float>(rawTemp) / 100.0;
//         int8_t highByte = (rawTemp >> 8) & 0xFF;
//         float temperature = static_cast<float>(highByte + 16881) / 397.2; //za prikaz v temperaturah v chip temp
//         //float temperature = static_cast<float>(rawTemp) / 397.2;

//         return temperature;
//     }

//     return 0.0;
// }


DS3231::DS3231() {
  Wire.begin();
}

void DS3231::UpdateData() {
  I2C_SendDataToSlave(DS3231_ADDRESS, CTRL_REGISTER, 0x3C);  // Control register -> Convert temperature enabled (this bit has to be raised constantly), 8.192 kHz

  for (int i = 0; i < NUMBER_OF_REGS; i++)
    Read_Buff[i] = I2C_ReadDataFromSlave(DS3231_ADDRESS, i);

  Year = GetYear();
  Month = GetMonth();
  Date = GetDate();
  Day = GetDayOfTheWeek();
  Hours = GetHours();
  Minutes = GetMinutes();
  Seconds = GetSeconds();
  Temp = GetTemperature();
}

void DS3231::SetTime(int Hours, int Minutes, int Seconds) {
  Hours = InttoBCD(Hours);
  Minutes = InttoBCD(Minutes);
  Seconds = InttoBCD(Seconds);

  I2C_SendDataToSlave(DS3231_ADDRESS, SECONDS, Seconds);  // seconds
  I2C_SendDataToSlave(DS3231_ADDRESS, MINUTES, Minutes);  // minutes
  I2C_SendDataToSlave(DS3231_ADDRESS, HOURS, Hours);      // hours
}

void DS3231::SetDate(int Year, int Month, int Date, int DayOfTheWeek) {
  Year = InttoBCD(Year);
  Month = InttoBCD(Month);
  Date = InttoBCD(Date);
  DayOfTheWeek = InttoBCD(DayOfTheWeek);

  I2C_SendDataToSlave(DS3231_ADDRESS, DAY_OF_WEEK, DayOfTheWeek);  // day of the week
  I2C_SendDataToSlave(DS3231_ADDRESS, DATE, Date);                 // date
  I2C_SendDataToSlave(DS3231_ADDRESS, MONTH, Month);               // month
  I2C_SendDataToSlave(DS3231_ADDRESS, YEAR, Year);                 // year (last 2 digits)
}

void DS3231::SetAlarm1(int Hours, int Minutes, int Seconds, int Day_Date, int Setting) {
  Day_Date = InttoBCD(Day_Date) | (((Setting >> 3) & 0x01) << 7) | (Setting & 0x40);  // A1M4 + DY/!DT
  Hours = InttoBCD(Hours) | (((Setting >> 2) & 0x01) << 7);                           // A1M3
  Minutes = InttoBCD(Minutes) | (((Setting >> 1) & 0x01) << 7);                       // A1M2
  Seconds = InttoBCD(Seconds) | (((Setting >> 0) & 0x01) << 7);                       // A1M1

  I2C_SendDataToSlave(DS3231_ADDRESS, ALARM1_SECONDS, Seconds);    // seconds
  I2C_SendDataToSlave(DS3231_ADDRESS, ALARM1_MINUTES, Minutes);    // minutes
  I2C_SendDataToSlave(DS3231_ADDRESS, ALARM1_HOURS, Hours);        // hours
  I2C_SendDataToSlave(DS3231_ADDRESS, ALARM1_DAY_DATE, Day_Date);  // day of the week/date of the month
}

void DS3231::SetAlarm2(int Hours, int Minutes, int Day_Date, int Setting) {
  Day_Date = InttoBCD(Day_Date) | (((Setting >> 3) & 0x01) << 7) | (Setting & 0x40);  // A2M4 + DY/!DT
  Hours = InttoBCD(Hours) | (((Setting >> 2) & 0x01) << 7);                           // A2M3
  Minutes = InttoBCD(Minutes) | (((Setting >> 1) & 0x01) << 7);                       // A2M2

  I2C_SendDataToSlave(DS3231_ADDRESS, ALARM2_MINUTES, Minutes);    // minutes
  I2C_SendDataToSlave(DS3231_ADDRESS, ALARM2_HOURS, Hours);        // hours
  I2C_SendDataToSlave(DS3231_ADDRESS, ALARM2_DAY_DATE, Day_Date);  // day of the week/date of the month
}

int DS3231::CheckCentury() {
  // bit 7 of the month register (address 0x05) store new century occurrence
  if (Read_Buff[MONTH] & 0x80) {
    I2C_SendDataToSlave(DS3231_ADDRESS, MONTH, Read_Buff[MONTH] & 0x7F);  // clearing the new century flag
    return 1;
  }
  return 0;
}

int DS3231::CheckAlarm1() {
  if (Read_Buff[STAT_REGISTER] & 0x01) {
    I2C_SendDataToSlave(DS3231_ADDRESS, STAT_REGISTER, Read_Buff[STAT_REGISTER] & (~0x01));  // clearing the Alarm1 flag
    return 1;
  }
  return 0;
}

int DS3231::CheckAlarm2() {
  if (Read_Buff[STAT_REGISTER] & 0x02) {
    I2C_SendDataToSlave(DS3231_ADDRESS, STAT_REGISTER, Read_Buff[STAT_REGISTER] & (~0x02));  // clearing the Alarm2 flag
    return 1;
  }
  return 0;
}

int DS3231::GetYear() {
  return BCDtoInt(Read_Buff[YEAR] & YEAR_MASK);
}

int DS3231::GetMonth() {
  return BCDtoInt(Read_Buff[MONTH] & MONTH_MASK);
}

int DS3231::GetDate() {
  return BCDtoInt(Read_Buff[DATE] & DATE_MASK);
}

int DS3231::GetDayOfTheWeek() {
  return BCDtoInt(Read_Buff[DAY_OF_WEEK] & DOTW_MASK);
}

int DS3231::GetHours() {
  return BCDtoInt(Read_Buff[HOURS] & HOURS_MASK);
}

int DS3231::GetMinutes() {
  return BCDtoInt(Read_Buff[MINUTES] & MINUTES_MASK);
}

int DS3231::GetSeconds() {
  return BCDtoInt(Read_Buff[SECONDS] & SECONDS_MASK);
}

double DS3231::GetTemperature() {
  return (double)(((signed int)((Read_Buff[TEMPERATURE_H] << 8) | Read_Buff[TEMPERATURE_L])) >> 6) / 4.0;
  ;
}