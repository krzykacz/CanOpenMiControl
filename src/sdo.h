#pragma once

#include "can_drv.h"
#include "stdint.h"
#include "string.h"


typedef struct
{
	uint8_t	type;
	uint16_t index;
	uint8_t subindex;
	uint8_t data[4];
	uint16_t timeout;
	int16_t trials;
} __attribute__ ((packed)) SdoCmd;


class Sdo
{

	static const uint8_t stackSize = 32;
	static const uint16_t idWrOffset = 0x600;
	static const uint16_t idRdOffset = 0x580;

	CanDrv * canDrv;

public:

	SdoCmd cmd[stackSize];								// stos wiadomosci

	volatile uint32_t data;
	volatile uint16_t cmdIndex;
	volatile uint16_t cmdNumber;					// liczba komunikatów do wyslania

	volatile bool	completed;
	volatile bool	started;
	volatile bool	transmitted;
	volatile bool	received;

	volatile bool	startTriger;

	volatile uint16_t time;
	volatile int16_t	trials;

	uint8_t id;
	uint16_t idWr;
	uint16_t idRd;

	Sdo(CanDrv * canDrv, uint16_t id)
	{
		cmdNumber = 0;
		this->id = id;
		this->canDrv = canDrv;
		cmdIndex = 0;
		idWr = id + idWrOffset;
		idRd = id + idRdOffset;
	}

	void Reset()
	{
		time = cmd[cmdIndex].timeout;
		//mask = m->mbox_tx_mask;
	}

	void PushCommand(SdoCmd * pCmd)
	{
		memcpy((void*)&cmd[cmdNumber++], (void*)pCmd, sizeof(SdoCmd));
		cmdNumber %= stackSize;
	}

	// wypełnienie skrzynki trescia
	void PrepareData()
	{
		SdoCmd * cmd = &cmd[cmdIndex];

		canDrv-
		mailboxData[0] = *(uint32_t*)&cmd->type;
		mailboxData[1] = *(uint32_t*)cmd->data;

		time = cmd->timeout;
		trials = cmd->trials;
		transmitted = 0;
		received = 0;
	}

	void StartSequence()
	{
		startTrigger = true;
	}


	bool StackWriteUpdate()
	{
		bool newCmd = false;
		if (completed) return false;
		if (!trials) return false;

		if (startTriger)
		{
			startTriger = false;
			completed = false;
		}
		else
		{
			// czy potwierdzono przeslanie danych?
			if (received)
			{
				cmdIndex++;
				cmdIndex %= stackSize;
				if (++cmdIndex < cmdNumber)
				{
					PrepareData();
					newCmd = true;
				}
				else
				{
					completed = true;
				}
			}
			else
			{
				if (!(--time))
				{
					Reset();
					PrepareData();
					newCmd = true;
					if (trials>0) trials--;
				}
			}
		}
		return newCmd;
	}

	void CheckAnswer()
	{


	}


};

