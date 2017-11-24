#include "main.h"
#include "usb.h"
#include "init.h"
#include "peripheral.h"
#include "HIDClassCommon.h"

/// GLOBAL VARS ////////////////////////////////////////////

volatile uint8_t g_systimer_400Hz_flag;
t_HID_Dev HID_Dev;
t_KeyCode* g_KeyCode = (t_KeyCode*)(KEY_SETTINGS_START_ADDR); // ��������� ������ � EEPROM

/// CALLBACKS //////////////////////////////////////////////

/*
 Write_key_settings - ���������� ������ �������� �������.
*/
void Write_key_settings(void)
{
	if (HID_Dev.Key_settings_to_write.write_flag) { // ���� ���� ���������
		if (HID_Dev.Key_settings_to_write.key_num < BTN_CNT) { // � ����� ����������
			FLASH_Unlock(FLASH_MEMTYPE_DATA); // ����� � EEPROM
			g_KeyCode[HID_Dev.Key_settings_to_write.key_num].UsagePage = HID_Dev.Key_settings_to_write.KeyCode.UsagePage;
			FLASH_WaitForLastOperation(FLASH_MEMTYPE_DATA);
			g_KeyCode[HID_Dev.Key_settings_to_write.key_num].Modifiers = HID_Dev.Key_settings_to_write.KeyCode.Modifiers;
			FLASH_WaitForLastOperation(FLASH_MEMTYPE_DATA);
			g_KeyCode[HID_Dev.Key_settings_to_write.key_num].KeyCode_LO = HID_Dev.Key_settings_to_write.KeyCode.KeyCode_LO;
			FLASH_WaitForLastOperation(FLASH_MEMTYPE_DATA);
			g_KeyCode[HID_Dev.Key_settings_to_write.key_num].KeyCode_HI = HID_Dev.Key_settings_to_write.KeyCode.KeyCode_HI;
			FLASH_WaitForLastOperation(FLASH_MEMTYPE_DATA);
			FLASH_Lock(FLASH_MEMTYPE_DATA);
		}
		HID_Dev.Key_settings_to_write.write_flag = 0; // ����� �����
	}
}

/*
 USB_EP0_RxReady_callback ������������ ��� ��������� ������,
 �������� EP0 � �������� Data stage. � ������ �������,
 ���������� ��������� �����, ���������������� �������� 
 ����������� ������ ���������� (STD keyboard), � ����� 
 ��������� ������� ������ �������� ������� � ��������� 
 ������ �������� ������� ��� ������ ��������.
 ��� ���� �������:
 BYTE 0 - Report ID (���������� �������)
 BYTE 1..7 - ������
*/
void USB_EP0_RxReady_callback(uint8_t *p_data, uint8_t length)
{
	if (length == 0) return;
	switch(p_data[0]) // ReportID
	{
		case STD_KEYBOARD_REPORT_ID: // ��������� ��������� �����������
			if (length < 2) break;
			if (p_data[1] & HID_KEYBOARD_LED_NUMLOCK) // ���� ��� Num Lock ���������
																Led_setmode(LED_0, LED_ON); // �������� ��������� LED_0
			else											Led_setmode(LED_0, LED_OFF); // ����� �����
			if (p_data[1] & HID_KEYBOARD_LED_CAPSLOCK) // ���� ��� Caps Lock ���������
																Led_setmode(LED_1, LED_ON); // �������� ��������� LED_1
			else											Led_setmode(LED_1, LED_OFF); // ����� �����
			if (p_data[1] & HID_KEYBOARD_LED_SCROLLLOCK) // ���� ��� Scroll lock ���������
																Led_setmode(LED_2, LED_ON); // �������� ��������� LED_2
			else											Led_setmode(LED_2, LED_OFF); // ����� �����
			break;
		case CONFIGURE_KEYS_REPORT_ID: // ������ �������� ������� � EEPROM
			if (length < 6) break;
			if (p_data[1] < BTN_CNT) {
				if (HID_Dev.Key_settings_to_write.write_flag == 0) {
					HID_Dev.Key_settings_to_write.key_num = p_data[1];
					HID_Dev.Key_settings_to_write.KeyCode.UsagePage = p_data[2];
					HID_Dev.Key_settings_to_write.KeyCode.Modifiers = p_data[3];
					HID_Dev.Key_settings_to_write.KeyCode.KeyCode_LO = p_data[4];
					HID_Dev.Key_settings_to_write.KeyCode.KeyCode_HI = p_data[5];
					HID_Dev.Key_settings_to_write.write_flag = 6;
				}
			}
			break;
		case READ_KEYS_REPORT_ID: // ��������� ������ �������� ������� ��� ������ ��������
			if (length < 2) break;
			if (p_data[1] < BTN_CNT) {
				HID_Dev.Key_settings.key_num = p_data[1];
				HID_Dev.Key_settings.KeyCode = g_KeyCode[HID_Dev.Key_settings.key_num];
			}
			break;
	}
}

/*
	EVENT_KEY_pressed - ������� "������ ������". key_num - ����� ������� ������
	EVENT_KEY_released - ������� "������ ��������". key_num - ����� ���������� ������
*/
void EVENT_KEY_pressed(uint8_t key_num) { }
void EVENT_KEY_released(uint8_t key_num) { }

/*
 BTN_USB_send_Loop - ������� �������� USB ������� �� 
 ����-������ ������� ������. ��� ����������� ����������,
 ����� ����-�����, ����� ������������ ���� "�������������" -
 CTRL, SHIFT, ALT, WIN. ���� ������� ������ - � ����� 
 ������������� ������������ ������ ���.
 USB ����� �� ����������� ����������, ������������ �� ���� 
 ReportID, 1 ���� ��� �������������, � 5 ���� ��� ����-�����
 ������� ������. ��. ��� USB_KeyboardReport_STD_Data_t. ����
 ������������ ������ ����� ���� ������, � ��������� ������
 ��� ����-����� ������ ���� ����. ���� ������������ ������ 
 ����� ���� ������, ���������� ������ ����-���� ������
 (�� �������) ���� ������.
 USB ������ �� ����������� ��������� �� ����� �������������,
 � ����� ����� ����� ����������� ����-���� (16 ��� ������ 8).
 ��. ���� USB_KeyboardReport_EXT1_Data_t � USB_KeyboardReport_EXT2_Data_t.
 ��� ����� ��������� ���������� ��. �������� 
 http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 ����-���� ��� ��.
 ***********************************************************
 ��� ��������� �������, �����:
 1) ��������� Report (HID SET_REPORT) ������� ���������� (USAGE_PAGE 0x1, USAGE 0x80):
	Byte0 = CONFIGURE_KEYS_REPORT_ID
	Byte1 = ����� �������
	Byte2 = UsagePage
	  = STD_KEYBOARD_USAGE_PAGE - ������ ����������
		= EXT1_KEYBOARD_USAGE_PAGE - ������ ����������
		= EXT2_KEYBOARD_USAGE_PAGE - ������ ����������
		������ �������� �������� ���������� �������������� � HID ������������.
	Byte3 = ���� �������������
	Byte4 = KeyCode_LO, ������� ���� ����-����
	Byte5 = KeyCode_HI, ������� ���� ����-���� (������ ��� ������ ����������)
 2) ��������� ~100��, ���� ��������� �� ��������� � EEPROM, 
	����� ���� ��������� ���������� ���������.
	��� ������ �������� � EEPROM, USB ���������� ����� "����������",
	�.�. �� ����� ������ ���������� �������� � ���������. � ����
	������ ����� ��������� ���������� ����������� ���������� � �������
	� ����� ���������� ��������� ���������� ���������.
 ***********************************************************
 ��� ������ �������� �������, �����:
 1) ��������� Report (HID SET_REPORT) ������� ���������� (USAGE_PAGE 0x1, USAGE 0x80):
	Byte0 = READ_KEYS_REPORT_ID
	Byte1 = ����� �������
 2) ��������� ������ HID GET_REPORT � ReportID = READ_KEYS_REPORT_ID
 � ������� Report, � ������� ����� ��������� ��������� �������:
	Byte0 = READ_KEYS_REPORT_ID
	Byte1 = ����� �������
	Byte2 = UsagePage
	Byte3 = ���� �������������
	Byte4 = KeyCode_LO, ������� ���� ����-����
	Byte5 = KeyCode_HI, ������� ���� ����-����
*/
void BTN_USB_send_Loop(void)
{
	uint8_t i;
	uint8_t KeyCodesMAX;
	uint16_t key_mask = Buttons_get_mask(); // ��������� ������� ����� ������� ������
	uint8_t UsedKeyCodes = 0; // ������� �������������� ����-�����
	
	if (HID_Dev.prev_key_mask != key_mask) { // ���� ��������� ������ ����������
		for(i=0;i<BTN_CNT;i++) { // ���� ������������ ������� � ���������� ������ �����
			if ((HID_Dev.prev_key_mask ^ key_mask) & (1 << i)) {
				switch(g_KeyCode[i].UsagePage)
				{
					case STD_KEYBOARD_USAGE_PAGE: // UsagePage ������� ������������� ������ ����������
						HID_Dev.STD_KB_Report_changed_flag = 1; // ��������� ������ ������ ���������� ����������
						break;
					case EXT1_KEYBOARD_USAGE_PAGE: // UsagePage ������� ������������� ������ ����������
						HID_Dev.EXT1_KB_Report_changed_flag = 1; // ��������� ������ ������ ���������� ����������
						break;
					case EXT2_KEYBOARD_USAGE_PAGE: // UsagePage ������� ������������� ������� ����������
						HID_Dev.EXT2_KB_Report_changed_flag = 1; // ��������� ������ ������� ���������� ����������
						break;
				}
			}
		}
		HID_Dev.prev_key_mask = key_mask; // ��������� ��������� ������
	}
	
	// STD Keyboard (Usage Page 0x7)
	if (HID_Dev.STD_KB_Report_changed_flag) { // ���� ��������� ������ ������ ���������� ���������� - ��������� � ���������� USB �����
		KeyCodesMAX = 5; // � ������ ���������� ������������ ����� ���� ������ �� 5 ������ (��� ����� �������������)
		HID_Dev.STD_KB_Report.Modifier = 0; // ���������� ���� �������������
		for(i=0;i<BTN_CNT;i++) { // ��������� ���� �������������
			if (key_mask & (1 << i)) { // ���� ������ ������� i
				if (g_KeyCode[i].UsagePage == STD_KEYBOARD_USAGE_PAGE) { // � UsagePage ������� ������������� ������ ����������
					HID_Dev.STD_KB_Report.Modifier |= g_KeyCode[i].Modifiers; // ��������� ����� ������������� ������ �������
				}
			}
		}
		
		for(i=0;i<BTN_CNT;i++) { // ��� ���� ������
			if (key_mask & (1 << i)) { // ���� ������ ������� i
				if (g_KeyCode[i].UsagePage == STD_KEYBOARD_USAGE_PAGE) { // � UsagePage ������� ������������� ������ ����������
					HID_Dev.STD_KB_Report.KeyCode[UsedKeyCodes++] = g_KeyCode[i].KeyCode_LO; // ���������� � ����� ����-��� ������
					if (UsedKeyCodes >= KeyCodesMAX) // ���� ��� 5 ���� ��� ����-���� ��� ������������
						goto STD_Send;	// ���������� ��������� ������� � ��������� � �������� ������
				}
			}
		}
		// ��������� ���������������� ����-����� � 0
		while(UsedKeyCodes < KeyCodesMAX) HID_Dev.STD_KB_Report.KeyCode[UsedKeyCodes++] = 0;
	STD_Send:
		// �������� USB ������ � ����� ����� ��������� ���������� ��� �������� ��������
		if (USB_Send_Data((uint8_t*)&HID_Dev.STD_KB_Report, sizeof(HID_Dev.STD_KB_Report), 1) == 0)
			HID_Dev.STD_KB_Report_changed_flag = 0;
		return;
	}
	
	// EXT1 Keyboard (Usage Page 0xC)
	// � ������ ���������� ������������ ����� ���� ������ ������ ���� �������
	if (HID_Dev.EXT1_KB_Report_changed_flag) {
		for(i=0;i<BTN_CNT;i++) { // ��� ���� ������
			if (key_mask & (1 << i)) { // ���� ������ ������� i
				if (g_KeyCode[i].UsagePage == EXT1_KEYBOARD_USAGE_PAGE) { // UsagePage ������� ������������� ������ ����������
					HID_Dev.EXT1_KB_Report.KeyCode_LO = g_KeyCode[i].KeyCode_LO; // ���������� � ����� ������� ���� ����-����
					HID_Dev.EXT1_KB_Report.KeyCode_HI = g_KeyCode[i].KeyCode_HI; // ���������� � ����� ������� ���� ����-����
					goto EXT1_Send; // ����� ��������� � ��������
				}
			}
		}
		// ��������� ���������������� ����-����� � 0
		HID_Dev.EXT1_KB_Report.KeyCode_LO = 0;
		HID_Dev.EXT1_KB_Report.KeyCode_HI = 0;
	EXT1_Send:
		// �������� USB ������ � ����� ����� ��������� ���������� ��� �������� ��������
		if (USB_Send_Data((uint8_t*)&HID_Dev.EXT1_KB_Report, sizeof(HID_Dev.EXT1_KB_Report), 1) == 0)
			HID_Dev.EXT1_KB_Report_changed_flag = 0;
		return;
	}
	
	// EXT2 Keyboard (Usage Page 0x1)
	if (HID_Dev.EXT2_KB_Report_changed_flag) {
		KeyCodesMAX = 7; // � ������ ���������� ������������ ����� ���� ������ �� 7 ������
		for(i=0;i<BTN_CNT;i++) { // ��� ���� ������
			if (key_mask & (1 << i)) { // ���� ������ ������� i
				if (g_KeyCode[i].UsagePage == EXT2_KEYBOARD_USAGE_PAGE) { // UsagePage ������� ������������� ������� ����������
					HID_Dev.EXT2_KB_Report.KeyCode[UsedKeyCodes++] = g_KeyCode[i].KeyCode_LO; // ���������� � ����� ����-��� ������
					if (UsedKeyCodes >= KeyCodesMAX) // ���� ��� 7 ���� ��� ����-���� ��� ������������
						goto EXT2_Send;	// ���������� ��������� ������� � ��������� � �������� ������
				}
			}
		}
		// ��������� ���������������� ����-����� � 0
		while(UsedKeyCodes < KeyCodesMAX) HID_Dev.EXT2_KB_Report.KeyCode[UsedKeyCodes++] = 0;
	EXT2_Send:
		// �������� USB ������ � ����� ����� ��������� ���������� ��� �������� ��������
		if (USB_Send_Data((uint8_t*)&HID_Dev.EXT2_KB_Report, sizeof(HID_Dev.EXT2_KB_Report), 1) == 0)
			HID_Dev.EXT2_KB_Report_changed_flag = 0;
		return;
	}
}

/*
	USB_Class_Init_callback ���������� ��� �������������
	USB ���������� ���������.
*/
int8_t USB_Class_Init_callback(uint8_t dev_config)
{
	uint8_t i;
	for(i=0;i<sizeof(HID_Dev);i++) ((uint8_t*)&HID_Dev)[i] = 0; // ���� memset
	// ������ ������ ReportID � ������������ ������
	HID_Dev.STD_KB_Report.ReportID = STD_KEYBOARD_REPORT_ID;
	HID_Dev.EXT1_KB_Report.ReportID = EXT1_KEYBOARD_REPORT_ID;
	HID_Dev.EXT2_KB_Report.ReportID = EXT2_KEYBOARD_REPORT_ID;
	HID_Dev.Key_settings.ReportID = READ_KEYS_REPORT_ID;
	// ������������� ������ � ����������� �������
	//HID_Dev.Key_settings.key_num = 0; // �������� ������� 0
	HID_Dev.Key_settings.KeyCode = g_KeyCode[HID_Dev.Key_settings.key_num];
	HID_Dev.Key_settings_to_write.write_flag = 0; // ���� ���������� ������ ��������
	// ��������� ���� ������ ��������� � "1" ��� �������� ������� � ��������� ���������� ������
	HID_Dev.STD_KB_Report_changed_flag = 1; 
	HID_Dev.EXT1_KB_Report_changed_flag = 1;
	HID_Dev.EXT2_KB_Report_changed_flag = 1;
	return 0;
}

/*
	USB_Class_DeInit_callback ���������� ��� ������
	USB ���������� ���������.
*/
int8_t USB_Class_DeInit_callback(void)
{
	return 0;
}

/*
	USB_Setup_Request_callback ���������� ��� ��������� USB ��������,
	�� ���������� � ����������� USB ����.
*/
int8_t USB_Setup_Request_callback(t_USB_SetupReq *p_req)
{
	switch (p_req->bmRequest & USB_REQ_TYPE_MASK)
  {
		case USB_REQ_TYPE_CLASS:
		{
			switch (p_req->bRequest)
			{
				case HID_REQ_GET_REPORT: // 0x01
				{
					if (p_req->wIndex_LO == INTERFACE_ID_HIDDev) // HID Device
					{
						//if (p_req->wValue_HI == HID_REPORT_FEATURE)
						if ((p_req->wValue_HI == HID_REPORT_FEATURE)||(p_req->wValue_HI == HID_REPORT_INPUT))
						{
							// ��� �������������� ������ USB ������ � ���������� ������, ���� ������ �������� �������� �������
							switch(p_req->wValue_LO) // ReportID
							{
								case STD_KEYBOARD_REPORT_ID: // STD Keyboard
									return USB_Send_Data((uint8_t*)&HID_Dev.STD_KB_Report, sizeof(HID_Dev.STD_KB_Report), 0);
								case EXT1_KEYBOARD_REPORT_ID: // EXT1 Keyboard
									return USB_Send_Data((uint8_t*)&HID_Dev.EXT1_KB_Report, sizeof(HID_Dev.EXT1_KB_Report), 0);
								case EXT2_KEYBOARD_REPORT_ID: // EXT2 Keyboard
									return USB_Send_Data((uint8_t*)&HID_Dev.EXT2_KB_Report, sizeof(HID_Dev.EXT2_KB_Report), 0);
								case READ_KEYS_REPORT_ID: // Read key settings
									return USB_Send_Data((uint8_t*)&HID_Dev.Key_settings, sizeof(HID_Dev.Key_settings), 0);
							}
						}
					}
					break;
				}
				
				case HID_REQ_SET_REPORT: // 0x09
				{
					if (p_req->wIndex_LO == INTERFACE_ID_HIDDev) // HID Device
					{
						if (p_req->wValue_HI == HID_REPORT_OUTPUT)
						{
							switch(p_req->wValue_LO) // ReportID
							{
								case CONFIGURE_KEYS_REPORT_ID: // ������ �������� �������
									// ��� SETUP STAGE ����� ��������� (������) ������ � 
									// ����������� �������, ��. USB_EP0_RxReady_callback
									if (HID_Dev.Key_settings_to_write.write_flag) break; // ���� ���������� ������ ��� ��������� - �������� �������
								case STD_KEYBOARD_REPORT_ID: // ��������� ��������� �����������
									// ��� SETUP STAGE ����� ��������� (������) ������ � ����������� 
									// ����������� ������ ����������, ��. USB_EP0_RxReady_callback
								case READ_KEYS_REPORT_ID: // ������ �������� �������
									// ��� SETUP STAGE ����� ��������� (������) ������ � 
									// ������� �������� �������, ��. USB_EP0_RxReady_callback
									return USB_Send_Data(NULL, 0, 0); // ���� ACK
							}
						}
					}
					break;
				}
				
				case HID_REQ_SET_PROTOCOL: // 0x0B
					HID_Dev.Protocol = p_req->wValue_LO;
					return USB_Send_Data(NULL, 0, 0);
					
				case HID_REQ_GET_PROTOCOL: // 0x03
					return USB_Send_Data(&HID_Dev.Protocol, 1, 0);
					
				case HID_REQ_SET_IDLE: // 0x0A
					HID_Dev.IdleState = p_req->wValue_HI;
					return USB_Send_Data(NULL, 0, 0);
					
				case HID_REQ_GET_IDLE: // 0x02
					return USB_Send_Data(&HID_Dev.IdleState, 1, 0);
			}
			break;
		}
    
		case USB_REQ_TYPE_STANDARD:
		{
			switch (p_req->bRequest)
			{
				case USB_REQ_GET_DESCRIPTOR:
				{
					uint16_t wLength = (uint16_t)p_req->wLength_LO | (uint16_t)((uint16_t)p_req->wLength_HI << 8);
					if (p_req->wValue_HI == TYPE_REPORT_DESCRIPTOR) // 0x22
					{
						if (p_req->wIndex_LO == INTERFACE_ID_HIDDev) {
							// ��� ������ Report ���������� HID ����������
							return USB_Send_Data(usb_report_descriptor, (uint8_t)MIN(wLength, SIZE_REPORT_DESCRIPTOR), 0);
						}
					} else 
					if (p_req->wValue_HI == TYPE_HID_DESCRIPTOR) // 0x21
					{
							// ��� ������ HID ���������� USB ����������
						return USB_Send_Data(USB_HID_descriptor, (uint8_t)MIN(wLength, SIZE_HID_DESCRIPTOR), 0);
					}
					break;
				}
					
				case USB_REQ_GET_INTERFACE:
				{
					if (p_req->wIndex_LO == INTERFACE_ID_HIDDev) {
						return USB_Send_Data(&HID_Dev.AltSetting, 1, 0);
					}
					break;
				}
					
				case USB_REQ_SET_INTERFACE:
				{
					if (p_req->wIndex_LO == INTERFACE_ID_HIDDev) {
						HID_Dev.AltSetting = p_req->wValue_LO;
						return USB_Send_Data(NULL, 0, 0);
					}
					break;
				}
			}
		}
  }
	return -1;
}

/*
 USB_EP1_RxReady_callback ������������ ��� ��������� ������,
 �������� EP1. � ������ ������� �� ������������.
*/
void USB_EP1_RxReady_callback(uint8_t *p_data, uint8_t length)
{
	// Nothing. Really.
}

/// MAIN ///////////////////////////////////////////////////

void main(void)
{
	uint8_t counter = 0;	
	Check_OPTION_BYTE();
	disableInterrupts();
	Init_Clock();
	Init_GPIO();
	Leds_init();
	Buttons_init();
	SYSTimer_Init();
	USB_Init();
	enableInterrupts();
	USB_connect(); // ����������� �������� 1K5

	while(1)
	{
		USB_loop(); // "�������" ���� USB, ��� ��������� ��������
		if (g_systimer_400Hz_flag) { // 400 Hz call
			g_systimer_400Hz_flag = 0;
			Buttons_loop(); // ����� ������
			counter++;
			if (counter >= 4) { // 100 Hz call
				counter = 0;
				BTN_USB_send_Loop(); // �������� USB ������� � ���������� ������
				USB_slow_loop(); // "���������" ���� USB ��� ��������� HSI ���������� � EP1 watchdog
				Write_key_settings(); // ���������� ������ �������� ������ � EEPROM
			}
		}
	}
}





