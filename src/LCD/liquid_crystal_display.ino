//================================================================================ //
// ACTIVATING A 16x2 LIQUID CRYSTAL DISPLAY
// 
// !4-bit data interface!
//
// - A Register for the 4 LCD Data Bits (PA4-D4, PA5-D5, PA6-D6, PA7-D7)
// - H register for LCD control pins (Enable - PH6 e RS - PH5)
// - LCD IC HD44780
// - Arduino Mega ATMega 2560
//
// |========================== CORRESPONDING ADDRESS ON LCD ===============================|
// | -     | 1  | 2  | 3  | 4  | 5  | 6  | 7  | 8  | 9  | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
// |-------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
// | row 1 | 80 | 81 | 82 | 83 | 84 | 85 | 86 | 87 | 88 | 89 | 8A | 8B | 8C | 8D | 8E | 8F |
// | row 2 | C0 | C1 | C2 | C3 | C4 | C5 | C6 | C7 | C8 | C9 | CA | CB | CC | CD | CE | CF |
// |=======================================================================================|
//================================================================================ //

#define F_CPU 16000000 UL

#include <avr/io.h>
#include <util/delay.h>

//============== Macro definitions for working with bits =====------============== // 
#define set_bit(y, bit)(y |= (1 << bit))    // sets the x bit of the variable Y to 1
#define clear_bit(y, bit)(y &= ~(1 << bit)) // sets bit x of variable Y to 0
#define toggle_bit(y, bit)(y ^= (1 << bit)) // changes the logical state of bit x of variable Y
#define test_bit(y, bit)(y & (1 << bit))    // returns 0 or 1 depending on the bit reading
//================================================================================ //

#define DADOS_LCD PORTA
#define nibble_dados 1 // (PA4-D4, PA5-D5, PA6-D6, PA7-D7)
#define CONTR_LCD PORTH
#define E PH6
#define RS PH5

#define LCD_INSTRUCTION 0
#define LCD_CHARACTER 1

#define pulso_enable() _delay_us(1); set_bit(CONTR_LCD, E); _delay_us(1); clear_bit(CONTR_LCD, E); _delay_us(45);

void send_command_to_LCD(unsigned char c, char type) {
  if (type == LCD_INSTRUCTION) clear_bit(CONTR_LCD, RS); // command is instruction
  if (type == LCD_CHARACTER) set_bit(CONTR_LCD, RS);     // command is character

  // ============== First data nibble - 4 MSB
  if(nibble_dados){
    DADOS_LCD = (DADOS_LCD & 0x0F) | (0xF0 & c);
  }else{
    DADOS_LCD = (DADOS_LCD & 0xF0) | (c >> 4);
  }

  pulso_enable();

  // ============== Second data nibble - 4 LSB
  if(nibble_dados){
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
  if(nibble_dados){
    DADOS_LCD = (DADOS_LCD & 0x0F) | 0x30;
  }else{
    DADOS_LCD = (DADOS_LCD & 0xF0) | 0x03;
  }

  pulso_enable(); _delay_ms(5); pulso_enable(); _delay_us(200); pulso_enable(); 

  // ============== Second data nibble - 4 MSB
  if(nibble_dados){
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
void writes_on_LCD(char * c) { for (;* c != 0; c++) send_command_to_LCD( * c, 1); }

int main() {
  DDRH = 0xFF; DDRB = 0xFF; // PORT B and H is outputs.
  
  initialize_LCD();
  writes_on_LCD("ABCDEFGHIJKLMNOP");
  send_command_to_LCD(0XC0, 0); // initializes cursor in the first position on the left - 2nd line
  writes_on_LCD("QRSTUVWXYZ123456789");
  
  for (;;) {}
}