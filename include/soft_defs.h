#ifndef _SOFT_DEF_
#define _SOFT_DEF_

#include <Arduino.h>

typedef enum PROGMEM {BEGIN, MENU, RUN, CONFIG}  state;
typedef enum PROGMEM {SHOW, GERAL, VEICULO, MOTORA, MOVIDA, SAVE_CONFIG} config_state;
typedef enum PROGMEM {START_, WAIT_30, WAIT_100, WAIT_101, END_RUN, SAVE_RUN} run_state;
typedef enum PROGMEM {GERAL_C, PRE, MOLA, PESO} cvt_state;

typedef struct
{
  String data = "11102000";
  char vei[4] = "MB1";
  char mola_mot[3] = "2X";
  char pre_mot[5] = "00mm";
  char peso_mot[6] = "62.0g";
  char mola_mov[5] = "VERM";
  char pre_mov[3] = "5";
} Config;

//state ss = BEGIN;
//config_state ss_c = SHOW;
//run_state ss_r = START_;
//cvt_state ss_cvt = GERAL_C;
//Config new_config;

byte ss = BEGIN;
byte ss_c = SHOW;
byte ss_r = START_;
byte ss_cvt = GERAL_C;
Config new_config;

bool interrupt = false;
unsigned long int t_curr = 0;
volatile unsigned long int t_30 = 0, t_100 = 0, t_101 = 0;
float vel = 0;
String str_30 = "00:000",
       str_100 = "00:000",
       str_101 = "00:000",
       str_vel = "00.00 km/h";
byte pot_sel = 0, old_pot = -1;
byte num_sel = 0, old_num = -1, clk_cnt = 0;
byte curr_pos = 0;
byte pos[4];
char c = 0;

#endif