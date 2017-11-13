#include "main.h"
#include "usb.h"
#include "init.h"
#include "peripheral.h"
#include "HIDClassCommon.h"

/// GLOBAL VARS ////////////////////////////////////////////

volatile uint8_t g_systimer_flag;
t_HID_Dev HID_Dev;

/// CALLBACKS //////////////////////////////////////////////

/*
 USB_EP0_RxReady_callback ������������ ��� ��������� ������,
 �������� EP0 � �������� Data stage. � ������ �������,
 ���������� ��������� �����, ���������������� �������� 
 ����������� ������ ���������� (STD keyboard).
 BYTE 0 - Report ID (������ ���������)
 BYTE 1 - LED DATA
*/
void USB_EP0_RxReady_callback(uint8_t *p_data)
{
	if (p_data[0] == STD_KEYBOARD_REPORT_ID) { // �������� ReportID
		if (p_data[1] & HID_KEYBOARD_LED_NUMLOCK) // ���� ��� Num Lock ���������
															Led_setmode(LED_0, LED_ON); // �������� ��������� LED_0
		else											Led_setmode(LED_0, LED_OFF); // ����� �����
		if (p_data[1] & HID_KEYBOARD_LED_CAPSLOCK) // ���� ��� Caps Lock ���������
															Led_setmode(LED_1, LED_ON); // �������� ��������� LED_1
		else											Led_setmode(LED_1, LED_OFF); // ����� �����
		if (p_data[1] & HID_KEYBOARD_LED_SCROLLLOCK) // ���� ��� Scroll lock ���������
															Led_setmode(LED_2, LED_ON); // �������� ��������� LED_2
		else											Led_setmode(LED_2, LED_OFF); // ����� �����
	}
}

/*
 ���������� KEY_changed ������������ ��� ����, �����
 ���������� ������������ �������, � ��������� ���� ���������
 ��� ����������, ������� ����������� ������������ �������.
 � ������ ������� ��� ������� ����������� ������ ���������� 
 (STD keyboard)
*/
void KEY_changed(uint8_t key_num)
{
	switch(key_num) {
		case BTN_0: // used in STD keyboard
		case BTN_1: // used in STD keyboard
		case BTN_2: // used in STD keyboard
		case BTN_3: // used in STD keyboard
		case BTN_4: // used in STD keyboard
		case BTN_5: // used in STD keyboard
		case BTN_6: // used in STD keyboard
		case BTN_7: // used in STD keyboard
			HID_Dev.STD_KB_Report_changed_flag = 1; // ��������� ������ ������ ���������� ����������
			break;
			//HID_Dev.EXT1_KB_Report_changed_flag = 1; // ��������� ������ ������ ���������� ����������
			//break;
			//HID_Dev.EXT2_KB_Report_changed_flag = 1; // ��������� ������ ������� ���������� ����������
			//break;
		default: // ��� ����� ������, ������� �� ��������� � �����������
			break;
	}
}

/*
	EVENT_KEY_pressed - ������� "������ ������". key_num - ����� ������� �������
	EVENT_KEY_released - ������� "������ ��������". key_num - ����� ���������� �������
*/
void EVENT_KEY_pressed(uint8_t key_num) { KEY_changed(key_num); }
void EVENT_KEY_released(uint8_t key_num) { KEY_changed(key_num); }

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
 ����� ���� ������, ���������� ������ ����-���� ������ (�� �������)
 ���� ������.
 USB ������ �� ����������� ��������� �� ����� �������������,
 � ����� ����� ����� ����������� ����-���� (16 ��� ������ 8).
 ��. ���� USB_KeyboardReport_EXT1_Data_t � USB_KeyboardReport_EXT2_Data_t.
 ��� ����� ��������� ���������� ��. �������� 
 http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 ����-���� ��� ��.
 � ������ ������� 4 ������� �������� �������������� (WIN, CTRL, SHIFT, ALT),
 � 4-� �������� ��������� ����-���� ������ "1", "2", "3" � "4".
 ��� ��������� ���������� �������, �����:
 1) � ���������� KEY_changed �������������� ����
 2) � �-��� BTN_USB_send_Loop, � ��������� ����� ������ ����������,
 ��������� ���������� ����-���� ��� ������ �������, � ������������ � ��������.
 3) ���������, ����� ������ ���������� �� ������������ ��� �������.
*/
void BTN_USB_send_Loop(void)
{
	uint8_t KeyCodesMAX;
	uint8_t key_mask = Buttons_get_mask(); // ��������� ������� ����� ������� ������
	uint8_t UsedKeyCodes = 0; // ������� �������������� ����-�����
	
	// STD Keyboard (Usage Page 0x7)
	if (HID_Dev.STD_KB_Report_changed_flag) { // ���� ��������� ������ ������ ���������� ���������� - ��������� � ���������� USB �����
		KeyCodesMAX = 5; // � ������ ���������� ������������ ����� ���� ������ �� 5 ������ (��� ����� �������������)
		HID_Dev.STD_KB_Report.Modifier = 0; // ���������� ���� �������������
		if (key_mask & (1 << BTN_0)) // ���� ������ ������� BTN_0
			HID_Dev.STD_KB_Report.Modifier |= HID_KEYBOARD_MODIFIER_LEFTGUI; // ���������� ��� "LEFTGUI" (WIN) � ����� �������������
		if (key_mask & (1 << BTN_1)) // ���������� ��� ������ ������
			HID_Dev.STD_KB_Report.Modifier |= HID_KEYBOARD_MODIFIER_LEFTCTRL;
		if (key_mask & (1 << BTN_6))
			HID_Dev.STD_KB_Report.Modifier |= HID_KEYBOARD_MODIFIER_LEFTSHIFT;
		if (key_mask & (1 << BTN_7))
			HID_Dev.STD_KB_Report.Modifier |= HID_KEYBOARD_MODIFIER_LEFTALT;
			
		if (key_mask & (1 << BTN_2)) { // ���� ������ ������� BTN_2
			HID_Dev.STD_KB_Report.KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_1_AND_EXCLAMATION; // ���������� � ����� ����-��� ������ "1" (Usage Page 0x7)
			if (UsedKeyCodes >= KeyCodesMAX) goto STD_Send; // ���� ��� 5 ���� ��� ����-���� ��� ������������ - 
																											// ���������� ��������� ������� � ��������� � �������� ������
		}
		if (key_mask & (1 << BTN_3)) { // ���������� ��� ������ ������
			HID_Dev.STD_KB_Report.KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_2_AND_AT;
			if (UsedKeyCodes >= KeyCodesMAX) goto STD_Send;
		}
		if (key_mask & (1 << BTN_4)) {
			HID_Dev.STD_KB_Report.KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_3_AND_HASHMARK;
			if (UsedKeyCodes >= KeyCodesMAX) goto STD_Send;
		}
		if (key_mask & (1 << BTN_5)) {
			HID_Dev.STD_KB_Report.KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_4_AND_DOLLAR;
			if (UsedKeyCodes >= KeyCodesMAX) goto STD_Send;
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
		/*if (key_mask & (1 << BTN_3)) { // ������ ��� BTN_3
			HID_Dev.EXT1_KB_Report.KeyCode_LO = LOBYTE(0x????); // ������� ���� ����-����, �� Usage Page 0xC
			HID_Dev.EXT1_KB_Report.KeyCode_HI = HIBYTE(0x????); // ������� ���� ����-����, �� Usage Page 0xC
			goto EXT1_Send; // ����� ��������� � ��������
		}*/
		/*if (key_mask & (1 << BTN_4)) { // ������ ��� BTN_4
			HID_Dev.EXT1_KB_Report.KeyCode_LO = LOBYTE(0x????); // ������� ���� ����-����, �� Usage Page 0xC
			HID_Dev.EXT1_KB_Report.KeyCode_HI = HIBYTE(0x????); // ������� ���� ����-����, �� Usage Page 0xC
			goto EXT1_Send; // ����� ��������� � ��������
		}*/
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
		/*if (key_mask & (1 << BTN_1)) { // ������ ��� BTN_1
			HID_Dev.EXT2_KB_Report.KeyCode[UsedKeyCodes++] = 0x??; // ����-���, �� Usage Page 0x1
			if (UsedKeyCodes >= KeyCodesMAX) goto EXT2_Send; // ���� ��� 7 ���� ��� ����-���� ��� ������������ - 
																											// ���������� ��������� ������� � ��������� � �������� ������
		}*/
		/*if (key_mask & (1 << BTN_2)) { // ������ ��� BTN_2
			HID_Dev.EXT2_KB_Report.KeyCode[UsedKeyCodes++] = 0x??; // ����-���, �� Usage Page 0x1
			if (UsedKeyCodes >= KeyCodesMAX) goto EXT2_Send; // ���� ��� 7 ���� ��� ����-���� ��� ������������ - 
																											// ���������� ��������� ������� � ��������� � �������� ������
		}*/
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
						if (p_req->wValue_HI == HID_REPORT_FEATURE)
						{
							// ��� �������������� ������ USB ������ � ���������� ������.
							switch(p_req->wValue_LO) // ReportID
							{
								case STD_KEYBOARD_REPORT_ID: // STD Keyboard
									return USB_Send_Data((uint8_t*)&HID_Dev.STD_KB_Report, sizeof(HID_Dev.STD_KB_Report), 0);
								case EXT1_KEYBOARD_REPORT_ID: // EXT1 Keyboard
									return USB_Send_Data((uint8_t*)&HID_Dev.EXT1_KB_Report, sizeof(HID_Dev.EXT1_KB_Report), 0);
								case EXT2_KEYBOARD_REPORT_ID: // EXT2 Keyboard
									return USB_Send_Data((uint8_t*)&HID_Dev.EXT2_KB_Report, sizeof(HID_Dev.EXT2_KB_Report), 0);
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
							if (p_req->wValue_LO == STD_KEYBOARD_REPORT_ID) // ReportID
							{
								// ��� SETUP STAGE ����� ��������� (������) ������ � ����������� 
								// ����������� ������ ����������, ��. USB_EP0_RxReady_callback
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
					if (p_req->wValue_HI == TYPE_REPORT_DESCRIPTOR) // 0x22
					{
						if (p_req->wIndex_LO == INTERFACE_ID_HIDDev) {
							// ��� ������ Report ���������� HID ����������
							return USB_Send_Data(usb_report_descriptor, (uint8_t)MIN(p_req->wLength_LO, SIZE_REPORT_DESCRIPTOR), 0);
						}
					} else 
					if (p_req->wValue_HI == TYPE_HID_DESCRIPTOR) // 0x21
					{
							// ��� ������ HID ���������� USB ����������
						return USB_Send_Data(USB_HID_descriptor, (uint8_t)MIN(p_req->wLength_LO, SIZE_HID_DESCRIPTOR), 0);
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
void USB_EP1_RxReady_callback(uint8_t *p_data)
{
	// Nothing. Really.
}

/// MAIN ///////////////////////////////////////////////////

void main(void)
{
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
		if (g_systimer_flag) { // 100 Hz call
			g_systimer_flag = 0;
			Buttons_loop(); // ����� ������
			BTN_USB_send_Loop(); // �������� USB ������� � ���������� ������
			USB_slow_loop(); // "���������" ���� USB ��� ��������� HSI ����������
		}
	}
}





