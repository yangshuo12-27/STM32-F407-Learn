#include "MLX90614.h"

/*
函数功能：IIC接口初始化
硬件连接：
SDA：PA7
SCL：PA6
*/
void IIC_Init(void)
{
	RCC->APB2ENR|=1<<2;//PA
	GPIOA->CRL&=0x00FFFFFF;
	GPIOA->CRL|=0x33000000;
	GPIOA->ODR|=0x3<<6;
        
}


/*
函数功能：IIC总线起始信号
*/
void IIC_Start(void)
{
    IIC_SDA_OUTMODE(); //初始化SDA为输出模式
    IIC_SDA_OUT=1; 		 //数据线拉高
    IIC_SCL=1;     		 //时钟线拉高
    DelayUs(4);        //电平保持时间
    IIC_SDA_OUT=0; 		 //数据线拉低
    DelayUs(4);        //电平保持时间
    IIC_SCL=0;     		 //时钟线拉低
}


/*
函数功能：IIC总线停止信号
*/
void IIC_Stop(void)
{
		IIC_SDA_OUTMODE(); //初始化SDA为输出模式
		IIC_SDA_OUT=0; 		 //数据线拉低
		IIC_SCL=0;     		 //时钟线拉低
		DelayUs(4);        //电平保持时间
		IIC_SCL=1;     		 //时钟线拉高
		DelayUs(4);        //电平保持时间
		IIC_SDA_OUT=1; 		 //数据线拉高
}

/*
函数功能：获取应答信号
返 回 值：1表示失败，0表示成功
*/
u8 IIC_GetACK(void)
{
		u8 cnt=0;
		IIC_SDA_INPUTMODE();//初始化SDA为输入模式
		IIC_SDA_OUT=1; 		  //数据线上拉
	  DelayUs(2);         //电平保持时间
		IIC_SCL=0;     		  //时钟线拉低，告诉从机，主机需要数据
		DelayUs(2);         //电平保持时间，等待从机发送数据
	  IIC_SCL=1;     		  //时钟线拉高，告诉从机，主机现在开始读取数据
		while(IIC_SDA_IN)   //等待从机应答信号
		{
				cnt++;
				if(cnt>250)return 1;
		}
		IIC_SCL=0;     		  //时钟线拉低，告诉从机，主机需要数据
		return 0;
}

/*
函数功能：主机向从机发送应答信号
函数形参：0表示应答，1表示非应答
*/
void IIC_SendACK(u8 stat)
{
		IIC_SDA_OUTMODE(); //初始化SDA为输出模式
		IIC_SCL=0;     		 //时钟线拉低，告诉从机，主机需要发送数据
		if(stat)IIC_SDA_OUT=1; //数据线拉高，发送非应答信号
		else IIC_SDA_OUT=0; 	 //数据线拉低，发送应答信号
		DelayUs(2);            //电平保持时间，等待时钟线稳定
		IIC_SCL=1;     		     //时钟线拉高，告诉从机，主机数据发送完毕
		DelayUs(2);            //电平保持时间，等待从机接收数据
		IIC_SCL=0;     		  	 //时钟线拉低，告诉从机，主机需要数据
}


/*
函数功能：IIC发送1个字节数据
函数形参：将要发送的数据
*/
void IIC_WriteOneByteData(u8 data)
{
		u8 i;
		IIC_SDA_OUTMODE(); //初始化SDA为输出模式
		IIC_SCL=0;     		 //时钟线拉低，告诉从机，主机需要发送数据
		for(i=0;i<8;i++)
		{
				if(data&0x80)IIC_SDA_OUT=1; //数据线拉高，发送1
				else IIC_SDA_OUT=0; 	 //数据线拉低，发送0
				IIC_SCL=1;     		     //时钟线拉高，告诉从机，主机数据发送完毕
				DelayUs(2);            //电平保持时间，等待从机接收数据
				IIC_SCL=0;     		 		 //时钟线拉低，告诉从机，主机需要发送数据
				DelayUs(2);            //电平保持时间，等待时钟线稳定
				data<<=1;              //先发高位
		}
}


/*
函数功能：IIC接收1个字节数据
返 回 值：收到的数据
*/
u8 IIC_ReadOneByteData(void)
{
		u8 i,data;
		IIC_SDA_INPUTMODE();//初始化SDA为输入模式
	  for(i=0;i<8;i++)
	  {
			 	IIC_SCL=0;     		  //时钟线拉低，告诉从机，主机需要数据
				DelayUs(2);         //电平保持时间，等待从机发送数据
				IIC_SCL=1;     		  //时钟线拉高，告诉从机，主机现在正在读取数据
				data<<=1;           
				if(IIC_SDA_IN)data|=0x01;
				DelayUs(2);         //电平保持时间，等待时钟线稳定
	  }
		IIC_SCL=0;     		  		//时钟线拉低，告诉从机，主机需要数据 (必须拉低，否则将会识别为停止信号)
		return data;
}


void MLX90614_Init()
{
	IIC_Init();
    IIC_SDA_OUTMODE(); //初始化SDA为输出模式
    // 发送初始化命令到MLX90614
     //函数部分 
     IIC_SCL=1;IIC_SDA_OUT=1;
     DelayUs(10);
     IIC_SCL=0; 
     DelayUs(1000); 
     IIC_SCL=1; 
    
}


#define READ_TIMES 20  // 读取次数
#define SELECT_COUNT 3  // 选取最小的3 个值计算平均值

float read_MLX90614(void) 
{ 
    u8 i=0;
   u8 mlx90614_data[3]; // 存放温度数据的缓冲区
   float temperature;        // 存放计算后的温度值
        
	IIC_Start(); //发送起始信号	
	IIC_WriteOneByteData(0xB4); //设置写模式
	IIC_GetACK();//获取应答	
	IIC_WriteOneByteData(0x07); //设置读取数据的位置
	IIC_GetACK();//获取应答	
	IIC_Start(); //发送起始信号	
	IIC_WriteOneByteData(0x01); //设置读模式
	IIC_GetACK();//获取应答	
 
	for(i=0;i<3;i++)
	{
		 mlx90614_data[i]=IIC_ReadOneByteData(); //接收数据
		 IIC_SendACK(0); //发送应答信号
	}
	IIC_SendACK(1); //发送非应答信号
	IIC_Stop(); //停止信号
    
      // 计算温度值
    temperature = ((mlx90614_data[1] << 8) | mlx90614_data[0]) * 0.02 - 273.15;
    
    //temperature = mlx90614_data[1]*256+ mlx90614_data[0];
    
    return temperature;
} 




// 交换函数，用于排序
void swap(float *a, float *b) {
    float temp = *a;
    *a = *b;
    *b = temp;
}

// 冒泡排序
void sort(float arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
            }
        }
    }
}

// 计算最小的 3 个值的平均温度
float calculate_min_average() {
    float temperature[READ_TIMES];

    // 读取温度数据
    for (int i = 0; i < READ_TIMES; i++) {
        temperature[i] = read_MLX90614();
    }

    // 排序（从小到大）
    sort(temperature, READ_TIMES);

    // 计算前 3 个最小值的平均值
    float sum = 0;
    for (int i = 0; i < SELECT_COUNT; i++) {
        sum += temperature[i];
    }

    return sum / SELECT_COUNT;
}
