
#include "menu.h"
#include "stdint.h"
#include "park_dsp_lite_4x4.h"
//#include "amp_4x4_param.h"
#include "front_board.h"

//menuItem Null_Menu = {(void*)0, (void*)0, (void*)0, (void*)0, (menuType)0, (MenuParam*)0};

menuItem* selectedMenuItem;             // Curent menu item 
menuItem* previousMenuItem;             // Previous munu item 
menuItem* nextMenuItem;                 // Next menu item
menuItem NULL_ENTRY = {(void*)0, (void*)0, (void*)0, (void*)0, (menuType)0, (MenuParam*)0};
uint8_t selected_chenel = 0;

MenuParam MenuExit = {EXIT, "EXIT"};
MenuParam MenuMain = {MAIN_MENU, "MAIN SCREEN"};
MenuParam MenuVolume = {OUT_VOLUME, "VOLUME"/*, &SpeakerOUT*/};
MenuParam MenuDelay = {OUT_DELAY, "DELAY"/*, &SpeakerOUT*/};
MenuParam MenuMatrix = {MENU_MATRIX, "MATRIX"/*, &Matrix*/};
MenuParam MenuMatrixIN1 = {MATRIX_IN1, "MATRIX INPUT1"/*, &Matrix*/};
MenuParam MenuMatrixIN2 = {MATRIX_IN2, "MATRIX INPUT2"/*, &Matrix*/};
MenuParam MenuMatrixIN3 = {MATRIX_IN3, "MATRIX INPUT3"/*, &Matrix*/};
MenuParam MenuMatrixIN4 = {MATRIX_IN4, "MATRIX INPUT4"/*, &Matrix*/};
MenuParam MenuAmpInSetting = {AMP_IN_SETTING, "AMP IN SETTING"/*, &Matrix*/};
MenuParam MenuModeXLR1 = {MODE_XLR1, "MODE XLR1"/*, &Matrix*/};
MenuParam MenuModeXLR3 = {MODE_XLR3, "MODE XLR3"/*, &Matrix*/};
MenuParam MenuAesSeting = {AES_SETTING, "AES SETTING"/*, &Matrix*/};
MenuParam MenuAesDelay = {AES_DELAY, "AES DELAY"/*, &Matrix*/};
MenuParam MenuAesGain = {AES_GAIN, "AES GAIN"/*, &Matrix*/};
MenuParam MenuAnalogSeting = {ANALOG_SETTING, "ANALOG SETTING"/*, &Matrix*/};
MenuParam MenuAnalogDelay = {ANALOG_DELAY, "ANALOG DELAY"/*, &Matrix*/};
MenuParam MenuAnalogGain = {ANALOG_GAIN, "ANALOG GAIN"/*, &Matrix*/};
MenuParam MenuSourceSelect = {SOURCE_SELECT, "SOURCE SELECT"/*, &Matrix*/};
MenuParam MenuInput1 = {SOURCE_INPUT_1, "INPUT 1"/*, &Matrix*/};
MenuParam MenuInput2 = {SOURCE_INPUT_2, "INPUT 2"/*, &Matrix*/};
MenuParam MenuInput3 = {SOURCE_INPUT_3, "INPUT 3"/*, &Matrix*/};
MenuParam MenuInput4 = {SOURCE_INPUT_4, "INPUT 4"/*, &Matrix*/};
MenuParam MenuLoadSpeaker = {MENU_LOAD_SPEAKER, "LOAD SPEAKER"/*, &Matrix*/};
MenuParam MenuLoadPreset = {MENU_LOAD_PRESET, "LOAD PRESET"/*, &Matrix*/};
MenuParam MenuOutConfig = {MENU_OUT_CONFIG, "OUT CONFIG"/*, &Matrix*/};
MenuParam MenuOutConfigBridge = {MENU_OUT_CONFIG_BRIDGE, "BRIDGE"/*, &Matrix*/};
MenuParam MenuOutConfigBridgeAB = {MENU_OUT_CONFIG_BRIDGE_AB, "OUTA+B"/*, &Matrix*/};
MenuParam MenuOutConfigBridgeCD = {MENU_OUT_CONFIG_BRIDGE_CD, "OUTC+D"/*, &Matrix*/};
MenuParam MenuOutInfo = {MENU_OUT_INFO, "SPEAKER INFO"/*, &Matrix*/};
MenuParam MenuSpeakerReset = {MENU_SPEAKER_RST, "SPEAKER RESET"/*, &Matrix*/};
MenuParam MenuOutConfigWarningBridge = {MENU_OUT_CONFIG_WARNING_BRIDGE, "WARNING"/*, &Matrix*/};
MenuParam MenuUSB = {USB_MENU, "USB"/*, &Matrix*/};
MenuParam MenuSysReset = {MENU_SYS_RESET, "SYSTEM RESET"/*, &Matrix*/};
//MenuParam MenuOutConfigLink = {MENU_OUT_CONFIG_LINK, "LINK"/*, &Matrix*/};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//        CURENT                NEXT,             PREVIOUS,                 PARENT,                 CHILD,                  MENUTYPE,       PARAMTYPE     //                                 
MAKE_MENU(menu_0_0_0_0,         NULL_ENTRY,       NULL_ENTRY,               NULL_ENTRY,             menu_1_0_0_0,           MENU,           MenuMain);            //Main Screen

////First layer
//MAKE_MENU(menu_1_0_0_0,         menu_2_0_0_0,     NULL_ENTRY,               menu_0_0_0_0,           menu_0_0_0_0,           MENU,           MenuExit);            //Speaker volume
//MAKE_MENU(menu_2_0_0_0,         menu_3_0_0_0,     menu_1_0_0_0,             menu_0_0_0_0,           menu_2_1_0_0,           MENU,           MenuVolume);            //Speaker volume
//MAKE_MENU(menu_3_0_0_0,         menu_4_0_0_0,     menu_2_0_0_0,             menu_0_0_0_0,           menu_3_1_0_0,           MENU,           MenuDelay);             //Speaker delay
//MAKE_MENU(menu_4_0_0_0,         menu_5_0_0_0,     menu_3_0_0_0,             menu_0_0_0_0,           menu_4_1_0_0,           MENU,           MenuMatrix);            //Matrix
//MAKE_MENU(menu_5_0_0_0,         menu_6_0_0_0,     menu_4_0_0_0,             menu_0_0_0_0,           menu_5_1_0_0,           MENU,           MenuAmpInSetting);            //Amp IN setting
//MAKE_MENU(menu_6_0_0_0,         menu_7_0_0_0,     menu_5_0_0_0,             menu_0_0_0_0,           menu_6_1_0_0,           MENU,           MenuSourceSelect);            //Source Slelct
//MAKE_MENU(menu_7_0_0_0,         menu_8_0_0_0,     menu_6_0_0_0,             menu_0_0_0_0,           NULL_ENTRY,             MENU,           MenuPreset);            //Menu Load Preset
//MAKE_MENU(menu_8_0_0_0,         menu_9_0_0_0,     menu_7_0_0_0,             menu_0_0_0_0,           menu_8_1_0_0,           MENU,           MenuLoadSpeaker);            //Menu Load Speaker
//MAKE_MENU(menu_9_0_0_0,         menu_10_0_0_0,    menu_8_0_0_0,             menu_0_0_0_0,           menu_9_1_0_0,           MENU,           MenuOutConfig);            //Menu Out Config
//MAKE_MENU(menu_10_0_0_0,         NULL_ENTRY,       menu_9_0_0_0,            menu_0_0_0_0,           menu_0_0_0_0,           MENU,           MenuExit);            //Menu Exit

//First layer CURENT            NEXT                    PREVIOUS                PARENT                  CHILD                   MENUTYPE        PARAMTYPE
MAKE_MENU(menu_1_0_0_0,         menu_2_0_0_0,           NULL_ENTRY,             menu_0_0_0_0,           menu_0_0_0_0,           MENU,           MenuExit);              //Speaker volume
MAKE_MENU(menu_2_0_0_0,         menu_3_0_0_0,           menu_1_0_0_0,           menu_0_0_0_0,           menu_2_1_0_0,           MENU,           MenuVolume);            //Speaker volume
MAKE_MENU(menu_3_0_0_0,         menu_4_0_0_0,           menu_2_0_0_0,           menu_0_0_0_0,           menu_3_1_0_0,           MENU,           MenuDelay);             //Speaker delay
MAKE_MENU(menu_4_0_0_0,         menu_5_0_0_0,           menu_3_0_0_0,           menu_0_0_0_0,           menu_4_1_0_0,           MENU,           MenuAmpInSetting);      //Amp IN setting
MAKE_MENU(menu_5_0_0_0,         menu_6_0_0_0,           menu_4_0_0_0,           menu_0_0_0_0,           menu_5_1_0_0,           MENU,           MenuSourceSelect);      //Source Slelct
MAKE_MENU(menu_6_0_0_0,         menu_7_0_0_0,           menu_5_0_0_0,           menu_0_0_0_0,           menu_6_1_0_0,           MENU,           MenuMatrix);            //Matrix
MAKE_MENU(menu_7_0_0_0,         menu_8_0_0_0,           menu_6_0_0_0,           menu_0_0_0_0,           menu_7_1_0_0,           MENU,           MenuOutConfig);         //Menu Out Config
MAKE_MENU(menu_8_0_0_0,         menu_9_0_0_0,           menu_7_0_0_0,           menu_0_0_0_0,           menu_8_1_0_0,           MENU,           MenuLoadSpeaker);       //Menu Load Speaker
MAKE_MENU(menu_9_0_0_0,         menu_10_0_0_0,          menu_8_0_0_0,           menu_0_0_0_0,           menu_9_1_0_0,             MENU,         MenuLoadPreset);            //Menu Load Preset
MAKE_MENU(menu_10_0_0_0,        menu_11_0_0_0,          menu_9_0_0_0,        menu_0_0_0_0,              menu_10_1_0_0,           MENU,           MenuSysReset);              //Menu Exit
MAKE_MENU(menu_11_0_0_0,        NULL_ENTRY,             menu_10_0_0_0,           menu_0_0_0_0,           menu_0_0_0_0,           MENU,           MenuExit);              //Menu Exit
                                                                            
//Second layer VOLUME                                                       
MAKE_MENU(menu_2_1_0_0,         NULL_ENTRY,             NULL_ENTRY,             menu_2_0_0_0,           menu_2_0_0_0,           PARAM,          MenuVolume);            //UserEQ VOLUME Ch_n Parameter
                                                                            
//Second layer DELAY                                                        
MAKE_MENU(menu_3_1_0_0,         NULL_ENTRY,             NULL_ENTRY,             menu_3_0_0_0,           menu_3_0_0_0,           PARAM,          MenuDelay);             //UserEQ DELAY Ch_n Parameter

//Second layer Amp IN Setting
MAKE_MENU(menu_4_1_0_0,         menu_4_2_0_0,       NULL_ENTRY,             menu_4_0_0_0,           menu_4_0_0_0,           MENU,           MenuExit);                      //MODE XLR1 Menu item
MAKE_MENU(menu_4_2_0_0,         menu_4_3_0_0,       menu_4_1_0_0,           menu_4_0_0_0,           menu_4_2_1_0,           MENU,           MenuModeXLR1);                      //MODE XLR1 Menu item
MAKE_MENU(menu_4_3_0_0,         menu_4_4_0_0,       menu_4_2_0_0,           menu_4_0_0_0,           menu_4_3_1_0,           MENU,           MenuModeXLR3);                      //MODE XLR3 Menu item
MAKE_MENU(menu_4_4_0_0,         menu_4_5_0_0,       menu_4_3_0_0,           menu_4_0_0_0,           menu_4_4_1_0,           MENU,           MenuAesSeting);                      //AES Setting Menu item
MAKE_MENU(menu_4_5_0_0,         menu_4_6_0_0,       menu_4_4_0_0,           menu_4_0_0_0,           menu_4_5_1_0,           MENU,           MenuAnalogSeting);                      //ANALOG Setting Menu item
MAKE_MENU(menu_4_6_0_0,         NULL_ENTRY,         menu_4_5_0_0,           menu_4_0_0_0,           menu_4_0_0_0,           MENU,           MenuExit);                      //ANALOG Setting Menu item

//Third layer Amp IN Setting
MAKE_MENU(menu_4_2_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_4_2_0_0,           menu_4_2_0_0,           PARAM,          MenuModeXLR1);                      //MODE XLR1 Menu item
MAKE_MENU(menu_4_3_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_4_3_0_0,           menu_4_3_0_0,           PARAM,          MenuModeXLR3);                      //MODE XLR3 Menu item

//Third layer Amp IN Setting -> AES setting
MAKE_MENU(menu_4_4_1_0,         menu_4_4_2_0,       NULL_ENTRY,             menu_4_4_0_0,           menu_4_4_0_0,           MENU,           MenuExit);                      //AES Delay Menu item
MAKE_MENU(menu_4_4_2_0,         menu_4_4_3_0,       menu_4_4_1_0,           menu_4_4_0_0,           menu_4_4_2_1,           MENU,           MenuAesDelay);                      //AES Delay Menu item
MAKE_MENU(menu_4_4_3_0,         menu_4_4_4_0,       menu_4_4_2_0,           menu_4_4_0_0,           menu_4_4_3_1,           MENU,           MenuAesGain);                      //AES Gain Menu item
MAKE_MENU(menu_4_4_4_0,         NULL_ENTRY,         menu_4_4_3_0,           menu_4_4_0_0,           menu_4_4_0_0,           MENU,           MenuExit);                      //AES Gain Menu item

//Fourth layer Amp IN Setting -> AES setting -> AES Delay, AES gain
MAKE_MENU(menu_4_4_2_1,         NULL_ENTRY,         NULL_ENTRY,             menu_4_4_2_0,           menu_4_4_2_0,           PARAM,          MenuAesDelay);                           //AES Delay PARAM 
MAKE_MENU(menu_4_4_3_1,         NULL_ENTRY,         NULL_ENTRY,             menu_4_4_3_0,           menu_4_4_3_0,           PARAM,          MenuAesGain);                            //AES Gain PARAM

//Third layer Amp IN Setting -> ANALOG setting
MAKE_MENU(menu_4_5_1_0,         menu_4_5_2_0,       NULL_ENTRY,             menu_4_5_0_0,           menu_4_5_0_0,           MENU,           MenuExit);                      //ANALOG Delay 
MAKE_MENU(menu_4_5_2_0,         menu_4_5_3_0,       menu_4_5_1_0,           menu_4_5_0_0,           menu_4_5_2_1,           MENU,           MenuAnalogDelay);                      //ANALOG Delay 
MAKE_MENU(menu_4_5_3_0,         menu_4_5_4_0,       menu_4_5_2_0,           menu_4_5_0_0,           menu_4_5_3_1,           MENU,           MenuAnalogGain);                      //ANALOG Gain
MAKE_MENU(menu_4_5_4_0,         NULL_ENTRY,         menu_4_5_3_0,           menu_4_5_0_0,           menu_4_5_0_0,           MENU,           MenuExit);                      //ANALOG Gain

//Fourth layer Amp IN Setting -> ANALOG setting -> ANALOG Delay, ANALOG gain
MAKE_MENU(menu_4_5_2_1,         NULL_ENTRY,         NULL_ENTRY,             menu_4_5_2_0,           menu_4_5_2_0,           PARAM,          MenuAnalogDelay);                           //ANALOG Delay PARAM 
MAKE_MENU(menu_4_5_3_1,         NULL_ENTRY,         NULL_ENTRY,             menu_4_5_3_0,           menu_4_5_3_0,           PARAM,          MenuAnalogGain);                            //ANALOG Gain PARAM

//Second layer Source Select -> IN1-IN4
MAKE_MENU(menu_5_1_0_0,         menu_5_2_0_0,       NULL_ENTRY,             menu_5_0_0_0,           menu_5_0_0_0,           MENU,           MenuExit);                      //Menu Source Select IN1
MAKE_MENU(menu_5_2_0_0,         menu_5_3_0_0,       menu_5_1_0_0,           menu_5_0_0_0,           menu_5_2_1_0,           MENU,           MenuInput1);                      //Menu Source Select IN1
MAKE_MENU(menu_5_3_0_0,         menu_5_4_0_0,       menu_5_2_0_0,           menu_5_0_0_0,           menu_5_3_1_0,           MENU,           MenuInput2);                      //Menu Source Select IN2
MAKE_MENU(menu_5_4_0_0,         menu_5_5_0_0,       menu_5_3_0_0,           menu_5_0_0_0,           menu_5_4_1_0,           MENU,           MenuInput3);                      //Menu Source Select IN3
MAKE_MENU(menu_5_5_0_0,         menu_5_6_0_0,       menu_5_4_0_0,           menu_5_0_0_0,           menu_5_5_1_0,           MENU,           MenuInput4);                      //Menu Source Select IN4
MAKE_MENU(menu_5_6_0_0,         NULL_ENTRY,         menu_5_5_0_0,           menu_5_0_0_0,           menu_5_0_0_0,           MENU,           MenuExit);                      //Menu Source Select IN4

//Third layer Source Select -> IN1-IN4
MAKE_MENU(menu_5_2_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_5_2_0_0,           menu_5_2_0_0,           PARAM,          MenuInput1);                      //Param Source Select IN1
MAKE_MENU(menu_5_3_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_5_3_0_0,           menu_5_3_0_0,           PARAM,          MenuInput2);                      //Param Source Select IN1
MAKE_MENU(menu_5_4_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_5_4_0_0,           menu_5_4_0_0,           PARAM,          MenuInput3);                      //Param Source Select IN1
MAKE_MENU(menu_5_5_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_5_5_0_0,           menu_5_5_0_0,           PARAM,          MenuInput4);                      //Param Source Select IN1
                                                                            
//Second layer MATRIX                                                       
MAKE_MENU(menu_6_1_0_0,         menu_6_2_0_0,       NULL_ENTRY,             menu_6_0_0_0,           menu_6_0_0_0,           MENU,           MenuExit);                           //Matrix Exit Menu item
MAKE_MENU(menu_6_2_0_0,         menu_6_3_0_0,       menu_6_1_0_0,           menu_6_0_0_0,           menu_6_2_1_0,           MENU,           MenuMatrixIN1);                      //Matrix IN1 Menu item
MAKE_MENU(menu_6_3_0_0,         menu_6_4_0_0,       menu_6_2_0_0,           menu_6_0_0_0,           menu_6_3_1_0,           MENU,           MenuMatrixIN2);                      //Matrix IN2 Menu item
MAKE_MENU(menu_6_4_0_0,         menu_6_5_0_0,       menu_6_3_0_0,           menu_6_0_0_0,           menu_6_4_1_0,           MENU,           MenuMatrixIN3);                      //Matrix IN3 Menu item
MAKE_MENU(menu_6_5_0_0,         menu_6_6_0_0,       menu_6_4_0_0,           menu_6_0_0_0,           menu_6_5_1_0,           MENU,           MenuMatrixIN4);                      //Matrix IN4 Menu item
MAKE_MENU(menu_6_6_0_0,         NULL_ENTRY,         menu_6_5_0_0,           menu_6_0_0_0,           menu_6_0_0_0,           MENU,           MenuExit);                      //Matrix Exit Menu item

//Third layer MATRIX
MAKE_MENU(menu_6_2_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_6_2_0_0,           menu_6_2_0_0,           PARAM,          MenuMatrixIN1);                      //Matrix IN1 Ch_n Parameter
MAKE_MENU(menu_6_3_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_6_3_0_0,           menu_6_3_0_0,           PARAM,          MenuMatrixIN2);                      //Matrix IN1 Ch_n Parameter
MAKE_MENU(menu_6_4_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_6_4_0_0,           menu_6_4_0_0,           PARAM,          MenuMatrixIN3);                      //Matrix IN1 Ch_n Parameter
MAKE_MENU(menu_6_5_1_0,         NULL_ENTRY,         NULL_ENTRY,             menu_6_5_0_0,           menu_6_5_0_0,           PARAM,          MenuMatrixIN4);                      //Matrix IN1 Ch_n Parameter

////Second layer Out Config 
////      CURENT                  NEXT,             PREVIOUS,                 PARENT,                 CHILD,                  MENUTYPE,       PARAMTYPE
//MAKE_MENU(menu_7_1_0_0,         menu_7_2_0_0,       NULL_ENTRY,             menu_7_0_0_0,           menu_7_0_0_0,             MENU,          MenuExit);                         //Menu Out Config Exit
//MAKE_MENU(menu_7_2_0_0,         menu_7_3_0_0,       menu_7_1_0_0,          menu_7_0_0_0,            menu_7_2_1_0,             MENU,              MenuOutConfigBridge);          //Menu Out Config Bridge
//MAKE_MENU(menu_7_3_0_0,         NULL_ENTRY,         menu_7_2_0_0,             menu_7_0_0_0,           menu_7_0_0_0,             MENU,          MenuExit);                       //Menu Out Config Exit

//Second layer Out Config 
//      CURENT                  NEXT,             PREVIOUS,                 PARENT,                 CHILD,                  MENUTYPE,       PARAMTYPE
MAKE_MENU(menu_7_1_0_0,         menu_7_2_0_0,       NULL_ENTRY,             menu_7_0_0_0,           menu_7_0_0_0,             MENU,          MenuExit);                                 //Menu Out Config Exit
MAKE_MENU(menu_7_2_0_0,         menu_7_3_0_0,       menu_7_1_0_0,               menu_7_0_0_0,            menu_7_2_1_0,             MENU,              MenuOutConfigBridgeAB);           //Menu Out Config Bridge
MAKE_MENU(menu_7_3_0_0,         menu_7_4_0_0,       menu_7_2_0_0,               menu_7_0_0_0,            menu_7_3_1_0,             MENU,              MenuOutConfigBridgeCD);           //Menu Out Config Bridge
MAKE_MENU(menu_7_4_0_0,         menu_7_5_0_0,       menu_7_3_0_0,               menu_7_0_0_0,            menu_7_4_1_0,             MENU,              MenuOutInfo);                     //Menu Out info
MAKE_MENU(menu_7_5_0_0,         menu_7_6_0_0,       menu_7_4_0_0,               menu_7_0_0_0,            menu_7_5_1_0,             MENU,              MenuSpeakerReset);                //Menu Speaker Reset

MAKE_MENU(menu_7_6_0_0,         NULL_ENTRY,         menu_7_5_0_0,             menu_7_0_0_0,           menu_7_0_0_0,             MENU,          MenuExit);                       //Menu Out Config Exit

//Third layer Out Config -> Bridge -> OUT A+B PARAM
MAKE_MENU(menu_7_2_1_0,         NULL_ENTRY,       NULL_ENTRY,               menu_7_2_0_0,           menu_7_2_0_0,             PARAM,          MenuOutConfigBridgeAB);                   //Param Out Config Bridge A+B
MAKE_MENU(menu_7_3_1_0,         NULL_ENTRY,       NULL_ENTRY,               menu_7_3_0_0,           menu_7_3_0_0,             PARAM,          MenuOutConfigBridgeCD);           //Param Out Config Bridge C+D
MAKE_MENU(menu_7_4_1_0,         NULL_ENTRY,       NULL_ENTRY,               menu_7_4_0_0,           menu_7_4_0_0,             PARAM,          MenuOutInfo);                     //Param Out Info
MAKE_MENU(menu_7_5_1_0,         NULL_ENTRY,       NULL_ENTRY,               menu_7_5_0_0,           menu_7_5_0_0,             PARAM,          MenuSpeakerReset);                     //Param Speaker Reset


MAKE_MENU(menu_7_2_5_1,         NULL_ENTRY,       NULL_ENTRY,               menu_7_3_0_0,           menu_7_3_0_0,             PARAM,          MenuOutConfigWarningBridge);                   //Param Out Config Bridge Warning
MAKE_MENU(menu_20_0_0_0,         NULL_ENTRY,       NULL_ENTRY,               menu_0_0_0_0,           menu_0_0_0_0,             MENU,          MenuUSB);                   //Menu USB Conected

//Second layer Load Speaker 
MAKE_MENU(menu_8_1_0_0,         NULL_ENTRY,       NULL_ENTRY,             menu_8_0_0_0,           menu_8_0_0_0,             PARAM,          MenuLoadSpeaker);                   //Param Load Speaker

//Second layer Load Preset
MAKE_MENU(menu_9_1_0_0,         NULL_ENTRY,       NULL_ENTRY,           menu_9_0_0_0,           menu_9_0_0_0,         PARAM,           MenuLoadPreset);                      //Menu Source Select IN1

MAKE_MENU(menu_10_1_0_0,         NULL_ENTRY,       NULL_ENTRY,          menu_10_0_0_0,          menu_10_0_0_0,        PARAM,           MenuSysReset);                      //System Reset


void Menu_Init()
{
  selectedMenuItem = &menu_0_0_0_0;
  previousMenuItem = selectedMenuItem->Previous;
  nextMenuItem = selectedMenuItem->Next;
}

void menuChange(menuItem* NewMenu)
{
  if ((void*)NewMenu == (void*)&NULL_ENTRY) return;
 
  selectedMenuItem = NewMenu;
  previousMenuItem = selectedMenuItem->Previous;
  nextMenuItem = selectedMenuItem->Next;
  
  if(selectedMenuItem != &menu_0_0_0_0);
}

void Menu_Next()
{
  menuChange(NEXT);
}


void Menu_Prev()
{
  menuChange(PREVIOUS);
}

void Menu_Child()
{
  menuChange(CHILD);
}

void Menu_Parent()
{
  menuChange(PARENT);
}

void Menu_Change(ControlStateType* control)
{
  switch(control->Comand)
  {
    case INC : Menu_Next(); break;
    case DEC :  Menu_Prev(); break;
    case ENTER : Menu_Child();  break;
    case ESC : Menu_Parent(); break;
  }
}

void Menu_Warning()
{
  selectedMenuItem = &menu_7_2_5_1;
  previousMenuItem = selectedMenuItem->Previous;
  nextMenuItem = selectedMenuItem->Next;
}

void Menu_USB()
{
  selectedMenuItem = &menu_20_0_0_0;
  previousMenuItem = selectedMenuItem->Previous;
  nextMenuItem = selectedMenuItem->Next;
}



