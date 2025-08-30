#include "git.h"

Data_TypeDef Data_init;						  // �豸���ݽṹ��
Threshold_Value_TypeDef threshold_value_init; // �豸��ֵ���ýṹ��
Device_Satte_Typedef device_state_init;		  // �豸״̬
DHT11_Data_TypeDef DHT11_Data;
// ��ȡ���ݲ���
mySta Read_Data(Data_TypeDef *Device_Data)
{
	Read_DHT11(&DHT11_Data); // ��ȡ��ʪ������
	Device_Data->temperatuer = DHT11_Data.temp_int + DHT11_Data.temp_deci * 0.01;
	Device_Data->humiditr = DHT11_Data.humi_int + DHT11_Data.humi_deci * 0.01;

	return MY_SUCCESSFUL;
}
// ��ʼ��
mySta Reset_Threshole_Value(Threshold_Value_TypeDef *Value, Device_Satte_Typedef *device_state)
{

  //д
	//W_Test();
	// ��
  R_Test();
	device_state_init.turn = 15;
	threshold_value_init.temp_value = 25;
	
	return MY_SUCCESSFUL;
}
// ����OLED��ʾ��������
mySta Update_oled_massage()
{
#if OLED // �Ƿ��
	char str[50];
	// �Զ�ģʽ
	if(Data_init.Flage == 1){
		sprintf(str, "ҡͷģʽ   ");
	}else
		sprintf(str, "�̶�ģʽ   ");
	OLED_ShowCH(0, 0, (unsigned char *)str);
	if(device_state_init.time_state){
		sprintf(str, "Time: %d min �� ",device_state_init.tiem);
	}else 
		sprintf(str, "Time: %d min �� ",device_state_init.tiem);
	OLED_ShowCH(0, 2, (unsigned char *)str);
	sprintf(str, "����ת�� : %d ", turn_init.Turn_Num);
	OLED_ShowCH(0, 4, (unsigned char *)str);
	if(device_state_init.fan_state){
		sprintf(str, "����״̬ : �� ");
	}else 
		sprintf(str, "����״̬ : �� ");
	OLED_ShowCH(0, 6, (unsigned char *)str);
#endif

	return MY_SUCCESSFUL;
}

// �����豸״̬
mySta Update_device_massage()
{
	// ��ת 
	if(Data_init.Flage == 1){
		if(device_state_init.turn_state == 0 && device_state_init.turn > 5 ){
			device_state_init.turn --;
			if(device_state_init.turn == 5){
				// �л�
				device_state_init.turn_state = 1;
			}
		}else if(device_state_init.turn_state  && device_state_init.turn < 24 )
		{
			device_state_init.turn++;
			if(device_state_init.turn == 24){
				// �л�
				device_state_init.turn_state = 0;
			}
		}
	}
	TIM_SetCompare2(TIM3,device_state_init.turn);
	//��ֹ����
	delay_ms(10);
	
	// PWM ����
	if(turn_init.Turn_Num > 0 && device_state_init.fan_state == 1){
		TIM_SetCompare1(TIM3,turn_init.Turn_Num * 2);
	}else{
		TIM_SetCompare1(TIM3,0);
	}

	// ִ������ģ�鷢����Ϣ
	if (Data_init.cmd)
	{
		switch (Data_init.cmd)
		{
		case 1:
				// ������
				device_state_init.fan_state = 1;
			break;
		case 2:
				// �ط���
				device_state_init.fan_state = 0;
				Data_init.Flage =0;
			break;
		case 3:
				// ����
				turn_init.Turn_Num = turn_init.Turn_Num + 20;
				if (turn_init.Turn_Num > 100)
				{
					turn_init.Turn_Num = 100;
				}
			break;
		case 4:
				// ����
				turn_init.Turn_Num = turn_init.Turn_Num - 20;
				if (turn_init.Turn_Num < 0)
				{
					turn_init.Turn_Num = 0;
				}
			break;
		 case 5:
				// ҡͷģʽ
				Data_init.Flage = 1;
		 	break;
		 case 6:
				// �̶�ģʽ
				Data_init.Flage = 0;
			break; 
		}
		Data_init.cmd = 0;
	}	
	return MY_SUCCESSFUL;
}

// ��ʱ��
void Automation_Close(void)
{
	if(Data_init.temperatuer > threshold_value_init.temp_value ){
		if(device_state_init.state == 0){
			device_state_init.state =1;
			u2_printf("cmd:1");
		}
	}else{
		device_state_init.state =0;
	}
	
	// ʵ��1s��ʱ
	if(turn_init.Turn_Num_copy != turn_init.Turn_Num){
		 W_Test(); // �����¼���������
	}
	// ��ʱ���ܴ�
	if(device_state_init.time_state){
		device_state_init.time_count++;
		// 1���Ӽ���
		if(device_state_init.time_count % 60 == 0){
			device_state_init.time_count = 0;
			// ��ʱʱ��
			if(device_state_init.tiem ){
				device_state_init.tiem--;
			}
		}
	}
	//��ʱ�رշ���
	if(device_state_init.fan_state && device_state_init.tiem ==0 && device_state_init.time_state){
		device_state_init.fan_state = 0;
		device_state_init.time_state= 0;
		Data_init.Flage =0;
	}

	// �����ݸ�app
	if (Data_init.App)
	{
		switch (Data_init.App)
		{
		case 1:
			Send(1); // �������ݵ�APP
			break;
		}
		Data_init.App = 0;
	}
}

// ����json����
mySta massage_parse_json(char *message)
{

	cJSON *cjson_test = NULL; // ���json��ʽ
	// cJSON *cjson_data = NULL; // ����
	// const char *massage;
	// ������������
	u8 cjson_cmd; // ָ��,����

	/* ��������JSO���� */
	cjson_test = cJSON_Parse(message);
	if (cjson_test == NULL)
	{
		// ����ʧ��
		printf("parse fail.\n");
		return MY_FAIL;
	}

	/* ���θ���������ȡJSON���ݣ���ֵ�ԣ� */
	cjson_cmd = cJSON_GetObjectItem(cjson_test, "cmd")->valueint;
	/* ����Ƕ��json���� */
	//cjson_data = cJSON_GetObjectItem(cjson_test, "data");

	switch (cjson_cmd)
	{
	case 0x01: // ��Ϣ��
		Data_init.Flage = cJSON_GetObjectItem(cjson_test, "flage")->valueint;
		break;
	case 0x02: // ��Ϣ��
		device_state_init.fan_state= cJSON_GetObjectItem(cjson_test, "fan")->valueint;
		Data_init.Flage =0;
		Data_init.App = 1;
		break;
	case 0x03: // ���ݰ�
		turn_init.Turn_Num = cJSON_GetObjectItem(cjson_test, "pwm")->valueint;
		threshold_value_init.temp_value = cJSON_GetObjectItem(cjson_test, "temp_v")->valueint;
		Data_init.App = 1;
		break;
	case 0x04: // ���ݰ�
		device_state_init.tiem = cJSON_GetObjectItem(cjson_test, "time")->valueint;
		Data_init.App = 1;
		break;
	case 0x05: // ���ݰ�
		device_state_init.time_state = cJSON_GetObjectItem(cjson_test, "time_s")->valueint;
		Data_init.App = 1;
		device_state_init.time_count=0;
		break;
	default:
		break;
	}

	/* ���JSON����(��������)���������� */
	cJSON_Delete(cjson_test);

	return MY_SUCCESSFUL;
}
// ��������
mySta massage_speak(char *message)
{

	char *dataPtr = NULL;

	char numBuf[10];
	int num = 0;

	dataPtr = strchr(message, ':'); // ����':'

	if (dataPtr != NULL) // ����ҵ���
	{
		dataPtr++;
		while (*dataPtr >= '0' && *dataPtr <= '9') // �ж��Ƿ����·��������������
		{
			numBuf[num++] = *dataPtr++;
		}
		numBuf[num] = 0;
		num = atoi((const char *)numBuf); // תΪ��ֵ��ʽ
		if (strstr((char *)message, "cmd")) // ����"redled"
		{
			Data_init.cmd = num;
		}
	}
	return MY_SUCCESSFUL;
}

