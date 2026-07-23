#include "esp8266.h"

/*
函数功能: ESP8266命令发送函数
函数返回值:0表示成功  1表示失败
*/
u8 ESP8266_SendCmd(char *cmd)
{
    u8 i,j;
	USART2_RX_FLAG=0;
	USART2_RX_CNT=0;
    for(i=0;i<5;i++) //检测的次数--发送指令的次数
    {
		//清除缓冲区
		USART2_RX_FLAG=0;
		USART2_RX_CNT=0;
		memset(USART2_RX_BUFFER,0,sizeof(USART2_RX_BUFFER));
		
        USARTx_StringSend(USART2,cmd);
        for(j=0;j<10;j++) //等待的时间
        {
            delay_ms(100);
            if(USART2_RX_FLAG)
            {
                USART2_RX_BUFFER[USART2_RX_CNT]='\0';
                USART2_RX_FLAG=0;
                USART2_RX_CNT=0;
                printf("%s\r\n",USART2_RX_BUFFER);
                if(strstr((char*)USART2_RX_BUFFER,"OK"))
                {
                    return 0;
                }
            }
        }
    }
    return 1;
}




/*
函数功能: ESP8266命令发送函数
函数返回值:0表示成功  1表示失败


char *cmd

u32 wait_time   等待的时间长短 --毫秒

char *find_str   查找的返回值

*/
u8 ESP8266_SendCmd_time(char *cmd,u32 wait_time,char *find_str)
{
    u32 i,j;
    for(i=0;i<5;i++) //检测的次数--发送指令的次数
    {
		//清除缓冲区
		USART2_RX_FLAG=0;
		USART2_RX_CNT=0;
		memset(USART2_RX_BUFFER,0,sizeof(USART2_RX_BUFFER));
		
        USARTx_StringSend(USART2,cmd);
        for(j=0;j<10;j++) //等待的时间
        {
            delay_ms(wait_time);
            if(USART2_RX_FLAG)
            {
                USART2_RX_BUFFER[USART2_RX_CNT]='\0';
                USART2_RX_FLAG=0;
                USART2_RX_CNT=0;
                
                printf("WIFI: %s\r\n",USART2_RX_BUFFER);
                
                if(strstr((char*)USART2_RX_BUFFER,find_str))
                {
                    return 0;
                }
                
                if(strstr((char*)USART2_RX_BUFFER,"OK"))
                {
                    return 0;
                }
            }
        }
    }
    return 1;
}


/*
函数功能: ESP8266硬件初始化检测函数
函数返回值:0表示成功  1表示失败
*/
u8 ESP8266_Init(void)
{
    //退出透传模式
    USARTx_StringSend(USART2,"+++");
    delay_ms(100);
     //退出透传模式
    USARTx_StringSend(USART2,"+++");
    delay_ms(100);
    return ESP8266_SendCmd("AT\r\n");
}


/*
函数功能: 配置WIFI为STA模式+TCP客户端模式
函数参数:
char *ssid  创建的热点名称
char *pass  创建的热点密码 （最少8位）
char *p     将要连接的服务器IP地址
u16 port    将要连接的服务器端口号
u8 flag     1表示开启透传模式 0表示关闭透传模式
函数返回值:0表示成功  其他值表示对应的错误
*/
u8 ESP8266_STA_TCP_Client_Mode(char *ssid,char *pass,char *ip,u16 port,u8 flag)
{
    int i=0;
    char ESP8266_SendCMD[100]; //组合发送过程中的命令
    //退出透传模式
    //USARTx_StringSend(USART2,"+++");
    //delay_ms(50);
    /*1. 测试硬件*/
    if(ESP8266_SendCmd("AT\r\n"))return 1;
    /*2. 关闭回显*/
    //if(ESP8266_SendCmd("ATE0\r\n"))return 2;
    /*3. 设置WIFI模式*/
    if(ESP8266_SendCmd("AT+CWMODE=1\r\n"))return 3;
    /*4. 复位*/
    ESP8266_SendCmd("AT+RST\r\n");
    delay_ms(1000);
    delay_ms(1000);
    delay_ms(1000);
    
    
    /*6. 配置将要连接的WIFI热点信息*/
    for(i=0;i<20;i++)
    {
         sprintf(ESP8266_SendCMD,"AT+CWJAP=\"%s\",\"%s\"\r\n",ssid,pass);
         if(ESP8266_SendCmd_time(ESP8266_SendCMD,5000,"WIFI GOT IP")==0)
         {
            break;
         } 
    }
    
    //最终无力回天，还是连接不上WIFI
    if(i>=20)
    {
        printf("WIFI热点连接不上...\r\n");
        return 6;
    }
    
    

    //if(ESP8266_SendCmd(ESP8266_SendCMD))return 6;
    
    /*7. 设置单连接*/
    if(ESP8266_SendCmd("AT+CIPMUX=0\r\n"))return 7;
    
    //强制关闭所有链接
    //USARTx_StringSend(USART2,"AT+CIPSHUT\r\n");
    
    for(i=0;i<3;i++)
    {
        /*8. 配置要连接的TCP服务器信息*/
        sprintf(ESP8266_SendCMD,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",ip,port);
        if(ESP8266_SendCmd(ESP8266_SendCMD)==0)
        {
            break;
        }
        delay_ms(1000);
    }
  
    //最终无力回天，还是连接不上服务器
    if(i>=3)
    {
        printf("服务器连接不上...\r\n");
        return 8;
    }
    
    
     /*9. 开启透传模式*/
    if(flag)
    {
       if(ESP8266_SendCmd("AT+CIPMODE=1\r\n"))return 9; //开启
       if(ESP8266_SendCmd("AT+CIPSEND\r\n"))return 10;  //开始透传
       if(!(strstr((char*)USART2_RX_BUFFER,">")))
       {
            return 11;
       }
        //如果想要退出发送:  "+++"
    }

    printf("WIFI模式:STA+TCP客户端\r\n");
    printf("Connect_WIFI热点名称:%s\r\n",ssid);
    printf("Connect_WIFI热点密码:%s\r\n",pass);
    printf("TCP服务器端口号:%d\r\n",port);
    printf("TCP服务器IP地址:%s\r\n",ip);
    return 0;
}


/*
函数功能: TCP客户端模式下的发送函数
发送指令: 
*/
u8 ESP8266_ClientSendData(u8 *data,u16 len)
{
    u8 i,j,n;
    char ESP8266_SendCMD[100]; //组合发送过程中的命令
    for(i=0;i<10;i++)
    {
        sprintf(ESP8266_SendCMD,"AT+CIPSEND=%d\r\n",len);
        USARTx_StringSend(USART2,ESP8266_SendCMD);
        for(j=0;j<10;j++)
        {
            delay_ms(50);
            if(USART2_RX_FLAG)
            {
                USART2_RX_BUFFER[USART2_RX_CNT]='\0';
                USART2_RX_FLAG=0;
                USART2_RX_CNT=0;
                if(strstr((char*)USART2_RX_BUFFER,">"))
                {
                    //继续发送数据
                    USARTx_DataSend(USART2,data,len);
                    //等待数据发送成功
                    for(n=0;n<200;n++)
                    {
                        delay_ms(50);
                        if(USART2_RX_FLAG)
                        {
                            USART2_RX_BUFFER[USART2_RX_CNT]='\0';
                            USART2_RX_FLAG=0;
                            USART2_RX_CNT=0;
                            if(strstr((char*)USART2_RX_BUFFER,"SEND OK"))
                            {
                                return 0;
                            }
                         }            
                    }   
                }
            }
        }
    }
    return 1;
}

