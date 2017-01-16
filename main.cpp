#include "stm32f4xx_conf.h"


#include "sdo.h"
#include "pdo.h"
#include "can_drv.h"
#include "mi_controller.h"
#include "led_interface.h"

#define CPU_CLK 168000000L

bool tick = false;
CanDrv canDrv;
Sdo* SDO;



extern "C"
{
	void SysTick_Handler(void);
	void CAN1_RX0_IRQHandler(void);
}



volatile uint32_t counter1 = 0, cnt2 = 0;
void GeneralHardwareInit()
{
	// Inicjalizacja mikrokontrolera



		//**************** port A
		// ustawienie predkosci pracy linii portu
		GPIOA->OSPEEDR = GPIO_OSPEEDER_OSPEEDR0_1 | GPIO_OSPEEDER_OSPEEDR4_1;
		// wybor urzadzen na odpowiednich liniach (wedlug dokumencjacji danego typu procesora)
		// wybor pracy linii portu
		GPIOA->MODER |=  GPIO_MODER_MODER1_0 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1 |GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1 | GPIO_MODER_MODER9_1 | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1;

		GPIOA->IDR |= GPIO_IDR_IDR_6;

}


void SysTick_Handler(void)
{
	if (++counter1==500)
	{
		counter1 = 0;
		Led::Yellow() ^= 1;
		tick = true;
	}
	cnt2++;
}


unsigned int Angle[1000];
int Index = 0;

void CAN1_RX0_IRQHandler(void)
{
	canDrv.IrqRead();
	SDO->StackUpdate();

	// Odczytywanie 2 z 4 bajt�w pozycji enkodera.
	if (canDrv.dataRx[canDrv.indexRx].index == 0x181)
		if (Index < 1000) Angle[Index++] = canDrv.dataRx[canDrv.indexRx].data[0];
		else int a = 0;
}



int main(void)
{
	Sdo sdo(&canDrv, 1);
	Pdo pdo(&canDrv, 1);
	SDO = &sdo;
	MiControlCmds Command;

	SystemInit();

	if (SysTick_Config(CPU_CLK/1000))
	{ // ustawienie zegara systemowego w programie
		while (1);
	}

	Led::Init();
	canDrv.Init(CanDrv::B1M);

	__enable_irq();


	sdo.PushCommand(Command.ClearError());
	sdo.PushCommand(Command.RestoreParam());
	sdo.PushCommand(Command.MotorEnable());

	sdo.PushCommand(Command.DisableRPDO());
	sdo.PushCommand(Command.MapRPDO(1, Command.SetSubvel(0), 32)); // Alternatywna sk�adnia: MapRPDO(1, 0x3500, 0, 32)
	sdo.PushCommand(Command.EnableRPDO(1));

	sdo.PushCommand(Command.DisableTPDO());
	sdo.PushCommand(Command.TransmissionType());
	sdo.PushCommand(Command.MapTPDO(1, 0x3762, 0, 32)); // Pozycja enkoderad silnika.
	//sdo.PushCommand(Command.MapTPDO(2, 0x3A04, 1, 32)); // Pr�dkosc enkoderad silnika.
	//sdo.PushCommand(Command.MapTPDO(3, 0x3262, 0, 32)); // Pr�d silnika.
	sdo.PushCommand(Command.EnableTPDO(1));

	sdo.StartSequence();

    while(!pdo.Operational)
    	if (tick)
    	{
    		tick = false;
    		if (!sdo.completed) sdo.SendTrigger();
    		else pdo.SetOperational();
    	}

    while (true) if (cnt2 > 100) { cnt2 = 0; pdo.Send(100); break; }
    while (true)
    	{
    		if (cnt2 > 5) { cnt2 = 0; pdo.Read(0x80); }
    		//if (cnt2 > 200) { pdo.Read(0x81); }
    		//if (cnt2 > 300) { cnt2 = 0; pdo.Read(0x82); }
    	}

    return 0;
}
