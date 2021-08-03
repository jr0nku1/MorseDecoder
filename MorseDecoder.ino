/*----- MorseDecoder -----*/
/**********************************************

    Morse decoder  ver 3.0
    
    programmed by kaneko  Jan.6  2015
    ver2.0 Add LED monitor by kaneko Nov.11 2016
    ver3.0 New demodulator by kaneko Jun.10 2017
    
***********************************************/
#include <MsTimer2.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,4,5,6,7);

/*=============================================
    constants
===============================================*/
#define    OPENNING      "MORSE DECODER3.0"

#define    ADC_PIN       1      /* ADC pin      */
#define    LED_PIN       13     /* LED pin      */
#define    TIM_INT       5      /* timer (ms)   */

#define    TBUFF_LEN     32     /* time count buffer length */
//#define    TBUFF_UPDATE  5      /* time count buffer update threshold */

#define    DEMOD_TH_H    3
#define    DEMOD_TH_L    0

#define    MRS_TH_SL     20     /* LONG count threshold (TIM_INT*MRS_TH_SL = 100ms) */

#define    LCD_ROW       16     /* LCD row      */

#define    LANG_ENG      0      /* English */
#define    LANG_JAP      1      /* Japanese */

/*----- for bottun -----*/
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

/*=============================================
    globals
===============================================*/
int    adc_max_n;            /* ADC loop         */
int    adc_amax;             /* for debug        */
int    demod_th;             /* demod threshold  */
int    demod_cnt;            /* demod counter    */
boolean demod_mark;          /* demod mark flag  */

char   tbuff[TBUFF_LEN];     /* time count buffer  */
int    tbuff_top;            /* time buffer top  */
int    tbuff_bottom;         /* time buffer bottom */

int    lang;                 /* language         */

char   *mrs_char[6];         /* morse code index */
char   mrs_len1[2]  = {'E', 'T'};
char   mrs_len2[4]  = {'I', 'N', 'A', 'M'};
char   mrs_len3[8]  = {'S', 'D', 'R', 'G', 'U', 'K', 'W', 'O'};
char   mrs_len4[16] = {'H', 'B', 'L', 'Z', 'F', 'C', 'P', ' ',
                       'V', 'X', ' ', 'Q', ' ', 'Y', 'J', ' '};
char   mrs_len5[32] = {'5', '6', ' ', '7', ' ', ' ', ' ', '8',
                       ' ', '/', ' ', ' ', ' ', ' ', ' ', '9',
                       '4', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       '3', ' ', ' ', ' ', '2', ' ', '1', '0'};
char   mrs_len6[64] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', '?', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

char   *mrs_jchar[6];         /* morse code index */
char   mrs_jlen1[2]  = {0xcd, 0xd1};
char   mrs_jlen2[4]  = {0xde, 0xc0, 0xb2, 0xd6};
char   mrs_jlen3[8]  = {0xd7, 0xce, 0xc5, 0xd8, 0xb3, 0xdc, 0xd4, 0xda};
char   mrs_jlen4[16] = {0xc7, 0xca, 0xb6, 0xcc, 0xc1, 0xc6, 0xc2, 0xbf,
                        0xb8, 0xcf, 0xdb, 0xc8, 0xc9, 0xb9, 0xa6, 0xba};
char   mrs_jlen5[32] = {'5', '6', 0xb5, '7', 0xc4, 0xb7, ' ', '8',
                       ' ', 0xd3, 0xdd, 0xbc, 0xdf, 0xd9, 0xbe, '9',
                       '4', 0xd2, ' ', 0xcb, 0xd0, 0xbb, 0xb0, 0xbd,
                       '3', 0xd5, 0xc3, 0xb1, '2', 0xb4, '1', '0'};
char   mrs_jlen6[64] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', '?', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', 0xa4, ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                       ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

char   lcd_buff[LCD_ROW+1];    /* LCD roll buffer    */
int    lcd_x, lcd_y;         /* LCD display position */

/*=============================================
    LCD display
===============================================*/
void lcd_disp(char c)
  {
  if(lcd_x == LCD_ROW)
    {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lcd_buff);
    lcd_x = 0;
    lcd_y = 1;
    }
  lcd.setCursor(lcd_x, lcd_y);
  lcd.write(c);
  lcd_buff[lcd_x] = c;
  ++lcd_x;
  }

/*=============================================
    button
===============================================*/
int read_LCD_buttons(){               // read the buttons
    int    adc_key_in;

    noInterrupts();
    adc_key_in = analogRead(0);       // read the value from the sensor 
    interrupts();
    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    // We make this the 1st option for speed reasons since it will be the most likely result

    if (adc_key_in > 1000) return btnNONE; 

    // For V1.1 us this threshold
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 250)  return btnUP; 
    if (adc_key_in < 450)  return btnDOWN; 
    if (adc_key_in < 650)  return btnLEFT; 
    if (adc_key_in < 850)  return btnSELECT;  

   // For V1.0 comment the other threshold and use the one below:
   /*
     if (adc_key_in < 50)   return btnRIGHT;  
     if (adc_key_in < 195)  return btnUP; 
     if (adc_key_in < 380)  return btnDOWN; 
     if (adc_key_in < 555)  return btnLEFT; 
     if (adc_key_in < 790)  return btnSELECT;   
   */

    return btnNONE;                // when all others fail, return this.
}


/*=============================================
    interrupt
===============================================*/
/*--------------------------------------------- timer int. */
void timer_int()
  {
  volatile int  i;
  volatile int  dat;
  
  /*----- get ADC max -----*/
  adc_amax = analogRead(ADC_PIN);
  for(i = 1; i < adc_max_n; ++i)
    {
    dat = analogRead(ADC_PIN);
    if(adc_amax <= dat) adc_amax = dat;
    }

  /*----- demod -----*/
  if(adc_amax > demod_th)
    {
//    digitalWrite(LED_PIN, HIGH);
    if(demod_cnt < DEMOD_TH_H) ++demod_cnt;
    }
    else
    {
//    digitalWrite(LED_PIN, LOW);
    if(demod_cnt > DEMOD_TH_L) --demod_cnt;
    }
    
  /*----- calculate length -----*/
  if((demod_cnt == DEMOD_TH_H) || (demod_mark == true)) 
      {
      if(demod_mark == false)
        {
        ++tbuff_top;
        if(tbuff_top == TBUFF_LEN) tbuff_top = 0;
        tbuff[tbuff_top] = 0;
        demod_mark = true;
        }
      if(tbuff[tbuff_top] < MRS_TH_SL) ++tbuff[tbuff_top];
      }
      
  if(demod_cnt == DEMOD_TH_L || (demod_mark == false))
      {
      if(demod_mark == true)
        {
        ++tbuff_top;
        if(tbuff_top == TBUFF_LEN) tbuff_top = 0;
        tbuff[tbuff_top] = 0;
        demod_mark = false;
        }

//      if(tbuff[tbuff_top] > TBUFF_MIN) --tbuff[tbuff_top];
      --tbuff[tbuff_top];
      if(tbuff[tbuff_top]  <= -1*MRS_TH_SL)
        {
        ++tbuff_top;
        if(tbuff_top == TBUFF_LEN) tbuff_top = 0;
        tbuff[tbuff_top] = 0;
        }
      }

    /*----- LED -----*/
    if(demod_mark)
      digitalWrite(LED_PIN, HIGH);
    else
      digitalWrite(LED_PIN, LOW);
  
  }

/*=============================================
    main
===============================================*/
void setup()
  {
  lcd.begin(16,2);
  pinMode(LED_PIN, OUTPUT);
  analogReference(INTERNAL);
//  analogReference(DEFAULT);
  adc_max_n = 20;    /* default cicle of ADC (3)     */
  adc_amax = 0;     /* for debug                */
//  demod_th = 80;   /* default demod threshold  */
  demod_th = 40;   /* default demod threshold  */
  demod_cnt = DEMOD_TH_L;
  demod_mark = false;
  tbuff_top = 0;    /* time buffer top */
  tbuff_bottom = 0;  /* time buffer bottom */
  tbuff[tbuff_top] = -1;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(OPENNING);
  lcd_buff[LCD_ROW] = 0x00;
  lcd_x = 0;          /* LCD display position */
  lcd_y = 1;          /* LCD display position */
  lang = LANG_ENG;    /* default language    */
  
  mrs_char[0] = mrs_len1;  /* set morse code table */
  mrs_char[1] = mrs_len2;
  mrs_char[2] = mrs_len3;
  mrs_char[3] = mrs_len4;
  mrs_char[4] = mrs_len5;
  mrs_char[5] = mrs_len6;
  
  mrs_jchar[0] = mrs_jlen1;  /* set morse code table */
  mrs_jchar[1] = mrs_jlen2;
  mrs_jchar[2] = mrs_jlen3;
  mrs_jchar[3] = mrs_jlen4;
  mrs_jchar[4] = mrs_jlen5;
  mrs_jchar[5] = mrs_jlen6;
  
  Serial.begin(9600);
  MsTimer2::set(TIM_INT, timer_int);
  MsTimer2::start();
  Serial.println("Morse Decoder  ver 3.0");
  }

void loop()
  {
  int    cmd;
  int    flag_sp;
  int    mcode, mlen;
  char   c;

  flag_sp = 2;
  mcode = 0x00;
  mlen = 0;
  Serial.println();  
  do
    {
    /* decode */
    if(tbuff_top != tbuff_bottom) 
      {
      if(tbuff[tbuff_bottom] <= -1*MRS_TH_SL)
         {
         switch(flag_sp)
           {
           case 0:
             if(mlen == 0) break;
             if(lang == LANG_ENG)
                 c = *(mrs_char[mlen - 1] + mcode);
             else
                 c = *(mrs_jchar[mlen - 1] + mcode);           
//             Serial.print(c);
//             Serial.print('(');
//             Serial.print(mlen,DEC);
//             Serial.print(mcode,HEX);
//             Serial.print(')');
             lcd_disp(c);
             ++flag_sp;
             break;
           case 1:
             ++flag_sp;
             break;
           case 2:
             Serial.print(" ");
             lcd_disp(' ');
             ++flag_sp;
           }
         mlen = 0;
         mcode = 0x00;
         }
      else
        {
        if((mlen != 0) || (tbuff[tbuff_bottom] > 0))
          {
          if((tbuff[tbuff_bottom] > 0) && (mlen < 6))
            {
            if(mlen > 5) mlen = 5;
            if(tbuff[tbuff_bottom] >= MRS_TH_SL) mcode |= 0x01 << mlen;
            ++mlen;
            }
          }
//        Serial.print(tbuff[tbuff_bottom], DEC);
//        Serial.print(" ");
        flag_sp = 0;
        }
      
      ++tbuff_bottom;
      if(tbuff_bottom == TBUFF_LEN) tbuff_bottom = 0;
      }
    
    /* scan button */
    if(read_LCD_buttons() == btnRIGHT)
      {
      lcd.clear();
      lcd.setCursor(0,0);
      if(lang == LANG_ENG)
        {
        lang = LANG_JAP;
        lcd.print("JAPANESE");
        }
      else
        {lang = LANG_ENG;
        lcd.print("ENGLISH");
        }
      lcd_x = 0;          /* LCD display position */
      lcd_y = 1;          /* LCD display position */
      while(read_LCD_buttons() != btnNONE);
      }

    /* scan serial */
    cmd = Serial.read();
    } while(cmd == -1);
  
  if(cmd == 's')
    {
    Serial.print("adc_max_n = ");  Serial.println(adc_max_n);
    Serial.print("adc_amax = ");  Serial.println(adc_amax);
    Serial.print("demod_th = ");  Serial.println(demod_th);    
    Serial.print("read_LCD_buttons() = ");  Serial.println(read_LCD_buttons());    
//    noInterrupts();
    Serial.print("analogRead(0) = ");  Serial.println(analogRead(0));    
//    interrupts();
    }
  }
  
