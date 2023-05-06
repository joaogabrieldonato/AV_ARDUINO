#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <avr/pgmspace.h>
#include <DS1302.h>
#include "hard_defs.h"
#include "soft_defs.h"


#define DEBOUNCE_TIME 150

//finalizar read_config
//RTC

/*Variáveis*/
LiquidCrystal_I2C lcd(0x3F, 16, 2);
DS1302 rtc(5, 6, 7);
//File f;

/*Declaração de funções*/
String format_time (unsigned long int t1);
char potSelect (byte pin, byte num_options);
void isr_30m_100m ();
void isr_101m ();
bool read_config();
//bool write_config();
byte countFiles(File dir);
void isr_void ();
void printRun();

void setup()
{
  rtc.halt(false);
  rtc.writeProtect(true);
  Serial.begin(9600);
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  //    init_screens();
  pinMode(13, OUTPUT);
  pinMode(M_ZERO, INPUT);
  pinMode(M_30_100, INPUT);
  pinMode(M_101, INPUT);
  pinMode(B_SEL, INPUT_PULLUP);
  pinMode(B_CANC, INPUT_PULLUP);

  pinMode(DS1302_VCC_PIN, OUTPUT);
  pinMode(DS1302_GND_PIN, OUTPUT);
  digitalWrite(DS1302_VCC_PIN, HIGH);
  digitalWrite(DS1302_GND_PIN, LOW);
}

void loop()
{
  switch (ss)
  {
    case BEGIN:
        lcd.clear();
        lcd.print(F("Bem vindo"));
        read_config();
        delay(500);

        ss = MENU;
        ss_c = SHOW;
        ss_r = START_;
      break;
    case MENU:
      pos[0] = 17; pos[1] = 23;
      pot_sel = potSelect(POT, 2);

      if (pot_sel != old_pot)
      {
        lcd.setCursor(0, 0);
        lcd.print(F("MANGUE AV - "));
        lcd.print(new_config.vei);
        lcd.print(" ");
        lcd.setCursor(0, 1);
        lcd.println(F("   RUN   CONFIG "));
        lcd.setCursor(pos[pot_sel] % 16, (int) pos[pot_sel] / 16);
        lcd.write('>');
        old_pot = pot_sel;
      }
      if (!digitalRead(B_SEL))
      {
        delay(DEBOUNCE_TIME);
        while (!digitalRead(B_SEL));

        t_30 = 0;
        t_100 = 0;
        t_101 = 0;
        vel = 0;
        t_curr = millis();
        ss = pot_sel + RUN;
        old_pot = -1;
      }

      break;
    case RUN:
      if (!digitalRead(B_CANC))
      {
        delay(2000);

        if (!digitalRead(B_CANC))
        {
          ss = MENU;
        }
        detachInterrupt(digitalPinToInterrupt(M_30_100));
        detachInterrupt(digitalPinToInterrupt(M_101));
        interrupt = false;
        old_pot = -1;
        ss_r = START_;

        break;
      }

      switch (ss_r)
      {
        case START_:
          t_30 = 0;
          t_100 = 0;
          t_101 = 0;
          vel = 0;
          str_vel = "00.00 km/h";
          printRun();
          
          if (digitalRead(M_ZERO))
          {
            ss_r = WAIT_30;
            t_curr = millis();
          }
          break;
        case WAIT_30:
          EIFR &= ~(1<<INTF1); //clear interrupt flag bit
          attachInterrupt(digitalPinToInterrupt(M_30_100), isr_30m_100m, FALLING);
          while(!interrupt && ss_r == WAIT_30)
          {
            t_30 = millis() - t_curr;
            t_100 = millis() - t_curr;
            printRun();

            
            if (!digitalRead(B_CANC))
            {
                delay(2000);
        
                if (!digitalRead(B_CANC))
                {
                  ss = MENU;
                }
                detachInterrupt(digitalPinToInterrupt(M_30_100));
                detachInterrupt(digitalPinToInterrupt(M_101));
                interrupt = false;
                old_pot = -1;
                ss_r = START_;
        
                break;
             }
          }
          printRun();
          interrupt = false;
          
          EIMSK &= ~(1 << INT0); //clear interrupts
          ss_r = WAIT_100;
          
          break;
        case WAIT_100:
          attachInterrupt(digitalPinToInterrupt(M_30_100), isr_void, FALLING); //interrupt that does nothing for clearing registers
          while(millis() - t_curr - t_30 < 2000)
          {
            t_100 = millis() - t_curr;
            printRun();
          }
          EIFR &= ~(1<<INTF1); //clear interrupt flag bit
          attachInterrupt(digitalPinToInterrupt(M_30_100), isr_30m_100m, FALLING);
          
          while(!interrupt && ss_r == WAIT_100)
          {
            t_100 = millis() - t_curr;
            printRun();
            
                  if (!digitalRead(B_CANC))
                  {
                    delay(2000);
            
                    if (!digitalRead(B_CANC))
                    {
                      ss = MENU;
                    }
                    detachInterrupt(digitalPinToInterrupt(M_30_100));
                    detachInterrupt(digitalPinToInterrupt(M_101));
                    interrupt = false;
                    old_pot = -1;
                    ss_r = START_;
            
                    break;
                  }
          }

          while(digitalRead(M_101))
            t_101 = millis() - t_curr;
          if(!digitalRead(M_101))
            t_101 = millis() - t_curr;
            
          printRun();
          interrupt = false;
          ss_r = WAIT_101;
          
          break;
        case WAIT_101:
        Serial.println(t_101);
//          EIFR |= 0x01;
//          attachInterrupt(digitalPinToInterrupt(M_101), isr_101m, FALLING);
//          while(!interrupt)
//          {
//            
//                  if (!digitalRead(B_CANC))
//                  {
//                    delay(2000);
//            
//                    if (!digitalRead(B_CANC))
//                    {
//                      ss = MENU;
//                    }
//                    detachInterrupt(digitalPinToInterrupt(M_30_100));
//                    detachInterrupt(digitalPinToInterrupt(M_101));
//                    interrupt = false;
//                    old_pot = -1;
//                    ss_r = START_;
//            
//                    break;
//                  }
//          }
//          interrupt = false;
          ss_r = END_RUN;
          break;
        case END_RUN:
        attachInterrupt(digitalPinToInterrupt(M_30_100), isr_void, FALLING);
        attachInterrupt(digitalPinToInterrupt(M_101), isr_void, FALLING);
          if (vel == 0)
          {
            vel = (double) (t_101 - t_100) / 1000.0;
            vel = (double) 3.6 / vel;
            str_vel = String(vel, 2) + " km/h";
          }
          printRun();

          if (!digitalRead(B_SEL))
          {
            delay(DEBOUNCE_TIME);
            while (!digitalRead(B_SEL));
            ss_r = SAVE_RUN;
            delay(100);
          }
          break;
        case SAVE_RUN:
          pos[0] = 17; pos[1] = 24;
          pot_sel = potSelect(POT, 2);

          if (pot_sel != old_pot)
          {
            lcd.setCursor(0, 0);
            lcd.print(F(" DESEJA SALVAR? "));
            lcd.setCursor(0, 1);
            lcd.print(F("   SIM    NAO   "));
            lcd.setCursor(pos[pot_sel] % 16, (int) pos[pot_sel] / 16);
            lcd.write('>');
            old_pot = pot_sel;
          }

          if (!digitalRead(B_SEL))
          {
            Time t = rtc.getTime();

            String time_now = rtc.getTimeStr();
            new_config.data = rtc.getDateStr();

            if (pot_sel == 0)
            {
              if (!SD.begin(10))
              {
                lcd.clear();
                lcd.print("Nao ha SD");
                delay(300);
                ss_r = END_RUN;
                old_pot = -1;
                return;
              }
              else
              {
                lcd.clear();
                lcd.println("   Salvando...  ");
              }
              //
              if (!SD.exists("/" + new_config.data + "/"))
                SD.mkdir(new_config.data);

              File dir = SD.open("/" + new_config.data + "/");
              byte num_files = countFiles(dir);
              String new_file = String(num_files) + 'x' + String(t_30) + ".txt";
              File f = SD.open("/" + new_config.data + "/" + new_file, FILE_WRITE);
              Serial.println(new_config.vei);

              f.print(F("HORA: "));
              f.println(time_now);
              f.print(F("VEICULO: "));
              f.println(new_config.vei);
              f.print(F("MOTORA:\n\tMOLA: "));
              f.println(new_config.mola_mot);
              f.print(F("\tPRE CARGA: "));
              f.println(new_config.pre_mot);
              f.print(F("\tPESO: "));
              f.println(new_config.peso_mot);
              f.print(F("MOVIDA:\n\tMOLA: "));
              f.println(new_config.mola_mov);
              f.print(F("\tPRE CARGA: "));
              f.println(new_config.pre_mov);
              f.print('\n');
              f.println("30m: " + str_30);
              f.println("100m: " + str_100);
              f.println("VEL: " + str_vel);

              f.close();
              delay(500);
            }

            old_pot = -1;
            ss = MENU;
            ss_r = START_;
          }
          break;
      }
      break;
    case CONFIG:
      if (ss_c != SHOW)
      {
        // volta para tela anterior
        if (!digitalRead(B_CANC))
        {
          delay(DEBOUNCE_TIME);
          while (!digitalRead(B_CANC));
          if (ss_cvt == GERAL_C)
            ss_c = GERAL;
          else
          {
            ss_cvt = GERAL_C;
            ss_c = GERAL;
          }
          old_pot = -1;
          old_num = -1;
          clk_cnt = 0;
        }
      }
      switch (ss_c)
      {
        case SHOW:
          //mostrar configurações atuais
          if (clk_cnt == 0)
          {
            lcd.clear();
            lcd.write(' ');
            lcd.print(new_config.mola_mot);
            lcd.write(' ');
            lcd.print(new_config.pre_mot);
            lcd.write(' ');
            lcd.print(new_config.peso_mot);
            lcd.setCursor(0, 1);
            lcd.write(' ');
            lcd.print(new_config.mola_mov);
            lcd.write(' ');
            lcd.print(new_config.pre_mov);
            lcd.print("o furo");
            clk_cnt++;
          }

          if (!digitalRead(B_SEL))
          {
            delay(DEBOUNCE_TIME);
            while (!digitalRead(B_SEL));
            ss_c = GERAL;
            old_pot = -1;
            clk_cnt = 0;
          }
          if (!digitalRead(B_CANC))
          {
            delay(DEBOUNCE_TIME);
            while (!digitalRead(B_CANC));
            ss = MENU;
            old_pot = -1;
            clk_cnt = 0;
          }
          break;
        case GERAL:
          pos[0] = 4; pos[1] = 16; pos[2] = 24;
          pot_sel = potSelect(POT, 3);
          if (pot_sel != old_pot)
          {
            lcd.setCursor(0, 0);
            lcd.println(F("     VEICULO    "));
            lcd.setCursor(0, 1);
            lcd.println(F(" MOTORA  MOVIDA "));
            lcd.setCursor(pos[pot_sel] % 16, (int) pos[pot_sel] / 16);
            lcd.write('>');
            old_pot = pot_sel;
          }

          if (!digitalRead(B_SEL))
          {
            delay(DEBOUNCE_TIME);
            while (!digitalRead(B_SEL));

            old_pot = -1;
            ss_c = pot_sel + 2;
          }
          else if (!digitalRead(B_CANC))
          {
            delay(DEBOUNCE_TIME);
            while (!digitalRead(B_CANC));
            old_pot = -1;
            ss_c = SAVE_CONFIG;
          }
          break;
        case VEICULO:
          pos[0] = 3; pos[1] = 19;
          pot_sel = potSelect(POT, 2);
          if (pot_sel != old_pot)
          {
            lcd.setCursor(0, 0);
            lcd.println(F("      MB1    VEI"));
            lcd.setCursor(0, 1);
            lcd.println(F("      MB2       "));
            lcd.setCursor(pos[pot_sel] % 16, (int) pos[pot_sel] / 16);
            lcd.write('>');
            old_pot = pot_sel;
          }
          if (!digitalRead(B_SEL))
          {
            delay(DEBOUNCE_TIME);
            while (!digitalRead(B_SEL));

            new_config.vei[2] = (pot_sel + 1) + '0';
            old_pot = -1;
            ss_c = GERAL;
          }
          break;
        case MOTORA:
          //mostrar as opções para mola da motora
          switch (ss_cvt)
          {
            case GERAL_C:
              pos[0] = 1; pos[1] = 17; pos[2] = 23;
              pot_sel = potSelect(POT, 3);
              if (pot_sel != old_pot)
              {
                lcd.setCursor(0, 0);
                lcd.println(F("  PRE-CARGA  MOT"));
                lcd.setCursor(0, 1);
                lcd.println(F("  MOLA  PESO    "));
                lcd.setCursor(pos[pot_sel] % 16, (int) pos[pot_sel] / 16);
                lcd.write('>');
                old_pot = pot_sel;
              }
              if (!digitalRead(B_SEL))
              {
                delay(DEBOUNCE_TIME);
                while (!digitalRead(B_SEL));

                old_pot = -1;
                ss_cvt = pot_sel + 1;
              }
              break;
            case PESO:
              pos[0] = 5; pos[1] = 6; pos[2] = 8;
              num_sel = potSelect(POT, 10);

              if (num_sel != old_num)
              {
                if (clk_cnt == 0)
                {
                  lcd.setCursor(0, 0);
                  lcd.println(F("     00.0g   MOT"));
                  lcd.setCursor(0, 1);
                  lcd.println(F("            PESO"));
                }

                lcd.setCursor(pos[clk_cnt] % 16, (int) pos[clk_cnt] / 16);
                lcd.write(num_sel + '0');
                old_num = num_sel;
              }

              if (!digitalRead(B_SEL))
              {
                delay(DEBOUNCE_TIME);
                while (!digitalRead(B_SEL));

                if (clk_cnt == 1)
                {
                  new_config.peso_mot[1] = num_sel + '0';
                  new_config.peso_mot[2] = '.';
                  clk_cnt++;
                }
                else if (clk_cnt == 2)
                {
                  new_config.peso_mot[3] = num_sel + '0';
                  new_config.peso_mot[4] = 'g';
                  clk_cnt = 0;
                  ss_cvt = GERAL_C;
                }
                else
                {
                  new_config.peso_mot[clk_cnt] = num_sel + '0';
                  clk_cnt++;
                }

                old_num = -1;
              }
              break;
            case PRE:
              pos[0] = 6; pos[1] = 7;
              num_sel = potSelect(POT, 10);

              if (num_sel != old_num)
              {
                if (clk_cnt == 0)
                {
                  lcd.setCursor(0, 0);
                  lcd.println(F("      00mm   MOT"));
                  lcd.setCursor(0, 1);
                  lcd.println(F("             PRE"));
                }
                lcd.setCursor(pos[clk_cnt] % 16, (int) pos[clk_cnt] / 16);
                lcd.write(num_sel + '0');
                old_num = num_sel;
              }

              if (!digitalRead(B_SEL))
              {
                delay(DEBOUNCE_TIME);
                while (!digitalRead(B_SEL));

                new_config.pre_mot[clk_cnt] = num_sel + '0';

                if (clk_cnt == 1)
                {
                  new_config.pre_mot[2] = 'm';
                  new_config.pre_mot[3] = 'm';
                  clk_cnt = 0;
                  ss_cvt = GERAL_C;
                }
                else
                  clk_cnt++;
                old_num = -1;
              }
              break;
            case MOLA:
              pos[0] = 6;
              clk_cnt = 0;
              num_sel = potSelect(POT, 10);

              if (num_sel != old_num)
              {
                if (clk_cnt == 0)
                {
                  lcd.setCursor(0, 0);
                  lcd.println(F("      0X     MOT"));
                  lcd.setCursor(0, 1);
                  lcd.println(F("            MOLA"));
                }

                lcd.setCursor(pos[clk_cnt] % 16, (int) pos[clk_cnt] / 16);
                lcd.write(num_sel + '0');
                old_num = num_sel;
              }

              if (!digitalRead(B_SEL))
              {
                delay(DEBOUNCE_TIME);
                while (!digitalRead(B_SEL));

                new_config.mola_mot[0] = num_sel + '0';
                new_config.mola_mot[1] = 'X';
                old_num = -1;
                ss_cvt = GERAL_C;
              }
              break;
          }
          break;
        case MOVIDA:
          switch (ss_cvt)
          {
            case GERAL_C:
              pos[0] = 1; pos[1] = 17;
              pot_sel = potSelect(POT, 2);
              if (pot_sel != old_pot)
              {
                lcd.setCursor(0, 0);
                lcd.println(F("  PRE-CARGA  MOV"));
                lcd.setCursor(0, 1);
                lcd.println(F("  MOLA          "));
                lcd.setCursor(pos[pot_sel] % 16, (int) pos[pot_sel] / 16);
                lcd.write('>');
                old_pot = pot_sel;
              }
              if (!digitalRead(B_SEL))
              {
                delay(DEBOUNCE_TIME);
                while (!digitalRead(B_SEL));

                old_pot = -1;
                ss_cvt = pot_sel + 1;
              }
              break;
            case PRE:
              pos[0] = 3;
              clk_cnt = 0;
              num_sel = potSelect(POT, 10);

              if (num_sel != old_num)
              {
                if (clk_cnt == 0)
                {
                  lcd.setCursor(0, 0);
                  lcd.println(F("   0o furo   MOV"));
                  lcd.setCursor(0, 1);
                  lcd.println(F("             PRE"));
                }
                lcd.setCursor(pos[clk_cnt] % 16, (int) pos[clk_cnt] / 16);
                lcd.write(num_sel + '0');
                old_num = num_sel;
              }

              if (!digitalRead(B_SEL))
              {
                delay(DEBOUNCE_TIME);
                while (!digitalRead(B_SEL));

                new_config.pre_mov[0] = num_sel + '0';
                new_config.pre_mov[1] = '\0';
                ss_cvt = GERAL_C;
                old_num = -1;
              }
              break;
            case MOLA:
              pos[0] = 0; pos[1] = 6; pos[2] = 16; pos[3] = 22;
              pot_sel = potSelect(POT, 4);

              if (pot_sel != old_pot)
              {
                lcd.setCursor(0, 0);
                lcd.println(F(" AMAR. AZUL  MOV"));
                lcd.setCursor(0, 1);
                lcd.println(F(" VERM. CINZ MOLA"));
                lcd.setCursor(pos[pot_sel] % 16, (int) pos[pot_sel] / 16);
                lcd.write('>');
                old_pot = pot_sel;
              }

              if (!digitalRead(B_SEL))
              {
                delay(DEBOUNCE_TIME);
                while (!digitalRead(B_SEL));

                if (pot_sel == 0)
                  strcpy(new_config.mola_mov, "AMAR");
                else if (pot_sel == 1)
                  strcpy(new_config.mola_mov, "AZUL");
                else if (pot_sel == 2)
                  strcpy(new_config.mola_mov, "VERM");
                else
                  strcpy(new_config.mola_mov, "CINZ");

                old_pot = -1;
                ss_cvt = GERAL_C;
              }
              break;
          }
          break;
        //                case DATA:
        //                    pos[0] = 1; pos[1] = 2; pos[2] = 4; pos[3] = 5; pos[4] = 9; pos[5] = 10; pos[6] = 17; pos[7] = 18; pos[8] = 20; pos[9] = 21;
        //                    num_sel = potSelect(POT, 10);
        //
        //                    if (num_sel != old_num)
        //                    {
        //                        if (clk_cnt == 0)
        //                        {
        //                            lcd.setCursor(0,0);
        //                            lcd.println(F(" 11/10/2000  DAT"));
        //                            lcd.setCursor(0,1);
        //                            lcd.println(F(" 04:20          "));
        //                        }
        //
        //                        lcd.setCursor(pos[clk_cnt]%16, (int) pos[clk_cnt]/16);
        //                        lcd.write(num_sel + '0');
        //                        old_num = num_sel;
        //                    }
        //
        //                    if (!digitalRead(B_SEL))
        //                    {
        //                        delay(DEBOUNCE_TIME);
        //                        while(!digitalRead(B_SEL));
        //
        //                        if (clk_cnt == 9)
        //                        {
        //                            clk_cnt = 0;
        //                            old_num = -1;
        //                            ss_c = GERAL;
        //                            ss_cvt = GERAL_C;
        //                        }
        //                        else
        //                            clk_cnt++;
        //                        }
        //                    break;
        case SAVE_CONFIG:
          //                            Serial.println(new_config.vei + "," + new_config.data + "," + new_config.mola_mot + "," + new_config.mola_mov + "," + new_config.pre_mot + "," + new_config.pre_mov + "," + new_config.peso_mot);
          //                            lcd.print(new_config.mola_mov);
          lcd.clear();
          lcd.print(F("  Salvando...   "));
          File f;
          if (SD.exists("config.txt"))
            SD.remove("config.txt");

          f = SD.open("config.txt", FILE_WRITE);

          //                            f.print(new_config.data);
          //                            f.print(',');
          f.print(new_config.vei);
          f.print(',');
          f.print(new_config.mola_mot);
          f.print(',');
          f.print(new_config.pre_mot);
          f.print(',');
          f.print(new_config.peso_mot);
          f.print(',');
          f.print(new_config.mola_mov);
          f.print(',');
          f.print(new_config.pre_mov);
          f.print('\n');
          //                            f.println(new_config.vei + "," + new_config.data + "," + new_config.mola_mot + "," + new_config.mola_mov + "," + new_config.pre_mot + "," + new_config.pre_mov + "," + new_config.peso_mot);
          f.close();
          delay(2000);

          old_pot = -1;
          ss_c = SHOW;
          ss_r = START_;
          ss = MENU;
          break;
      }
      break;
  }
}

String format_time(unsigned long int t1)
{
  if (t1 < 10000)
    return '0' + String(t1 / 1000) + ':' + String(t1 % 1000);
  else
    return String(t1 / 1000) + ':' + String(t1 % 1000);
}

char potSelect (byte pin, byte num_options)
/*Mapeia o curso do potenciometro para percorrer 2 vezes as opções de seleção,
  retorna o valor da opção escolhida*/
{
  int read_val = analogRead(pin);
  byte option = map(read_val, 50, 1000, 0, 2 * num_options - 1);
  //    if (option >= num_options)
  //        option -= num_options;
  return option % num_options;
}

void printRun()
{
    str_30 = format_time(t_30);
    str_100 = format_time(t_100);
    str_101 = format_time(t_101);
    lcd.setCursor(0, 0);
    lcd.print(' ' + str_30 + "  " + str_100 + "    ");
    lcd.setCursor(0, 1);
    lcd.print("   " + str_vel + "        ");
}

void isr_30m_100m ()
{
  if (ss_r == WAIT_30)
  {
//    ss_r = WAIT_100;
    t_30 = millis() - t_curr;
  }
  else
  {
//    ss_r = WAIT_101;
    t_100 = millis() - t_curr;
  }
  interrupt = true;
  detachInterrupt(digitalPinToInterrupt(M_30_100));
}

void isr_101m ()
{
  ss_r = END_RUN;
  t_101 = millis() - t_curr;
  interrupt = true;
  detachInterrupt(digitalPinToInterrupt(M_101));
}

void isr_void () // isr that just resets interrupt variables
{
interrupt = false;
Serial.println ("Vamo Mangue Baja");
}

bool read_config()
{
  File foo, root;
  byte word_count = 0, letter_count = 0;
  char c = 0;
  char** aux_config = (char**)malloc(6 * sizeof(char*));
  //    17,4,3,5,6,5,2
  //    aux_config[0] = (char*)malloc(17 * sizeof(char));
  aux_config[0] = (char*)malloc(4 * sizeof(char));
  aux_config[1] = (char*)malloc(3 * sizeof(char));
  aux_config[2] = (char*)malloc(5 * sizeof(char));
  aux_config[3] = (char*)malloc(6 * sizeof(char));
  aux_config[4] = (char*)malloc(5 * sizeof(char));
  aux_config[5] = (char*)malloc(3 * sizeof(char));

  if (!SD.exists("config.txt"))
  {
    lcd.clear();
    lcd.print(" Nao ha config.");
    lcd.setCursor(0, 1);
    lcd.print("   salvas no SD");
    delay(1000);

    return false;
  }
  else
  {
    foo = SD.open("config.txt", FILE_READ);
    //        Serial.println(foo.seek(0));
    while (c != '\n')
    {
      while (c != ',')
      {
        c = foo.read();
        if (c != ',' && c != '\n')
        {
          aux_config[word_count][letter_count] = c;
          letter_count++;
        }
        else if (c == '\n')
          break;
      }
      aux_config[word_count][letter_count] = '\0';

      if (c != '\n')
        c = 0;
      letter_count = 0;
      word_count++;
    }

    foo.close();

    strcpy(new_config.vei, aux_config[0]);
    strcpy(new_config.mola_mot, aux_config[1]);
    strcpy(new_config.pre_mot, aux_config[2]);
    strcpy(new_config.peso_mot, aux_config[3]);
    strcpy(new_config.mola_mov, aux_config[4]);
    strcpy(new_config.pre_mov, aux_config[5]);

    return true;
  }
}

byte countFiles(File dir)
{
  int i = 0;
  dir.rewindDirectory();

  while (true) {

    File entry = dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }

    if (entry.isDirectory()) {
    } else {
      i++;
    }
    entry.close();
  }

  return i;
}