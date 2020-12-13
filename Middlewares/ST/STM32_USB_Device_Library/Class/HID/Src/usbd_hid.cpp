/**
  ******************************************************************************
  * @file    usbd_hid.c
  * @author  MCD Application Team
  * @brief   This file provides the HID core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                HID Class  Description
  *          ===================================================================
  *           This module manages the HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - The Mouse protocol
  *             - Usage Page : Generic Desktop
  *             - Usage : Joystick
  *             - Collection : Application
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      http://www.st.com/SLA0044
  *
  ******************************************************************************
  */

  /* BSPDependencies
  - "stm32xxxxx_{eval}{discovery}{nucleo_144}.c"
  - "stm32xxxxx_{eval}{discovery}_io.c"
  EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "circular_buffers.hpp"
#include "usbd_hid.h"
#include "usbd_ctlreq.h"
#include "usb_hid_descriptors.h"


static uint8_t  USBD_HID_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx);

static uint8_t  USBD_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);

static uint8_t  USBD_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

static uint8_t  *USBD_HID_GetFSCfgDesc (uint16_t *length);

static uint8_t  *USBD_HID_GetHSCfgDesc (uint16_t *length);

static uint8_t  *USBD_HID_GetOtherSpeedCfgDesc (uint16_t *length);

static uint8_t  *USBD_HID_GetDeviceQualifierDesc (uint16_t *length);

static uint8_t  USBD_HID_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_HID_EP0_RxReady (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_HID_EP0_TxSent(USBD_HandleTypeDef *pdev);

static uint8_t  USBD_HID_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);


//uint8_t USB_HID_Send_Next_Report(USBD_HandleTypeDef *pdev);

extern HIDContinuousBlockCircularBuffer hid_report_buf;

USBD_ClassTypeDef  USBD_HID =
{
  USBD_HID_Init,
  USBD_HID_DeInit,
  USBD_HID_Setup,
  NULL, // USBD_HID_EP0_TxSent, /*EP0_TxSent*/
  USBD_HID_EP0_RxReady, // USBD_HID_EP0_RxReady, /*EP0_RxReady*/
  USBD_HID_DataIn, /*DataIn*/
  USBD_HID_DataOut, /*DataOut*/
  NULL, /*SOF */
  NULL,
  NULL,
  USBD_HID_GetHSCfgDesc,
  USBD_HID_GetFSCfgDesc,
  USBD_HID_GetOtherSpeedCfgDesc,
  USBD_HID_GetDeviceQualifierDesc,
};


bool usbd_hid_callback_reg_state = false;
HandleHID_Rx_TypeDef Handle_HID_RX_Func;
uint8_t USBD_HID_Register_EP0RX_Callback(HandleHID_Rx_TypeDef a){
	Handle_HID_RX_Func = a;
	usbd_hid_callback_reg_state = true;
}

/**
  * @}
  */

/** @defgroup USBD_HID_Private_Functions
  * @{
  */

extern void HandleHIDOutputMsg(const uint8_t* buf, uint8_t len);

/**
  * @brief  USBD_HID_Init
  *         Initialize the HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */

static uint8_t  USBD_HID_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  /* Open EP IN */
  USBD_LL_OpenEP(pdev, HID_EPIN_ADDR, USBD_EP_TYPE_INTR, HID_EPIN_SIZE);
  pdev->ep_in[HID_EPIN_ADDR & 0xFU].is_used = 1U;

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev, HID_EPOUT_ADDR, USBD_EP_TYPE_INTR, HID_EPOUT_SIZE);
  pdev->ep_out[HID_EPOUT_ADDR & 0xFU].is_used = 1U;

  pdev->pClassData = USBD_malloc(sizeof (USBD_HID_HandleTypeDef));


  if (pdev->pClassData == NULL)
  {
    return USBD_FAIL;
  }

  ((USBD_HID_HandleTypeDef *)pdev->pClassData)->state = HID_IDLE;
  /* Prepare Out endpoint to receive 1st packet */

  USBD_LL_PrepareReceive(pdev, HID_EPOUT_ADDR, ((USBD_HID_HandleTypeDef *)pdev->pClassData)->Report_buf,HID_EPOUT_SIZE);

  return USBD_OK;
}

/**
  * @brief  USBD_HID_Init
  *         DeInitialize the HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx)
{
  /* Close HID EPs */
  USBD_LL_CloseEP(pdev, HID_EPIN_ADDR);
  pdev->ep_in[HID_EPIN_ADDR & 0xFU].is_used = 0U;

  /* FRee allocated memory */
  if(pdev->pClassData != NULL)
  {
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return USBD_OK;
}

/**
  * @brief  USBD_HID_Setup
  *         Handle the HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef*) pdev->pClassData;
  uint16_t len = 0U;
  uint8_t *pbuf = NULL;
  uint16_t status_info = 0U;
  USBD_StatusTypeDef ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {
    case HID_REQ_SET_PROTOCOL:
      hhid->Protocol = (uint8_t)(req->wValue);
      break;

    case HID_REQ_GET_PROTOCOL:
      USBD_CtlSendData (pdev, (uint8_t *)(void *)&hhid->Protocol, 1U);
      break;

    case HID_REQ_SET_IDLE:
      hhid->IdleState = (uint8_t)(req->wValue >> 8);
      break;

    case HID_REQ_GET_IDLE:
      USBD_CtlSendData (pdev, (uint8_t *)(void *)&hhid->IdleState, 1U);
      break;

    case HID_REQ_SET_REPORT:
      hhid->IsReportAvailable = 1U;
      USBD_CtlPrepareRx (pdev, hhid->Report_buf, req->wLength);
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_STATUS:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, (uint8_t *)(void *)&status_info, 2U);
      }
      else
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_DESCRIPTOR:
      if(req->wValue >> 8 == HID_REPORT_DESC)
      {
        len = MIN(HID_MOUSE_REPORT_DESC_SIZE , req->wLength);
        pbuf = HID_MOUSE_ReportDesc;
      }
      else if(req->wValue >> 8 == HID_DESCRIPTOR_TYPE)
      {
        pbuf = USBD_HID_Desc;
        len = MIN(USB_HID_DESC_SIZ, req->wLength);
      }
      else
      {
        USBD_CtlError (pdev, req);
        ret = USBD_FAIL;
        break;
      }
      USBD_CtlSendData (pdev, pbuf, len);
      break;

    case USB_REQ_GET_INTERFACE :
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, (uint8_t *)(void *)&hhid->AltSetting, 1U);
      }
      else
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    case USB_REQ_SET_INTERFACE :
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        hhid->AltSetting = (uint8_t)(req->wValue);
      }
      else
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;

  default:
    USBD_CtlError (pdev, req);
    ret = USBD_FAIL;
    break;
  }

  return ret;
}

/**
  * @brief  USBD_HID_SendReport
  *         Send HID Report
  * @param  pdev: device instance
  * @param  buff: pointer to report
  * @retval status
  */
uint8_t USBD_HID_SendReport     (USBD_HandleTypeDef  *pdev,
                                 uint8_t *report,
                                 uint16_t len)
{
  USBD_HID_HandleTypeDef     *hhid = (USBD_HID_HandleTypeDef*)pdev->pClassData;

  if (pdev->dev_state == USBD_STATE_CONFIGURED )
  {
    if(hhid->state == HID_IDLE)
    {
      hhid->state = HID_BUSY;
      USBD_LL_Transmit (pdev,
                        HID_EPIN_ADDR,
                        report,
                        len);
      return USBD_OK;
    }
    else{
    	return USBD_BUSY;
    }
  }else{
	  return USBD_FAIL;
  }
}

/**
  * @brief  USBD_HID_GetPollingInterval
  *         return polling interval from endpoint descriptor
  * @param  pdev: device instance
  * @retval polling interval
  */
uint32_t USBD_HID_GetPollingInterval (USBD_HandleTypeDef *pdev)
{
  uint32_t polling_interval = 0U;

  /* HIGH-speed endpoints */
  if(pdev->dev_speed == USBD_SPEED_HIGH)
  {
   /* Sets the data transfer polling interval for high speed transfers.
    Values between 1..16 are allowed. Values correspond to interval
    of 2 ^ (bInterval-1). This option (8 ms, corresponds to HID_HS_BINTERVAL */
    polling_interval = (((1U <<(HID_HS_BINTERVAL - 1U))) / 8U);
  }
  else   /* LOW and FULL-speed endpoints */
  {
    /* Sets the data transfer polling interval for low and full
    speed transfers */
    polling_interval =  HID_FS_BINTERVAL;
  }

  return ((uint32_t)(polling_interval));
}

/**
  * @brief  USBD_HID_GetCfgFSDesc
  *         return FS configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_HID_GetFSCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_HID_CfgFSDesc);
  return USBD_HID_CfgFSDesc;
}

/**
  * @brief  USBD_HID_GetCfgHSDesc
  *         return HS configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_HID_GetHSCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_HID_CfgHSDesc);
  return USBD_HID_CfgHSDesc;
}

/**
  * @brief  USBD_HID_GetOtherSpeedCfgDesc
  *         return other speed configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_HID_GetOtherSpeedCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_HID_OtherSpeedCfgDesc);
  return USBD_HID_OtherSpeedCfgDesc;
}

/**
  * @brief  USBD_HID_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */

/*
bool USB_HID_Ready_To_TX_Next_Report(USBD_HandleTypeDef *pdev){
	USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef*)pdev->pClassData;
	if ((pdev->dev_state == USBD_STATE_CONFIGURED ) && (hhid->state == HID_IDLE)){
		return true;
	}else{
		return false;
	}
}

uint8_t Assuming_Endpoint_Ready_Send_HID_Report(USBD_HandleTypeDef *pdev, char* report, uint16_t len){
	USBD_LL_Transmit (pdev,
			HID_EPIN_ADDR,
			(uint8_t*) report,
			len);
}*/


uint8_t USB_HID_Send_Next_Report(USBD_HandleTypeDef *pdev){
	USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef*)pdev->pClassData;
	if ((pdev->dev_state == USBD_STATE_CONFIGURED ) && (hhid->state == HID_IDLE)){
		auto [report, len] = hid_report_buf.transfer_out_next_report();
		if (len > 0){
			hhid->state = HID_BUSY;
			USBD_LL_Transmit (pdev,
					HID_EPIN_ADDR,
					(uint8_t*) report,
					len);
		}
		return USBD_OK;
	}else{
		if (pdev->dev_state != USBD_STATE_CONFIGURED ){
			return USBD_FAIL;
		}
		if (hhid->state != HID_IDLE){
			return USBD_BUSY;
		}
	}
}


static uint8_t  USBD_HID_DataIn (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{
  /* Ensure that the FIFO is empty before a new transfer, this condition could
  be caused by  a new transfer before the end of the previous transfer */
  //printf("USBD_HID_DataIn \n");
  ((USBD_HID_HandleTypeDef *)pdev->pClassData)->state = HID_IDLE;
  hid_report_buf.last_send_complete();
  USB_HID_Send_Next_Report(pdev);
}

/**
  * @brief  USBD_CUSTOM_HID_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_HID_DataOut (USBD_HandleTypeDef *pdev,
                                          uint8_t epnum)
{

  USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef*)pdev->pClassData;

  USBD_LL_PrepareReceive(pdev, HID_EPOUT_ADDR , hhid->Report_buf, HID_EPOUT_SIZE);

  return USBD_OK;
}

static uint8_t  USBD_HID_EP0_TxSent(USBD_HandleTypeDef *pdev)
{
	printf("USBD_HID_EP0_TxSent \n");
	return USBD_OK;
}

/**
  * @brief  USBD_CUSTOM_HID_EP0_RxReady
  *         Handles control request data.
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t USBD_HID_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
	USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef*)pdev->pClassData;

  if (hhid->IsReportAvailable == 1U)
  {
	  if(usbd_hid_callback_reg_state){
		  (*Handle_HID_RX_Func)((const uint8_t*) &hhid->Report_buf[1], 62);
	  }

	  //HandleHIDOutputMsg
	  //((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->OutEvent(hhid->Report_buf[0],
	  //                                                         hhid->Report_buf[1]);

    hhid->IsReportAvailable = 0U;
  }

  return USBD_OK;
}



/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
static uint8_t  *USBD_HID_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = sizeof (USBD_HID_DeviceQualifierDesc);
  return USBD_HID_DeviceQualifierDesc;
}

/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
