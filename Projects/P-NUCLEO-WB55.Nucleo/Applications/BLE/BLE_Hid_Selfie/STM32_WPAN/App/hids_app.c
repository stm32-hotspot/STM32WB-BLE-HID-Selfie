/**
  ******************************************************************************
  * @file    hids_app.c
  * @author  MCD Application Team
  * @brief   Human Interface Device Service Application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */



/* Includes ------------------------------------------------------------------*/
#include "app_common.h"

#include "dbg_trace.h"
#include "app_ble.h"
#include "ble.h"
#include "hids_app.h"
#include "stm32_seq.h"
#include <time.h>
#include "hids_menu.h"


/* Private typedef -----------------------------------------------------------*/
#if((BLE_CFG_HIDS_INPUT_REPORT_NB != 0) || (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0) || (BLE_CFG_HIDS_MOUSE_DEVICE != 0))
typedef struct
{
#if(BLE_CFG_HIDS_INPUT_REPORT_NB != 0)
  uint8_t ReportNotificationEnabled[BLE_CFG_HIDS_INPUT_REPORT_NB];
#endif
#if(BLE_CFG_HIDS_KEYBOARD_DEVICE != 0)
  uint8_t KeyboardInputNotificationEnabled;
#endif
#if(BLE_CFG_HIDS_MOUSE_DEVICE != 0)
  uint8_t MouseInputNotificationEnabled;
#endif
} HIDSAPP_Context_t;
#endif

typedef struct
{
  uint8_t buttons;
  int8_t x;
  int8_t y;
  int8_t wheel;
} mouse_report_t;

#ifdef ST_SELFIE
/* Keybaord key data*/
typedef struct
{
  uint8_t ReportID;
  uint8_t Data_0;
  uint8_t Data_1;
  uint8_t Data_2;
  uint8_t Data_3;
  uint8_t Data_4;
  uint8_t Data_5;
  uint8_t Data_6;
  uint8_t Data_7;
} keyboard_report_t;

/* Multimedia key data*/
typedef struct
{
  uint8_t ReportID;
  uint8_t Data_0;
  uint8_t Data_1;
  uint8_t Data_2;
  uint8_t Data_3;
} multimedia_report_t;
#endif

#ifdef ST_SELFIE
/* Generic Keycode */
typedef enum
{
  KEY_A         = 0x04,
  KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K,
  KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
  KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, 
  
  KEY_1         = 0x1E,
  KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
} T_KEYBOARD_KEYCODE;

/* Multimedia Keycode */
typedef enum
{
  MM_STOP       = 0x00B7,
  MM_PLAYPAUSE  = 0x00CD,
  MM_MUTE       = 0x00E2,
  MM_VOLUP      = 0x00E9,
  MM_VOLDN      = 0x00EA,
  MM_AC_SEARCH  = 0x0221,
  MM_AC_HOME    = 0x0223,
  MM_AC_BACK    = 0x0224,
} T_MULTIMEDIA_KEYCODE;
#endif

/* Private defines -----------------------------------------------------------*/
#define MOUSE_REPORT_SIZE       52

#define ENABLED         1
#define DISABLED        0

  
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#ifdef ST_SELFIE
static uint8_t report_selfie[] =
{
    0x05, 0x01,                   /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x06,                   /* USAGE (Keyboard) */
    0xa1, 0x01,                   /* COLLECTION (Application) */
    0x05, 0x07,                   /*   USAGE_PAGE (Keyboard) */
    0x09, 0x06,                   /*   USAGE (Keyboard) */
    0xa1, 0x01,                   /*   COLLECTION (Application) */
    0x85, KEYBOARD_REPORT_ID,     /*     REPORT ID (0x01) */
    0x95, 0x08,                   /*     REPORT_COUNT (8) */
    0x75, 0x08,                   /*     REPORT_SIZE (8) */
    0x15, 0x00,                   /*     LOGICAL_MINIMUM (0) */
    0x25, 0xff,                   /*     LOGICAL_MAXIMUM (255) */
    0x19, 0x00,                   /*     USAGE_MINIMUM (0) */
    0x29, 0xff,                   /*     USAGE_MAXIMUM (255) */
    0x81, 0x00,                   /*     INPUT (Data,Var,Abs) */
    0xc0,                         /*   END_COLLECTION (Application) */

    0x05, 0x0c,                   /*   USAGE_PAGE (Consumer) */
    0x09, 0x01,                   /*   USAGE (Consumer Control) */
    0xa1, 0x01,                   /*   COLLECTION (Application) */
    0x85, CONSUMERCTRL_REPORT_ID, /*     REPORT ID (0x02) */
    0x95, 0x02,                   /*     REPORT_COUNT (2) */
    0x75, 0x10,                   /*     REPORT_SIZE (16) */
    0x15, 0x00,                   /*     LOGICAL_MINIMUM (0) */
    0x26, 0x9C, 0x02,             /*     LOGICAL_MAXIMUM (1) */
    0x19, 0x00,                   /*     USAGE_MINIMUM  */
    0x2A, 0x9C, 0x02,             /*     USAGE_MAXIMUM  */
    0x81, 0x00,                   /*     INPUT (Data,Var,Abs) */
    0xc0,                         /*   END_COLLECTION (Application) */
    0xc0,                         /* END_COLLECTION (Application) */
};
#define SELFIE_REPORT_SIZE  (sizeof(report_selfie)/sizeof(uint8_t))
#else
static uint8_t report_mouse[MOUSE_REPORT_SIZE] =
{
  0x05, 0x01,         /* USAGE_PAGE (Generic Desktop) */
  0x09, 0x02,         /* USAGE (Mouse) */
  0xa1, 0x01,         /* COLLECTION (Application) */
  0x09, 0x01,         /*   USAGE (Pointer) */
  0xa1, 0x00,         /*   COLLECTION (Physical) */
  0x05, 0x09,         /*     USAGE_PAGE (Button) */
  0x19, 0x01,         /*     USAGE_MINIMUM (Button 1) */
  0x29, 0x03,         /*     USAGE_MAXIMUM (Button 3) */
  0x15, 0x00,         /*     LOGICAL_MINIMUM (0) */
  0x25, 0x01,         /*     LOGICAL_MAXIMUM (1) */
  0x95, 0x03,         /*     REPORT_COUNT (3) */
  0x75, 0x01,         /*     REPORT_SIZE (1) */
  0x81, 0x02,         /*     INPUT (Data,Var,Abs) */
  0x95, 0x01,         /*     REPORT_COUNT (1) */
  0x75, 0x05,         /*     REPORT_SIZE (5) */
  0x81, 0x03,         /*     INPUT (Cnst,Var,Abs) */
  0x05, 0x01,         /*     USAGE_PAGE (Generic Desktop) */
  0x09, 0x30,         /*     USAGE (X) */
  0x09, 0x31,         /*     USAGE (Y) */
  0x09, 0x38,         /*     USAGE (Wheel) */
  0x15, 0x81,         /*     LOGICAL_MINIMUM (-127) */
  0x25, 0x7f,         /*     LOGICAL_MAXIMUM (127) */
  0x75, 0x08,         /*     REPORT_SIZE (8) */
  0x95, 0x03,         /*     REPORT_COUNT (3) */
  0x81, 0x06,         /*     INPUT (Data,Var,Rel) */
  0xc0,               /*   END_COLLECTION (Physical) */
  0xc0,               /* END_COLLECTION (Application) */
};
#endif

/**
 * START of Section BLE_APP_CONTEXT
 */

#if((BLE_CFG_HIDS_INPUT_REPORT_NB != 0) || (BLE_CFG_HIDS_KEYBOARD_DEVICE != 0) || (BLE_CFG_HIDS_MOUSE_DEVICE != 0))
HIDSAPP_Context_t HIDSAPP_Context[BLE_CFG_HIDS_NUMBER];
#endif

/**
 * END of Section BLE_APP_CONTEXT
 */

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/
/* Private functions ----------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/
void HIDS_Notification(HIDS_App_Notification_evt_t *pNotification)
{
  switch(pNotification->HIDS_Evt_Opcode)
  {

#if(BLE_CFG_HIDS_INPUT_REPORT_NB != 0)
    case HIDS_REPORT_NOTIFICATION_ENABLED:
      {
        BLE_DBG_APP_MSG("HIDS_REPORT_NOTIFICATION_ENABLED\n");
        HIDSAPP_Context[pNotification->Index].ReportNotificationEnabled[pNotification->Index] = ENABLED;
      }
      break;

    case HIDS_REPORT_NOTIFICATION_DISABLED:
      {
        BLE_DBG_APP_MSG("HIDS_REPORT_NOTIFICATION_DISABLED\n");
        HIDSAPP_Context[pNotification->Index].ReportNotificationEnabled[pNotification->Index] = DISABLED;
      }
      break;
#endif
      
#if(BLE_CFG_HIDS_KEYBOARD_DEVICE != 0)
    case HIDS_KEYB_INPUT_NOTIFY_ENABLED:
      {
        BLE_DBG_APP_MSG("HIDS_KEYB_INPUT_NOTIFY_ENABLED\n");
        HIDSAPP_Context[pNotification->Index].KeyboardInputNotificationEnabled = ENABLED;
      }
      break;

    case HIDS_KEYB_INPUT_NOTIFY_DISABLED:
      {
        BLE_DBG_APP_MSG("HIDS_KEYB_INPUT_NOTIFY_DISABLED\n");
        HIDSAPP_Context[pNotification->Index].KeyboardInputNotificationEnabled = DISABLED;
      }
      break;
#endif

#if(BLE_CFG_HIDS_MOUSE_DEVICE != 0)          
    case HIDS_MOUSE_INPUT_NOTIFY_ENABLED:
      {
        BLE_DBG_APP_MSG("HIDS_MOUSE_INPUT_NOTIFY_ENABLED\n");
        HIDSAPP_Context[pNotification->Index].MouseInputNotificationEnabled = ENABLED;
      }
      break;

    case HIDS_MOUSE_INPUT_NOTIFY_DISABLED:
      {
        BLE_DBG_APP_MSG("HIDS_MOUSE_INPUT_NOTIFY_DISABLED\n");
        HIDSAPP_Context[pNotification->Index].MouseInputNotificationEnabled = DISABLED;
      }
      break;
#endif

#if(BLE_CFG_HIDS_OUTPUT_REPORT_NB != 0)
    case HIDS_OUTPUT_REPORT:
      {
        uint8_t i;
        
        BLE_DBG_APP_MSG("HIDS_OUTPUT_REPORT\n");
        BLE_DBG_HIDS_MSG("HID Instance %d Report %d \n", 
                          pNotification->Instance,
                          pNotification->Index); 
    
        for(i = 0; i < pNotification->ReportLength; i++)
          BLE_DBG_HIDS_MSG("Report[%d] 0x%X \n",
                           i,
                           pNotification->pReport[i]);
      }
      break;
#endif
    
#if((BLE_CFG_HIDS_MOUSE_DEVICE != 0) && (BLE_CFG_HIDS_MOUSE_INPUT_WRITE != 0))
    case HIDS_MOUSE_INPUT_REPORT:
      {
        uint8_t i;
        
        BLE_DBG_APP_MSG("HIDS_MOUSE_INPUT_REPORT\n");
        BLE_DBG_HIDS_MSG("HID Instance %d Report %d \n", 
                          pNotification->Instance,
                          pNotification->Index); 
    
        for(i = 0; i < pNotification->ReportLength; i++)
          BLE_DBG_HIDS_MSG("Report[%d] 0x%X \n",
                           i,
                           pNotification->pReport[i]);
      }
      break;
#endif
      
#if((BLE_CFG_HIDS_KEYBOARD_DEVICE != 0) && (BLE_CFG_HIDS_KEYBOARD_INPUT_WRITE != 0))
    case HIDS_KEYBOARD_INPUT_REPORT:
      {
        uint8_t i;
        
        BLE_DBG_APP_MSG("HIDS_KEYBOARD_INPUT_REPORT\n");
        BLE_DBG_HIDS_MSG("HID Instance %d Report %d \n", 
                          pNotification->Instance,
                          pNotification->Index); 
    
        for(i = 0; i < pNotification->ReportLength; i++)
          BLE_DBG_HIDS_MSG("Report[%d] 0x%X \n",
                           i,
                           pNotification->pReport[i]);
      }
      break;

    case HIDS_KEYBOARD_OUTPUT_REPORT:
      {
        uint8_t i;
        
        BLE_DBG_APP_MSG("HIDS_KEYBOARD_OUTPUT_REPORT\n");
        BLE_DBG_HIDS_MSG("HID Instance %d Report %d \n", 
                          pNotification->Instance,
                          pNotification->Index); 
    
        for(i = 0; i < pNotification->ReportLength; i++)
          BLE_DBG_HIDS_MSG("Report[%d] 0x%X \n",
                           i,
                           pNotification->pReport[i]);
      }
      break;
#endif
      
    default:
      break;
  }

  return;
}


void HIDSAPP_Init(void)
{
  tBleStatus result = BLE_STATUS_INVALID_PARAMS;

  UTIL_SEQ_RegTask( 1<< CFG_TASK_HID_UPDATE_REQ_ID, UTIL_SEQ_RFU, HIDSAPP_Profile_UpdateChar );

#ifdef ST_SELFIE
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SW1_ID, UTIL_SEQ_RFU, Process_GenericKeyboard_Demo);
  UTIL_SEQ_RegTask( 1<< CFG_TASK_SW2_ID, UTIL_SEQ_RFU, Process_ConsumerControl_Demo);

  BLE_DBG_APP_MSG("Report Map Size (%d)\n", SELFIE_REPORT_SIZE);
  result = HIDS_Update_Char(REPORT_MAP_CHAR_UUID, 
                            0, 
                            0, 
                            SELFIE_REPORT_SIZE,
                            (uint8_t *)& report_selfie);

  if( result == BLE_STATUS_SUCCESS )
  {
    BLE_DBG_APP_MSG("Selfie Report Map Successfully Sent (%d)\n", SELFIE_REPORT_SIZE);
  }
  else 
  {
    BLE_DBG_APP_MSG("Sending of Selfie Report Map Failed error 0x%X\n", result);
  }
#else
  result = HIDS_Update_Char(REPORT_MAP_CHAR_UUID, 
                            0, 
                            0, 
                            MOUSE_REPORT_SIZE,
                            (uint8_t *)&report_mouse);

  if( result == BLE_STATUS_SUCCESS )
  {
    BLE_DBG_APP_MSG("Report Map Successfully Sent\n");
  }
  else 
  {
    BLE_DBG_APP_MSG("Sending of Report Map Failed error 0x%X\n", result);
  }
#endif
}


/**
 * @brief  Alert Notification Application service update characteristic
 * @param  None
 * @retval None
 */
void HIDSAPP_Profile_UpdateChar(void)
{
  uint8_t action_type;
  mouse_report_t mouse_report;
  
  HIDS_Menu(&action_type, (uint8_t *)&mouse_report);
    
  if(action_type == 1)
  {
    tBleStatus result = BLE_STATUS_INVALID_PARAMS;

    result = HIDS_Update_Char(REPORT_CHAR_UUID, 
                              0, 
                              0, 
                              sizeof(mouse_report_t),
                              (uint8_t *)& mouse_report);

    if( result == BLE_STATUS_SUCCESS )
    {
      BLE_DBG_APP_MSG("Mouse Report 0x%x %d %d %d Successfully Sent\n",
                       mouse_report.buttons,
                       mouse_report.x,
                       mouse_report.y,
                       mouse_report.wheel);
    }
    else 
    {
      BLE_DBG_APP_MSG("Sending of Mouse Report Failed error 0x%X\n", result);
    }
  }
}

#ifdef ST_SELFIE
void  Process_GenericKeyboard_Demo(void)
{
  tBleStatus result = BLE_STATUS_INVALID_PARAMS;

  keyboard_report_t keyboard_keydata = {0x00, };
  T_KEYBOARD_KEYCODE keycode = KEY_1;

  BLE_DBG_APP_MSG("++Process_GenericKeyboard_Demo\n");

  //For press key
  keyboard_keydata.ReportID = KEYBOARD_REPORT_ID;
  keyboard_keydata.Data_2 = keycode;
  result = HIDS_Update_Char(REPORT_CHAR_UUID, 
                            0, 
                            0, 
                            sizeof(keyboard_report_t),
                            (uint8_t *)& keyboard_keydata);

  if( result == BLE_STATUS_SUCCESS )
  {
    BLE_DBG_APP_MSG("Keyboard Report(P) (Report ID : 0x%02X), %02X %02X %02X %02X %02X %02X %02X %02X Successfully Sent\n",
      keyboard_keydata.ReportID, keyboard_keydata.Data_0, keyboard_keydata.Data_1, keyboard_keydata.Data_2, keyboard_keydata.Data_3,
      keyboard_keydata.Data_4, keyboard_keydata.Data_5, keyboard_keydata.Data_6, keyboard_keydata.Data_7);
  }
  else 
  {
    BLE_DBG_APP_MSG("Sending of Keyboard Report(P) Failed error 0x%X\n", result);
  }

  //For release key
  keyboard_keydata.Data_2 = 0x00;
  result = HIDS_Update_Char(REPORT_CHAR_UUID, 
                            0, 
                            0, 
                            sizeof(keyboard_report_t),
                            (uint8_t *)& keyboard_keydata);

  if( result == BLE_STATUS_SUCCESS )
  {
    BLE_DBG_APP_MSG("Keyboard Report(P) (Report ID : 0x%02X), %02X %02X %02X %02X %02X %02X %02X %02X Successfully Sent\n",
      keyboard_keydata.ReportID, keyboard_keydata.Data_0, keyboard_keydata.Data_1, keyboard_keydata.Data_2, keyboard_keydata.Data_3,
      keyboard_keydata.Data_4, keyboard_keydata.Data_5, keyboard_keydata.Data_6, keyboard_keydata.Data_7);
  }
  else 
  {
    BLE_DBG_APP_MSG("Sending of Keyboard Report(R) Failed error 0x%X\n", result);
  }
}

void  Process_ConsumerControl_Demo(void)
{
  tBleStatus result = BLE_STATUS_INVALID_PARAMS;

  multimedia_report_t multimedia_keydata  = {0x00, };
  T_MULTIMEDIA_KEYCODE keycode = MM_VOLDN;

  BLE_DBG_APP_MSG("++Process_ConsumerControl_Demo\n");

  //For press key
  multimedia_keydata.ReportID = CONSUMERCTRL_REPORT_ID;
  multimedia_keydata.Data_0 = (uint8_t)(keycode & 0xff);
  multimedia_keydata.Data_1 = (uint8_t)((keycode >> 8) & 0xff);
  result = HIDS_Update_Char(REPORT_CHAR_UUID, 
                            0, 
                            0, 
                            sizeof(multimedia_report_t),
                            (uint8_t *)& multimedia_keydata);

  if( result == BLE_STATUS_SUCCESS )
  {
    BLE_DBG_APP_MSG("Multimedia Report(P) (Report ID : 0x%02X), %02X %02X %02X %02X Successfully Sent\n",
      multimedia_keydata.ReportID, multimedia_keydata.Data_0, multimedia_keydata.Data_1, multimedia_keydata.Data_2, multimedia_keydata.Data_3);
  }
  else 
  {
    BLE_DBG_APP_MSG("Sending of Multimedia Report(R) Failed error 0x%X\n", result);
  }

  //For release key
  multimedia_keydata.Data_0 = 0x00;
  multimedia_keydata.Data_1 = 0x00;
  result = HIDS_Update_Char(REPORT_CHAR_UUID, 
                            0, 
                            0, 
                            sizeof(multimedia_report_t),
                            (uint8_t *)& multimedia_keydata);

  if( result == BLE_STATUS_SUCCESS )
  {
    BLE_DBG_APP_MSG("Multimedia Report(P) (Report ID : 0x%02X), %02X %02X %02X %02X Successfully Sent\n",
      multimedia_keydata.ReportID, multimedia_keydata.Data_0, multimedia_keydata.Data_1, multimedia_keydata.Data_2, multimedia_keydata.Data_3);
  }
  else 
  {
    BLE_DBG_APP_MSG("Sending of Multimedia Report(R) Failed error 0x%X\n", result);
  }
}
#endif
