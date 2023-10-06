//================================================================================ //
// Reading sensor data from DHT22 without using a library
//================================================================================ //

#define F_CPU 16000000 UL

#include <util/delay.h>

#define DHT_PIN PH4

//============== Macro definitions for working with bits ======================== // 
#define set_bit(y, bit)(y |= (1 << bit))    // sets the x bit of the variable Y to 1
#define clear_bit(y, bit)(y &= ~(1 << bit)) // sets bit x of variable Y to 0
#define toggle_bit(y, bit)(y ^= (1 << bit)) // changes the logical state of bit x of variable Y
#define test_bit(y, bit)(y & (1 << bit))    // returns 0 or 1 depending on the bit reading
//================================================================================ //

float sensor_humidity = 0.0, sensor_temperature = 0.0;
uint8_t sensor_checksum;

void signal_sensor()
{
  set_bit(DDRH, DHT_PIN);    // set data pin for o/p
  clear_bit(PORTH, DHT_PIN); // first send low pulse
  
  _delay_us(1000);
  
  set_bit(PORTH, DHT_PIN);   // send high pulse
  
  _delay_us(40);
}

void response_signal()
{
  clear_bit(DDRH, DHT_PIN);        // set data pin for i/p
  while(test_bit(PINH, DHT_PIN));  // wait for low pulse
  while(!test_bit(PINH, DHT_PIN)); // wait for high pulse
  while(test_bit(PINH, DHT_PIN));  // wait for low pulse
}

void show_sensor_temperature(){
  Serial.print("Temperature: ");
  Serial.print(sensor_temperature);
  Serial.println("C");
}

void show_sensor_humidity(){
  Serial.print("Humidity: ");
  Serial.print(sensor_humidity);
  Serial.println("%");
}

void show_sensor_checksum(uint8_t RH_high, uint8_t RH_low, uint8_t temp_high, uint8_t temp_low){
  uint8_t sum = RH_high + RH_low + temp_high + temp_low;
  Serial.print("Checksum: ");
  if (sensor_checksum != sum) { 
    Serial.print("ERROR!!!"); 
  } else { 
    Serial.println("OK!"); 
  }
}

void read_sensor_data()
{
  uint8_t RH_high, RH_low, temp_high, temp_low;
  
  signal_sensor();
  response_signal();
  
  RH_high = read_DHT22_byte();
  RH_low = read_DHT22_byte(); 
  temp_high = read_DHT22_byte();
  temp_low = read_DHT22_byte();
  sensor_checksum = read_DHT22_byte();
  
  bool temp_is_negative = temp_high & 0x80; // check sign bit
  
  RH_high &= 0x7F; temp_high &= 0x7F; sensor_checksum &= 0x7F; // these bits are always zero, masking them reduces errors.

  sensor_humidity = ((RH_high << 8) | RH_low) * 0.1;
  sensor_temperature = ((temp_high << 8) | temp_low)* 0.1;

  if (temp_is_negative) sensor_temperature = -sensor_temperature;
 
  show_sensor_temperature();
  show_sensor_humidity();
  show_sensor_checksum(RH_high, RH_low, temp_high, temp_low);

  Serial.println(" ");

  _delay_ms(1000);
}

byte read_DHT22_byte()
{
  byte dByte = 0b00000000;
  for(byte i=0; i<8; i++)
  {
    while(!test_bit(PINH, DHT_PIN)); // detect data bit (high pulse)
    _delay_us(50);
    
    if(test_bit(PINH, DHT_PIN)){
      dByte = (dByte<<1)|(0x01);
    }else{
      dByte = (dByte<<1);           // store 1 or 0 in dByte
    }
    
    while(test_bit(PINH, DHT_PIN)); // wait for DHT22 low pulse
  }
  return dByte;
}

int main(void)
{
  Serial.begin(9600);
  while(1){read_sensor_data();}
  Serial.end();
}