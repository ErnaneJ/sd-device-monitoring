//================================================================================ //
// Reading sensor data from DHT22 without using a library and show in display 16x2
//================================================================================ //

#define F_CPU 16000000 UL

#include <avr/io.h>
#include <util/delay.h>

//============== Macro definitions for working with bits ======================== // 
#define set_bit(y, bit)(y |= (1 << bit))    // sets the x bit of the variable Y to 1
#define clear_bit(y, bit)(y &= ~(1 << bit)) // sets bit x of variable Y to 0
#define toggle_bit(y, bit)(y ^= (1 << bit)) // changes the logical state of bit x of variable Y
#define test_bit(y, bit)(y & (1 << bit))    // returns 0 or 1 depending on the bit reading
//================================================================================ //

//====================================== START - PROJECT SETTINGS ===================================== //

int SAMPPLING            =     1;
int CURRENT_SCREEN       =     0; // default screen 
int TEMP_UNIT_OPTION     =     0; // 0 = C ; 1 = F ; 2 = K

float MAX_TEMP           =  80.0;
float MIN_TEMP           = -40.0;
float MAX_HUM            = 100.0;
float MIN_HUM            =   0.0;
float SMOKE_INDEX        =     0;
float sensor_humidity    =   0.0; 
float sensor_temperature =   0.0;

uint8_t sensor_checksum  =   0.0;

#define SCREEN_HOME            0
#define SCREEN_SETUP           1
#define SCREEN_TEMP            2
#define SCREEN_MIN_TEMP        3
#define SCREEN_MAX_TEMP        4
#define SCREEN_HUM             5
#define SCREEN_MIN_HUM         6
#define SCREEN_MAX_HUM         7
#define SCREEN_TUNIT           8
#define SCREEN_SAMPPLING       9
#define SCREEN_WARN_SMOKE      10
#define SCREEN_WARN_LIMIT     11

#define DEBUG_SENSOR           0

#define TEMP_CELSIUS           0
#define TEMP_FAHRENHEIT        1
#define TEMP_KELVIN            2

#define LIMIT_SMOKE_INDEX     50

#define DHT_PIN              PH4
#define LED_GREEN_PIN        PE4
#define LED_YELLOW_PIN       PE5
#define LED_RED_PIN          PG5
#define DADOS_LCD          PORTA
#define NIBBLE_DATA            1 // (PA4-D4, PA5-D5, PA6-D6, PA7-D7)
#define CONTR_LCD          PORTH
#define E                    PH6
#define RS                   PH5

#define LCD_INSTRUCTION        0
#define LCD_CHARACTER          1

//====================================== END - PROJECT SETTINGS ===================================== //
//====================================== START - Code For Sensor ===================================== //

void signal_sensor_dht()
{
  set_bit(DDRH, DHT_PIN);    // set data pin for o/p
  clear_bit(PORTH, DHT_PIN); // first send low pulse
  
  _delay_us(1000);
  
  set_bit(PORTH, DHT_PIN);   // send high pulse
  
  _delay_us(40);
}

void response_dht_signal()
{
  clear_bit(DDRH, DHT_PIN);        // set data pin for i/p
  while(test_bit(PINH, DHT_PIN));  // wait for low pulse
  while(!test_bit(PINH, DHT_PIN)); // wait for high pulse
  while(test_bit(PINH, DHT_PIN));  // wait for low pulse
}

void read_dht_sensor_data()
{
  turn_on_yellow_led();
  uint8_t RH_high, RH_low, temp_high, temp_low;
  
  signal_sensor_dht();
  response_dht_signal();
  
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

  check_sensor_informations();
 
  #if DEBUG_SENSOR == 1
    show_sensor_temperature();
    show_sensor_humidity();
    show_sensor_checksum(RH_high, RH_low, temp_high, temp_low);
    Serial.println(" ");
  #endif

  _delay_ms(1000);

  if(CURRENT_SCREEN != SCREEN_WARN_SMOKE && CURRENT_SCREEN != SCREEN_WARN_LIMIT){
    turn_on_green_led();
  }
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

void show_sensor_temperature()
{
  Serial.print("Temperature: ");
  Serial.print(sensor_temperature);
  Serial.println("C");
}

void show_sensor_humidity()
{
  Serial.print("Humidity: ");
  Serial.print(sensor_humidity);
  Serial.println("%");
}

void show_sensor_checksum(uint8_t RH_high, uint8_t RH_low, uint8_t temp_high, uint8_t temp_low)
{
  uint8_t sum = RH_high + RH_low + temp_high + temp_low;
  Serial.print("Checksum: ");
  if (sensor_checksum != sum) { 
    Serial.print("ERROR!!!"); 
  } else { 
    Serial.println("OK!"); 
  }
}

void check_sensor_informations()
{
  if(
    (sensor_humidity < MIN_HUM || sensor_humidity > MAX_HUM ||
    sensor_temperature < MIN_TEMP || sensor_temperature > MAX_TEMP) 
    && (CURRENT_SCREEN == SCREEN_HOME || CURRENT_SCREEN == SCREEN_WARN_LIMIT)
  ) {
    CURRENT_SCREEN = SCREEN_WARN_LIMIT;
    update_screen();
  }else if(CURRENT_SCREEN == SCREEN_WARN_LIMIT){
    CURRENT_SCREEN = SCREEN_HOME;
    update_screen();
  }
}

//====================================== END - Code For Sensor ===================================== //
//====================================== START - Code For LCD ===================================== //

void pulso_enable()
{
  _delay_us(1);
  
  set_bit(CONTR_LCD, E);
  _delay_us(1);
  
  clear_bit(CONTR_LCD, E);
  _delay_us(45);
}

void send_command_to_LCD(unsigned char c, char type) 
{
  if (type == LCD_INSTRUCTION) clear_bit(CONTR_LCD, RS); // command is instruction
  if (type == LCD_CHARACTER) set_bit(CONTR_LCD, RS);     // command is character

  // ============== First data nibble - 4 MSB
  if(NIBBLE_DATA){
    DADOS_LCD = (DADOS_LCD & 0x0F) | (0xF0 & c);
  }else{
    DADOS_LCD = (DADOS_LCD & 0xF0) | (c >> 4);
  }

  pulso_enable();

  // ============== Second data nibble - 4 LSB
  if(NIBBLE_DATA){
    DADOS_LCD = (DADOS_LCD & 0x0F) | (0xF0 & (c << 4));
  }else{
    DADOS_LCD = (DADOS_LCD & 0xF0) | (0x0F & c);
  }

  pulso_enable();

  // if it is a return or cleaning instruction, wait for the LCD to be ready
  if ((type == LCD_INSTRUCTION) && (c < 4)) _delay_ms(2);
}

void initialize_LCD()
{
  clear_bit(CONTR_LCD, RS);
  clear_bit(CONTR_LCD, E);
  _delay_ms(20);

  // ============== First data nibble - 4 MSB
  if(NIBBLE_DATA){
    DADOS_LCD = (DADOS_LCD & 0x0F) | 0x30;
  }else{
    DADOS_LCD = (DADOS_LCD & 0xF0) | 0x03;
  }

  pulso_enable(); _delay_ms(5); pulso_enable(); _delay_us(200); pulso_enable(); 

  // ============== Second data nibble - 4 MSB
  if(NIBBLE_DATA){
    DADOS_LCD = (DADOS_LCD & 0x0F) | 0x20;
  }else{
    DADOS_LCD = (DADOS_LCD & 0xF0) | 0x02;
  }

  pulso_enable(); send_command_to_LCD(0x28, 0);

  send_command_to_LCD(0x08, 0); // turn off the display
  send_command_to_LCD(0x01, 0); // cleans the entire display
  send_command_to_LCD(0x0C, 0); // apparent message inactive cursor not blinking
  send_command_to_LCD(0x80, 0); // initializes cursor in the first position on the left - 1st line
}

void writes_on_LCD(char * c)
{ 
  for (;* c != 0; c++) send_command_to_LCD( * c, 1); 
}

void show_temperature_on_LCD()
{
  float aux_temp = sensor_temperature;
  char unit[2]; unit[0] = 'C'; unit[1] = '\0';
  
  if(TEMP_UNIT_OPTION == TEMP_FAHRENHEIT){
    aux_temp = (sensor_temperature * (9.0/5.0)) + 32;
    unit[0] = 'F';
  }else if (TEMP_UNIT_OPTION == TEMP_KELVIN){
    aux_temp = sensor_temperature + 273.15;
    unit[0] = 'K';
  }

  char str_temperature[20];
  dtostrf(aux_temp, 6, 2, str_temperature);
  
  writes_on_LCD("Temp:    ");
  writes_on_LCD(str_temperature);
  writes_on_LCD(unit);
}

void show_humidity_on_LCD()
{
  char str_humidity[20];
  dtostrf(sensor_humidity, 6, 2, str_humidity);
  
  writes_on_LCD("Hum:     ");
  writes_on_LCD(str_humidity);
  writes_on_LCD("%");
}

//====================================== END - Code For LCD ===================================== //
//====================================== START - BUTTONS ===================================== //
void setup_buttons()
{
  // CENTER BUTTON - INT0
  clear_bit(DDRD, PD0);
  set_bit(PORTD, PD0);
  set_bit(EIMSK, INT0); // enabble interruption with int0

  // RIGHT BUTTON - INT1
  clear_bit(DDRD, PD1);
  set_bit(PORTD, PD1);
  set_bit(EIMSK, INT1); // enabble interruption with int1

  // LEFT BUTTON - INT2
  clear_bit(DDRD, PD2);
  set_bit(PORTD, PD2);
  set_bit(EIMSK, INT2); // enabble interruption with int2

  // ISCn0 = 0 and ISCn0 = 0 => The low level of INTn generates an interrupt request.
  // set_bit(EICRA, ISC00);
  // set_bit(EICRA, ISC10);
  // set_bit(EICRA, ISC20);

  sei();
}
//====================================== END - BUTTONS ===================================== //
//====================================== START - SCREENS ===================================== //

void show_SCREEN_HOME()
{
  send_command_to_LCD(0X01, 0); // clear display
  send_command_to_LCD(0X80, 0);
  show_temperature_on_LCD();
  send_command_to_LCD(0XC0, 0);
  show_humidity_on_LCD();
}

void show_SCREEN_SETUP()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  writes_on_LCD("     SETUP");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("TEMP  HUM   SAMP");
}

void show_SCREEN_TEMP()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  writes_on_LCD("  TEMPERATURE");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("MIN  T.UNIT  MAX");
}

void show_SCREEN_MIN_TEMP()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  float aux_temp = MIN_TEMP;
  char unit[2]; unit[0] = 'C'; unit[1] = '\0';
  
  if(TEMP_UNIT_OPTION == TEMP_FAHRENHEIT){
    aux_temp = (MIN_TEMP * (9.0/5.0)) + 32;
    unit[0] = 'F';
  }else if (TEMP_UNIT_OPTION == TEMP_KELVIN){
    aux_temp = MIN_TEMP + 273.15;
    unit[0] = 'K';
  }

  char str_temperature[20];
  dtostrf(aux_temp, 6, 2, str_temperature);
  
  writes_on_LCD("  MIN TEMP");
  writes_on_LCD(" (");
  writes_on_LCD(unit);
  writes_on_LCD(")");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("  + (");
  writes_on_LCD(str_temperature);
  writes_on_LCD(") -");
}

void show_SCREEN_MAX_TEMP()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  float aux_temp = MAX_TEMP;
  char unit[2]; unit[0] = 'C'; unit[1] = '\0';
  
  if(TEMP_UNIT_OPTION == TEMP_FAHRENHEIT){
    aux_temp = (MAX_TEMP * (9.0/5.0)) + 32;
    unit[0] = 'F';
  }else if (TEMP_UNIT_OPTION == TEMP_KELVIN){
    aux_temp = MAX_TEMP + 273.15;
    unit[0] = 'K';
  }

  char str_temperature[20];
  dtostrf(aux_temp, 6, 2, str_temperature);
  
  writes_on_LCD("  MAX TEMP");
  writes_on_LCD(" (");
  writes_on_LCD(unit);
  writes_on_LCD(")");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("  + (");
  writes_on_LCD(str_temperature);
  writes_on_LCD(") -");
}

void show_SCREEN_HUM()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  writes_on_LCD("    HUMIDITY");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD(" MIN        MAX");
}

void show_SCREEN_MIN_HUM()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  char str_hum[20];
  dtostrf(MIN_HUM, 6, 2, str_hum);
  
  writes_on_LCD("  MIN HUM");
  writes_on_LCD(" (%)");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("  + (");
  writes_on_LCD(str_hum);
  writes_on_LCD(") -");
}

void show_SCREEN_MAX_HUM()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  char str_hum[20];
  dtostrf(MAX_HUM, 6, 2, str_hum);
  
  writes_on_LCD("  MAX HUM");
  writes_on_LCD(" (%)");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("  + (");
  writes_on_LCD(str_hum);
  writes_on_LCD(") -");
}

void show_SCREEN_TUNIT()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  writes_on_LCD("   TEMP UNIT");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("(F)    (C)   (K)");
}

void show_SCREEN_SAMPPLING()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  char str_samp[20];
  dtostrf(SAMPPLING, 1,0, str_samp);
  
  writes_on_LCD("   SAMPPLING");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("  +  (");
  writes_on_LCD(str_samp);
  writes_on_LCD("s)  -");
}

void show_SCREEN_WARN_SMOKE()
{
  turn_on_red_led();
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  char str_smoke[20];
  dtostrf(SMOKE_INDEX, 6, 2, str_smoke);
  
  writes_on_LCD(" SMOKE DETECTED");
  send_command_to_LCD(0XC0, 0);

  writes_on_LCD("    ");
  writes_on_LCD(str_smoke);
  writes_on_LCD("%");
}

void show_SCREEN_WARN_LIMIT()
{ 
  float aux_temp = sensor_temperature;
  char unit[2]; unit[0] = 'C'; unit[1] = '\0';
  
  if(TEMP_UNIT_OPTION == TEMP_FAHRENHEIT){
    aux_temp = (sensor_temperature * (9.0/5.0)) + 32;
    unit[0] = 'F';
  }else if (TEMP_UNIT_OPTION == TEMP_KELVIN){
    aux_temp = sensor_temperature + 273.15;
    unit[0] = 'K';
  }

  char str_temperature[20];
  dtostrf(aux_temp, 4, 1, str_temperature);

  char str_humidity[20];
  dtostrf(sensor_humidity, 3, 1, str_humidity);

  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);
  writes_on_LCD(" LIMIT EXCEEDED");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("T:");
  writes_on_LCD(str_temperature);
  writes_on_LCD(unit);
  writes_on_LCD(" H: ");
  writes_on_LCD(str_humidity);
  writes_on_LCD("%");

  _delay_ms(1000);
  turn_on_red_led();
}

void show_SCREEN_START()
{
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  writes_on_LCD("  ERNANE TECH");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("SOLUTIONS .LTDA");
}

void start_project()
{
  DDRH = 0xFF; DDRA = 0xFF; DDRB = 0xFF; // A, B and H registers are digital outputs by default.

  initialize_LCD();
  show_SCREEN_START();
  
  leds_setup();
  turn_on_all_leds();
  
  smoke_sensor_setup();
  signal_sensor_dht();
  
  setup_buttons();
  
  turn_on_green_led();

  _delay_ms(2000);

  update_screen();
}

void show_error()
{
  turn_on_red_led();
  send_command_to_LCD(0X01, 0);
  send_command_to_LCD(0X80, 0);

  writes_on_LCD("===== ERROR ====");
  send_command_to_LCD(0XC0, 0);
  writes_on_LCD("===== ERROR ====");
}

void update_screen()
{
  switch(CURRENT_SCREEN){
    case SCREEN_HOME:
      show_SCREEN_HOME();
      break;
    case SCREEN_SETUP:
      show_SCREEN_SETUP();
      break;
    case SCREEN_TUNIT:
      show_SCREEN_TUNIT();
      break;
    case SCREEN_TEMP:
      show_SCREEN_TEMP();
      break;
    case SCREEN_MIN_TEMP:
      show_SCREEN_MIN_TEMP();
      break;
    case SCREEN_MAX_TEMP:
      show_SCREEN_MAX_TEMP();
      break;
    case SCREEN_HUM:
      show_SCREEN_HUM();
      break;
    case SCREEN_MIN_HUM:
      show_SCREEN_MIN_HUM();
      break;
    case SCREEN_MAX_HUM:
      show_SCREEN_MAX_HUM();
      break;
    case SCREEN_SAMPPLING:
      show_SCREEN_SAMPPLING();
      break;
    case SCREEN_WARN_SMOKE:
      show_SCREEN_WARN_SMOKE();
      break;
    case SCREEN_WARN_LIMIT:
      show_SCREEN_WARN_LIMIT();
      break;
    default:
      show_error();
      break;
  }
}

//====================================== END - SCREENS ============================================== //
//====================================== START - SMOKE DETECTOR ===================================== //

void smoke_sensor_setup()
{
  ADMUX = 0; // 0x40;
  ADCSRA = (1 << ADEN);

  clear_bit(DDRH, PH3);
}

void read_smoke_sensor()
{
  ADMUX = (ADMUX & 0xF8);
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  SMOKE_INDEX = (ADC / 1023.0) * 100.0;

  if(SMOKE_INDEX >= LIMIT_SMOKE_INDEX){
    CURRENT_SCREEN = SCREEN_WARN_SMOKE;
    update_screen();
    set_bit(PINH, PH3);
    tone(6, 100); // for simulation
  }else{
    clear_bit(PINH, PH3);
    noTone(6); // for simulation
    if(CURRENT_SCREEN == SCREEN_WARN_SMOKE) CURRENT_SCREEN = SCREEN_HOME;
  }
}

//====================================== END - SMOKE DETECTOR ===================================== //

//====================================== START - LEDS ===================================== //

void leds_setup()
{
  clear_bit(DDRE, LED_GREEN_PIN);
  clear_bit(DDRE, LED_YELLOW_PIN);
  clear_bit(DDRG, LED_RED_PIN);
}

void turn_on_green_led()
{
  set_bit(PORTE, LED_GREEN_PIN);
  clear_bit(PORTE, LED_YELLOW_PIN);
  clear_bit(PORTG, LED_RED_PIN);
}

void turn_on_yellow_led()
{
  clear_bit(PORTE, LED_GREEN_PIN);
  set_bit(PORTE, LED_YELLOW_PIN);
  clear_bit(PORTG, LED_RED_PIN);
}

void turn_on_red_led()
{
  clear_bit(PORTE, LED_GREEN_PIN);
  clear_bit(PORTE, LED_YELLOW_PIN);
  set_bit(PORTG, LED_RED_PIN);
}

void turn_on_all_leds()
{
  set_bit(PORTE, LED_GREEN_PIN);
  set_bit(PORTE, LED_YELLOW_PIN);
  set_bit(PORTG, LED_RED_PIN);
}

void turn_off_all_leds()
{
  clear_bit(PORTE, LED_GREEN_PIN);
  clear_bit(PORTE, LED_YELLOW_PIN);
  clear_bit(PORTG, LED_RED_PIN);
}

//====================================== END - LEDS ===================================== //
//====================================== START - Main method ============================================ //

int main(void)
{
  #if DEBUG_SENSOR == 1
    Serial.begin(9600);
  #endif

  start_project();
  
  while(1){
    read_dht_sensor_data();
    read_smoke_sensor();

    for(int i = 0; i < SAMPPLING; i++){ _delay_ms(1000); }
    if(CURRENT_SCREEN == SCREEN_HOME) update_screen();
  }

  #if DEBUG_SENSOR == 1
    Serial.end();
  #endif

  return 0;
}

//====================================== END - Main method ============================================ //
//====================================== Start - Interruptions ============================================ //

ISR(INT2_vect)
{ // LEFT
  _delay_ms(100);

  switch (CURRENT_SCREEN) {
    case SCREEN_TUNIT:
      if (!test_bit(PIND, PD2)) return;
      TEMP_UNIT_OPTION = TEMP_FAHRENHEIT;
      CURRENT_SCREEN = SCREEN_HOME;
      break;
    case SCREEN_SETUP:
      if (!test_bit(PIND, PD2)) return;
      CURRENT_SCREEN = SCREEN_TEMP;
      break;
    case SCREEN_TEMP:
      if (!test_bit(PIND, PD2)) return;
      CURRENT_SCREEN = SCREEN_MIN_TEMP;
      break;
    case SCREEN_MIN_TEMP:
      if (MIN_TEMP < 80.0) MIN_TEMP += 0.1;
      break;
    case SCREEN_MAX_TEMP:
      if (MAX_TEMP < 80.0) MAX_TEMP += 0.1;
      break;
    case SCREEN_HUM:
      if (!test_bit(PIND, PD2)) return;
      CURRENT_SCREEN = SCREEN_MIN_HUM;
      break;
    case SCREEN_MIN_HUM:
      if (MIN_HUM < 100.00) MIN_HUM += 0.1;
      break;
    case SCREEN_MAX_HUM:
      if (MAX_HUM < 100.00) MAX_HUM += 0.1;
      break;
    case SCREEN_SAMPPLING:
      SAMPPLING += 1;
      break;
    case SCREEN_WARN_SMOKE:
    case SCREEN_WARN_LIMIT:
      CURRENT_SCREEN = SCREEN_HOME;
      break;
    default:
      break;
  }

  update_screen();
  _delay_ms(100);
}

ISR(INT0_vect)
{ // CENTER
  _delay_ms(100);

  if (!test_bit(PIND, PD0)) return; // Botão pressionado

  switch (CURRENT_SCREEN) {
    case SCREEN_HOME:
      CURRENT_SCREEN = SCREEN_SETUP;
      break;
    case SCREEN_TUNIT:
      TEMP_UNIT_OPTION = TEMP_CELSIUS;
      CURRENT_SCREEN = SCREEN_HOME;
      break;
    case SCREEN_TEMP:
      CURRENT_SCREEN = SCREEN_TUNIT;
      break;
    case SCREEN_MIN_TEMP:
    case SCREEN_MAX_TEMP:
    case SCREEN_MIN_HUM:
    case SCREEN_MAX_HUM:
    case SCREEN_SAMPPLING:
    case SCREEN_HUM:
    case SCREEN_WARN_SMOKE:
    case SCREEN_WARN_LIMIT:
      CURRENT_SCREEN = SCREEN_HOME;
      break;
    case SCREEN_SETUP:
      CURRENT_SCREEN = SCREEN_HUM;
      break;
    default:
      break;
  }

  update_screen();
  _delay_ms(100);
}

ISR(INT1_vect)
{ // RIGHT
  _delay_ms(100);

  switch (CURRENT_SCREEN) {
    case SCREEN_SETUP:
      if (!test_bit(PIND, PD1)) return; // Botão pressionado
      CURRENT_SCREEN = SCREEN_SAMPPLING;
      break;
    case SCREEN_TUNIT:
      if (!test_bit(PIND, PD1)) return; // Botão pressionado
      TEMP_UNIT_OPTION = TEMP_KELVIN;
      CURRENT_SCREEN = SCREEN_HOME;
      break;
    case SCREEN_TEMP:
      if (!test_bit(PIND, PD1)) return; // Botão pressionado
      CURRENT_SCREEN = SCREEN_MAX_TEMP;
      break;
    case SCREEN_MIN_TEMP:
      if (MIN_TEMP > -40.0) MIN_TEMP -= 0.1;
      break;
    case SCREEN_MAX_TEMP:
      if (MAX_TEMP > -40.0) MAX_TEMP -= 0.1;
      break;
    case SCREEN_HUM:
      if (!test_bit(PIND, PD1)) return; // Botão pressionado
      CURRENT_SCREEN = SCREEN_MAX_HUM;
      break;
    case SCREEN_MIN_HUM:
      if (MIN_HUM > 0.1) MIN_HUM -= 0.1;
      break;
    case SCREEN_MAX_HUM:
      if (MAX_HUM > 0.1) MAX_HUM -= 0.1;
      break;
    case SCREEN_SAMPPLING:
      if (SAMPPLING > 1) SAMPPLING -= 1;
      break;
    case SCREEN_WARN_SMOKE:
    case SCREEN_WARN_LIMIT:
      CURRENT_SCREEN = SCREEN_HOME;
      break;
    default:
      break;
  }

  update_screen();
  _delay_ms(100);
}

//====================================== End - Interruptions ============================================ //