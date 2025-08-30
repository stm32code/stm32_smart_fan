#include "git.h"

Data_TypeDef Data_init;						  // 设备数据结构体
Threshold_Value_TypeDef threshold_value_init; // 设备阈值设置结构体
Device_Satte_Typedef device_state_init;		  // 设备状态
DHT11_Data_TypeDef DHT11_Data;
// 获取数据参数
mySta Read_Data(Data_TypeDef *Device_Data)
{
	Read_DHT11(&DHT11_Data); // 获取温湿度数据
	Device_Data->temperatuer = DHT11_Data.temp_int + DHT11_Data.temp_deci * 0.01;
	Device_Data->humiditr = DHT11_Data.humi_int + DHT11_Data.humi_deci * 0.01;

	return MY_SUCCESSFUL;
}
// 初始化
mySta Reset_Threshole_Value(Threshold_Value_TypeDef *Value, Device_Satte_Typedef *device_state)
{

  //写
	//W_Test();
	// 读
  R_Test();
	device_state_init.turn = 15;
	threshold_value_init.temp_value = 25;
	
	return MY_SUCCESSFUL;
}
// 更新OLED显示屏中内容
mySta Update_oled_massage()
{
#if OLED // 是否打开
	char str[50];
	// 自动模式
	if(Data_init.Flage == 1){
		sprintf(str, "摇头模式   ");
	}else
		sprintf(str, "固定模式   ");
	OLED_ShowCH(0, 0, (unsigned char *)str);
	if(device_state_init.time_state){
		sprintf(str, "Time: %d min 开 ",device_state_init.tiem);
	}else 
		sprintf(str, "Time: %d min 关 ",device_state_init.tiem);
	OLED_ShowCH(0, 2, (unsigned char *)str);
	sprintf(str, "风扇转速 : %d ", turn_init.Turn_Num);
	OLED_ShowCH(0, 4, (unsigned char *)str);
	if(device_state_init.fan_state){
		sprintf(str, "风扇状态 : 开 ");
	}else 
		sprintf(str, "风扇状态 : 关 ");
	OLED_ShowCH(0, 6, (unsigned char *)str);
#endif

	return MY_SUCCESSFUL;
}

// 更新设备状态
mySta Update_device_massage()
{
	// 旋转 
	if(Data_init.Flage == 1){
		if(device_state_init.turn_state == 0 && device_state_init.turn > 5 ){
			device_state_init.turn --;
			if(device_state_init.turn == 5){
				// 切换
				device_state_init.turn_state = 1;
			}
		}else if(device_state_init.turn_state  && device_state_init.turn < 24 )
		{
			device_state_init.turn++;
			if(device_state_init.turn == 24){
				// 切换
				device_state_init.turn_state = 0;
			}
		}
	}
	TIM_SetCompare2(TIM3,device_state_init.turn);
	//防止抖动
	delay_ms(10);
	
	// PWM 调控
	if(turn_init.Turn_Num > 0 && device_state_init.fan_state == 1){
		TIM_SetCompare1(TIM3,turn_init.Turn_Num * 2);
	}else{
		TIM_SetCompare1(TIM3,0);
	}

	// 执行语音模块发来消息
	if (Data_init.cmd)
	{
		switch (Data_init.cmd)
		{
		case 1:
				// 开风扇
				device_state_init.fan_state = 1;
			break;
		case 2:
				// 关风扇
				device_state_init.fan_state = 0;
				Data_init.Flage =0;
			break;
		case 3:
				// 调快
				turn_init.Turn_Num = turn_init.Turn_Num + 20;
				if (turn_init.Turn_Num > 100)
				{
					turn_init.Turn_Num = 100;
				}
			break;
		case 4:
				// 调慢
				turn_init.Turn_Num = turn_init.Turn_Num - 20;
				if (turn_init.Turn_Num < 0)
				{
					turn_init.Turn_Num = 0;
				}
			break;
		 case 5:
				// 摇头模式
				Data_init.Flage = 1;
		 	break;
		 case 6:
				// 固定模式
				Data_init.Flage = 0;
			break; 
		}
		Data_init.cmd = 0;
	}	
	return MY_SUCCESSFUL;
}

// 定时器
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
	
	// 实现1s定时
	if(turn_init.Turn_Num_copy != turn_init.Turn_Num){
		 W_Test(); // 保存记录，掉电记忆
	}
	// 定时功能打开
	if(device_state_init.time_state){
		device_state_init.time_count++;
		// 1分钟计数
		if(device_state_init.time_count % 60 == 0){
			device_state_init.time_count = 0;
			// 定时时间
			if(device_state_init.tiem ){
				device_state_init.tiem--;
			}
		}
	}
	//定时关闭风扇
	if(device_state_init.fan_state && device_state_init.tiem ==0 && device_state_init.time_state){
		device_state_init.fan_state = 0;
		device_state_init.time_state= 0;
		Data_init.Flage =0;
	}

	// 发数据给app
	if (Data_init.App)
	{
		switch (Data_init.App)
		{
		case 1:
			Send(1); // 发送数据到APP
			break;
		}
		Data_init.App = 0;
	}
}

// 解析json数据
mySta massage_parse_json(char *message)
{

	cJSON *cjson_test = NULL; // 检测json格式
	// cJSON *cjson_data = NULL; // 数据
	// const char *massage;
	// 定义数据类型
	u8 cjson_cmd; // 指令,方向

	/* 解析整段JSO数据 */
	cjson_test = cJSON_Parse(message);
	if (cjson_test == NULL)
	{
		// 解析失败
		printf("parse fail.\n");
		return MY_FAIL;
	}

	/* 依次根据名称提取JSON数据（键值对） */
	cjson_cmd = cJSON_GetObjectItem(cjson_test, "cmd")->valueint;
	/* 解析嵌套json数据 */
	//cjson_data = cJSON_GetObjectItem(cjson_test, "data");

	switch (cjson_cmd)
	{
	case 0x01: // 消息包
		Data_init.Flage = cJSON_GetObjectItem(cjson_test, "flage")->valueint;
		break;
	case 0x02: // 消息包
		device_state_init.fan_state= cJSON_GetObjectItem(cjson_test, "fan")->valueint;
		Data_init.Flage =0;
		Data_init.App = 1;
		break;
	case 0x03: // 数据包
		turn_init.Turn_Num = cJSON_GetObjectItem(cjson_test, "pwm")->valueint;
		threshold_value_init.temp_value = cJSON_GetObjectItem(cjson_test, "temp_v")->valueint;
		Data_init.App = 1;
		break;
	case 0x04: // 数据包
		device_state_init.tiem = cJSON_GetObjectItem(cjson_test, "time")->valueint;
		Data_init.App = 1;
		break;
	case 0x05: // 数据包
		device_state_init.time_state = cJSON_GetObjectItem(cjson_test, "time_s")->valueint;
		Data_init.App = 1;
		device_state_init.time_count=0;
		break;
	default:
		break;
	}

	/* 清空JSON对象(整条链表)的所有数据 */
	cJSON_Delete(cjson_test);

	return MY_SUCCESSFUL;
}
// 解析数据
mySta massage_speak(char *message)
{

	char *dataPtr = NULL;

	char numBuf[10];
	int num = 0;

	dataPtr = strchr(message, ':'); // 搜索':'

	if (dataPtr != NULL) // 如果找到了
	{
		dataPtr++;
		while (*dataPtr >= '0' && *dataPtr <= '9') // 判断是否是下发的命令控制数据
		{
			numBuf[num++] = *dataPtr++;
		}
		numBuf[num] = 0;
		num = atoi((const char *)numBuf); // 转为数值形式
		if (strstr((char *)message, "cmd")) // 搜索"redled"
		{
			Data_init.cmd = num;
		}
	}
	return MY_SUCCESSFUL;
}

