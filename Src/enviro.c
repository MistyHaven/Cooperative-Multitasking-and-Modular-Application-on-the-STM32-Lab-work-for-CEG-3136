// Environmental monitor app


#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "enviro.h"
#include "spi.h"
#include "display.h"
#include "touchpad.h"
#include "systick.h"
static enum {WAIT_INIT, GET_PARAMS, WAIT_PARAMS, TRIGGER_MEAS, WAIT_STATUS, MEAS_READY} state;
#define READ 0x80
// --------------------------------------------------------
// SPI Transfers
// --------------------------------------------------------
// Environmental sensor write
typedef struct {
	uint8_t ctrl; // Write command (RW=0) and 7-bit address
	uint8_t data; // Write data byte
} EnvWrite_t;
////////////////////////////////
// Initialization


// Select Page 1 (addr 0x00..0x7F)
static const EnvWrite_t txPage1 = {0x73, 0x10};
static SPI_Xfer_t Page1 = {&EnvSPI, TX, (void *)&txPage1, 2, 1};


// Select Page 0 (addr 0x80..0xFF)
static const EnvWrite_t txPage0 = {0x73, 0x00};
static SPI_Xfer_t Page0 = {&EnvSPI, TX, (void *)&txPage0, 2, 1};


// Reset sensor
// Refer to datasheet 5.3.1.5 and Table 20
static const EnvWrite_t txResetSensor = {0x60, 0xB6}; // Page 1
static SPI_Xfer_t ResetSensor = {&EnvSPI, TX, (void *)&txResetSensor, 2, 1};


// Read ID
// Refer to datasheet 5.3.1.6 and Table 20
static uint8_t rxId[1];
static const EnvWrite_t txIdAddr = {0x50|READ}; // Page 1
static SPI_Xfer_t ReadId1 = {&EnvSPI, TX, (void *)&txIdAddr, 1, 0};
static SPI_Xfer_t ReadId2 = {&EnvSPI, RX, (void *)&rxId[0], 1, 1};


// Configure oversampling (temperature and humidity)
// Refer to datasheet 3.2.1 and Table 20
static const EnvWrite_t txOvsp[2] = {{0x74, 0x41}, {0x72, 0x01}}; // Page 1
static SPI_Xfer_t SetOvsp = {&EnvSPI, TX, (void *)&txOvsp[0], 4, 1};


////////////////////////////////
// Get Calibration Parameters (temperature)
// Refer to datasheet Table 11
static uint16_t par_t[3]; // = {par_t1, par_t2, par_t3}
static const EnvWrite_t txParTAddr1 = {0xE9|READ}; // Page 0
static SPI_Xfer_t ParT1 = {&EnvSPI, TX, (void *)&txParTAddr1, 1, 0};
static SPI_Xfer_t ParT2 = {&EnvSPI, RX, (void *)&par_t[0], 2, 1};
static const EnvWrite_t txParTAddr2 = {0x8A|READ}; // Page 0
static SPI_Xfer_t ParT3 = {&EnvSPI, TX, (void *)&txParTAddr2, 1, 0};
static SPI_Xfer_t ParT4 = {&EnvSPI, RX, (void *)&par_t[1], 3, 1};


////////////////////////////////
// Get Calibration Parameters (humidity)
// Refer to datasheet Table 13
static uint16_t par_h[8];
static uint8_t rx_buffer_hum[9];
static const EnvWrite_t txParHAddr = {0xE1|READ}; // Page 0
static SPI_Xfer_t ParH1 = {&EnvSPI, TX, (void *)&txParHAddr, 1, 0};
static SPI_Xfer_t ParH2 = {&EnvSPI, RX, (void *)&rx_buffer_hum[0], 9, 1};


void ProcessHumidityParameters (void) {
	// Byte 1 split and combined with bytes 0 and 2
	// Refer to datasheet Table 13
	par_h[0] = (rx_buffer_hum[2] << 4) | (rx_buffer_hum[1] & 0x0F);
	par_h[1] = (rx_buffer_hum[0] << 4) | ((rx_buffer_hum[1] & 0xF0) >> 4);


	// Remaining bytes assigned 1:1
	for (uint8_t i=0; i<5; i++) {
		par_h[2+i] = rx_buffer_hum[3+i];
	}
}
////////////////////////////////
// Measurement


// Trigger measurement
// Refer to datasheet 3.2.1, step 8 (combine with existing settings!)
static const EnvWrite_t txTrigMeasAddr = {0x74, 0x41}; // Page 1
static SPI_Xfer_t TrigMeas = {&EnvSPI, TX, (void *)&txTrigMeasAddr, 2, 1};


// Check Status
// Refer to datasheet 5.3.5.1 and Table 20
static uint8_t status;
static const EnvWrite_t txStatusAddr = {0x1D|READ}; // Page 1
static SPI_Xfer_t Status1 = {&EnvSPI, TX, (void *)&txStatusAddr, 1, 0};
static SPI_Xfer_t Status2 = {&EnvSPI, RX, (void *)&status, 1, 1};


// Read Humidity
// Refer to datasheet 5.3.4.3 and Table 20
static uint8_t humidityData[2];
static const EnvWrite_t txHumAddr = {0x25|READ}; // Page 1
static SPI_Xfer_t Hum1 = {&EnvSPI, TX, (void *)&txHumAddr, 1, 0};
static SPI_Xfer_t Hum2 = {&EnvSPI, RX, (void *)&humidityData, 2, 1};


// Read Temperature
// Refer to datasheet 5.3.4.2 and Table 20
static uint8_t tempData[3];
static const EnvWrite_t txTempAddr = {0x22|READ}; // Page 1
static SPI_Xfer_t Temp1 = {&EnvSPI, TX, (void *)&txTempAddr, 1, 0};
static SPI_Xfer_t Temp2 = {&EnvSPI, RX, (void *)&tempData, 3, 1};


// --------------------------------------------------------
// Calculations and display
// --------------------------------------------------------


static double CalcTemperature() {
	uint32_t temp_adc;
	double var1, var2, t_fine, temp_comp;


	// Refer to datasheet 3.3.1 and Table 11
	temp_adc = tempData[0]<<12|tempData[1]<<4|tempData[2]>>4; ;


	uint16_t par_t1 = par_t[0];
	uint16_t par_t2 = par_t[1];
	uint16_t par_t3 = par_t[2];
	var1 = (((double) temp_adc / 16384.0) - ((double) par_t1 / 1024.0)) * (double)par_t2;
	var2 = ((((double) temp_adc/131072.0) - ((double) par_t1 / 8192.0)) *(((double) temp_adc / 131072.0) - ((double) par_t1 / 8192.0))) *((double)par_t3 * 16.0);
	t_fine = var1 + var2;
	temp_comp = t_fine / 5120.0;
	return temp_comp;
}

static double CalcHumidity(double temp_comp) {
	 uint16_t hum_adc;
	 double var1, var2, var3, var4, hum_comp;
	 // Refer to datasheet 3.3.3 and Table 13

	 hum_adc = humidityData[0]<<8|humidityData[1];

	 uint16_t par_h1 = par_h[0];
	 uint16_t par_h2 = par_h[1];
	 uint16_t par_h3 = par_h[2];
	 uint16_t par_h4 = par_h[3];
	 uint16_t par_h5 = par_h[4];
	 uint16_t par_h6 = par_h[5];
	 uint16_t par_h7 = par_h[6];
	 //uint16_t par_h8 = par_h[7];
	 var1 = hum_adc - (((double) par_h1* 16.0) + (((double) par_h3 / 2.0) * temp_comp));
	 var2 = var1 * (((double)par_h2 / 262144.0) * (1.0 + (((double) par_h4 / 16384.0) * temp_comp) + (((double)par_h5 / 1048576.0) * temp_comp * temp_comp)));
	 var3 = (double)par_h6 / 16384.0;
	 var4 = (double)par_h7 / 2097152.0;
	 hum_comp = var2 + ((var3 + (var4 * temp_comp)) * var2 * var2);
	 return hum_comp;
}
static void ProcessEnvData (void) {
	double temperature;
	double humidity;
	temperature = CalcTemperature();
	humidity = CalcHumidity(temperature);


	// Display temperature and humidity to 1 decimal place
	DisplayPrint(ENVIRO,0,"Hum  %3.1f%%", humidity);
	DisplayPrint(ENVIRO,1,"Temp %3.1f'C", temperature);
}
// --------------------------------------------------------
// Initialization procedure
// --------------------------------------------------------
void Init_Enviro (void) {
	DisplayEnable();  //display init
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
 // Enable access to Floating Point Unit (FPU)
 SCB->CPACR |= 0b11 << 10*2 | 0b11 << 11*2;

#endif


 	 SPI_Enable(EnvSPI);


 	 //Initialize Humidity/Temperature Sensor
 	 SPI_Request(&Page0);
 	 SPI_Request(&ResetSensor);
 	 SPI_Request(&ReadId1);
 	 SPI_Request(&ReadId2);
 	 SPI_Request(&SetOvsp);
 	 state = WAIT_INIT;
}


// --------------------------------------------------------
// Task state machine
// --------------------------------------------------------


void Task_Enviro (void) {

	switch (state) {


	case WAIT_INIT:
		// Wait for initialization to complete
		if (!SetOvsp.busy) {
			printf("Read ID: %x\n",rxId[0]);
			if (rxId[0] != 0x61) {
				printf("ERROR: Read ID incorrect!\n");
			}
			state = GET_PARAMS;
		}
		break;


	case GET_PARAMS:
		// Get calibration parameters for temperature and humidity
		// Refer to SPI transfers above
		SPI_Request(&ParT1);
		SPI_Request(&ParT2);
		SPI_Request(&ParT3);
		SPI_Request(&ParT4);


		SPI_Request(&ParH1);
		SPI_Request(&ParH2);


		state = WAIT_PARAMS;
		break;


	case WAIT_PARAMS:
		// Wait for reads to complete
		if (!ParH2.busy) {
			// Temperature parameters saved in place
			// Humidity parameters require some processing
			ProcessHumidityParameters();
			state = TRIGGER_MEAS;
		}
		break;


	case TRIGGER_MEAS:
		// Trigger humidity and temperature measurement
		SPI_Request(&Page1);
		SPI_Request(&TrigMeas);
		SPI_Request(&Status1);
		SPI_Request(&Status2);
		state = WAIT_STATUS;
		break;


	case WAIT_STATUS:
		// Wait for status to show complete
		if (!(Status2.busy)) {
			if (status & 0x80) {
				// Read humidity and temperature
				// Refer to SPI transfers above
				SPI_Request(&Hum1);
				SPI_Request(&Hum2);


				SPI_Request(&Temp1);
				SPI_Request(&Temp2);


				state = MEAS_READY;
				break;
			}
			else {
				// Read status again
				SPI_Request(&Status1);
				SPI_Request(&Status2);
			}
		}
		break;


	case MEAS_READY:
		// Wait for reads to complete
		if (!Temp2.busy) {
			ProcessEnvData(); // Calculate temperature and humidity
			state = TRIGGER_MEAS; // Start the next measurement
		}
		break;
	}
}
