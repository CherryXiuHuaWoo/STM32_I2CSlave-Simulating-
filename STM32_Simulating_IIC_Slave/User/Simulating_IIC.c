
#include "Simulating_IIC.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "usart.h"

#define IIC_SDA_Pin			GPIO_PIN_9
#define IIC_SDA_GPIO_Port 	GPIOA
#define IIC_SCL_Pin 		GPIO_PIN_10
#define IIC_SCL_GPIO_Port 	GPIOA

#define IIC_NoMasterAck		-1
#define IIC_GetMasterAck	0

#define IIC_SlaveAddr_R		0x55
#define IIC_SlaveAddr_W		0x54

#define IIC_SDA_OUTPUT		0
#define IIC_SDA_EXTI		1

#define FAIL				-1
#define PASS				0

struct IIC_RxBuf
{ 
	uint8_t SlaveAddr;
	uint8_t RxBuf[8];
}Slave_IICBuf;


GPIO_InitTypeDef GPIO_InitStruct;

/*
* Funcaion:
 */
void IIC_SDA_TransStatus(int status)
{
  GPIO_InitStruct.Pin = IIC_SDA_Pin;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  if(status == IIC_SDA_OUTPUT)
  {
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  }
  else
  {
	  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  }
  HAL_GPIO_Init(IIC_SDA_GPIO_Port, &GPIO_InitStruct);
}

/*
 * Function: IIC delay
 * 			 A short delay for Master Clock
 */
void IIC_delay(void)
{
	int i;
	for(i = 0; i < 8000; i++)
	{
	
	}
}



/*
 * Function: IIC Slave ACK
 */
 void IIC_ACK()
{
	/*Waitting for High CLK*/
//	while(HAL_GPIO_ReadPin(IIC_SCL_GPIO_Port, IIC_SCL_Pin) == GPIO_PIN_RESET);

	/*SDA Reset*/
	HAL_GPIO_WritePin(IIC_SDA_GPIO_Port, IIC_SDA_Pin, GPIO_PIN_RESET);
	/*Waitting for Low CLK*/
	IIC_delay();
//	while(HAL_GPIO_ReadPin(IIC_SCL_GPIO_Port, IIC_SCL_Pin) == GPIO_PIN_SET);

	/*SDA Set*/
	HAL_GPIO_WritePin(IIC_SDA_GPIO_Port, IIC_SDA_Pin, GPIO_PIN_SET);
}



/*
 * Function: Receive Master Ack
 */
 int IIC_ReceiveMasterAck()
 {
 	int Ack = 0;
	/*waititing for High CLK*/
	while((IIC_SCL_GPIO_Port->IDR & (uint32_t)IIC_SCL_Pin) == GPIO_PIN_RESET);

	if((IIC_SDA_GPIO_Port->IDR & (uint32_t)IIC_SDA_Pin) != GPIO_PIN_RESET)
	{
		Ack = IIC_NoMasterAck;
	}
	else
	{
		Ack = IIC_GetMasterAck;
	}

	 /*Waitting for Low CLK*/
	 while((IIC_SCL_GPIO_Port->IDR & (uint32_t)IIC_SCL_Pin) != GPIO_PIN_RESET);
	 
	 return Ack;
 }



/*
 * Function: IIC Slave Send One Byte
 */
void IIC_SendOneByte(uint8_t data)
{
	int i = 0;
	uint8_t BitStatus = 0;

	for(i = 8; i > 0; i--)
	{
		/*Waitting for High CLK*/
		while((IIC_SCL_GPIO_Port->IDR & (uint32_t)IIC_SCL_Pin) == GPIO_PIN_RESET);

		BitStatus = (data >> i) & 0x01;

		if(BitStatus == 0x01)
		{
			/*SDA Set*/
			IIC_SDA_GPIO_Port->BSRR = (uint32_t)IIC_SDA_Pin;
		}
		else
		{
			/*SDA Reset*/
			IIC_SDA_GPIO_Port->BRR = (uint32_t)IIC_SDA_Pin;
		}

		/*Waitting for Low CLK*/
		while((IIC_SCL_GPIO_Port->IDR & (uint32_t)IIC_SCL_Pin) != GPIO_PIN_RESET);
	}
}



/*
 * Function: IIC Slave Send bytes
 */
int IIC_SendBytes(uint8_t *buf, int len)
{
	int i = 0;
	
	for(i = 0; i < len; i++)
	{
		IIC_SendOneByte(buf[i]);
		if(IIC_ReceiveMasterAck != IIC_GetMasterAck)
		{
			return FAIL;
		}
	}
	return PASS;
}

int IIC_StartFlag = 0;
uint8_t test[8] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
/*
 * Function: HAL_GPIO_EXTI_Callback
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	int i = 0, cnt = 0;
	
	/*Disables EXTI interrupt*/
	HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);

	/*Checking the Start Sign. Start Sign: SDA = Low, SCL = High*/	
	if(HAL_GPIO_ReadPin(IIC_SCL_GPIO_Port, IIC_SCL_Pin) == GPIO_PIN_SET)
	{	
		/*Waitting Start Sign end, SCL Low.*/
		while(HAL_GPIO_ReadPin(IIC_SCL_GPIO_Port, IIC_SCL_Pin) == GPIO_PIN_SET);
		
		/*Reset Buf*/
		Slave_IICBuf.SlaveAddr = 0;
		
		/*Read Slave Address*/
		for(i = 8; i > 0; i--)
		{
			/*Waitting for SCL High*/
			while(HAL_GPIO_ReadPin(IIC_SCL_GPIO_Port, IIC_SCL_Pin) == GPIO_PIN_RESET);
			
			Slave_IICBuf.SlaveAddr <<= 1;
			Slave_IICBuf.SlaveAddr |= (HAL_GPIO_ReadPin(IIC_SDA_GPIO_Port, IIC_SDA_Pin) != GPIO_PIN_RESET) ? 0x01 : 0x00;

			/*Waitting for SCL Low*/
			while(HAL_GPIO_ReadPin(IIC_SCL_GPIO_Port, IIC_SCL_Pin) == GPIO_PIN_SET);
		}
		
		/*Transfrom SDA Mode as OUTPUT*/
		IIC_SDA_TransStatus(IIC_SDA_OUTPUT);
		printf("Addr=0x%x\r\n",Slave_IICBuf.SlaveAddr);
		/*Checking IIC Slave Address*/
		if(Slave_IICBuf.SlaveAddr == IIC_SlaveAddr_R)
		{
			
			IIC_ACK();		
//			IIC_SendBytes(test, 8);
		}
		else if(Slave_IICBuf.SlaveAddr == IIC_SlaveAddr_W)
		{
			IIC_ACK();
		}

		
		
	}
	else
	{
		printf("No Start Sign\r\n");
	}

	/*Transfrom SDA Mode as OUTPUT*/
	IIC_SDA_TransStatus(IIC_SDA_EXTI);
	/*Enables EXTI interrupt*/
	__HAL_GPIO_EXTI_CLEAR_FLAG(IIC_SDA_Pin);
	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}
 
