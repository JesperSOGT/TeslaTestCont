/*
Software only for testing and/or learning.
Its author is not responsible for any personal or material damage.
The source code is experimental, does not follow any standard, is disorganized, and contains unfinished or unused variables and/or functions.
This software is developed for the LilyGo t-can485 ESP32 microcontroller, its purpose is to close contacor on a Tesla model 3.
*/
#include <driver/twai.h>
#include <Arduino.h>

// This variable sets the mode for sending CAN bus messages. If Mode1 is true, it will use sendingCANbusV1_task(), and if false, it will use sendingCANbusV2_task().
constexpr bool Mode1 = true;

// These lines define the GPIO pins used for CAN bus communication and other purposes
#define CAN_TX_GPIO  GPIO_NUM_27
#define CAN_RX_GPIO  GPIO_NUM_26
#define CAN_SE_GPIO  GPIO_NUM_23
#define PIN_5V_GPIO  GPIO_NUM_16

// Setup IO Pins
void setupPines()
{
	pinMode(PIN_5V_GPIO, OUTPUT);
	digitalWrite(PIN_5V_GPIO, HIGH);

	pinMode(CAN_SE_GPIO, OUTPUT);
	digitalWrite(CAN_SE_GPIO, LOW);
}

//Setup CANbus
void setupCANbus()
{
	constexpr twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // Define speed here if needed
	static constexpr twai_filter_config_t f_config = {
		.acceptance_code = 0, .acceptance_mask = 0xFFFFFFFF, .single_filter = true
	};

	//Set to NO_ACK mode due to self testing with single module
	twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NORMAL);

	// Configure
	if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
	{
		Serial.println("CANbus controller installed");
		if (twai_start() == ESP_OK)
		{
			Serial.println("CANbus service started");
		}
		else
		{
			Serial.println("Unable to start CANbus service");
		}
	}
	else
	{
		Serial.println("CANbus service not installed");
	}
}

// This function converts a buffer of bytes into a hexadecimal string for debugging purposes
String bufferToString(String text, const uint8_t* buffer)
{
	char data[64]{};
	const int size = sprintf(data, "%02x %02x %02x %02x %02x %02x %02x %02x", buffer[0], buffer[1],
	                         buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
	text += String(data, size);
	return text;
}

// This function sends a CAN bus message with the specified identifier, data buffer, data length, and whether it uses a 29-bit identifier. It also logs any errors.
void send_CANbus_message(const uint32_t identifier, const uint8_t* buffer, const uint8_t length,
                         const bool id29bits = false)
{
	twai_message_t message;
	String logmessage = "";
	message.data_length_code = length;
	String text = "";
	if (id29bits)
	{
		message.identifier = identifier;
		message.flags = TWAI_MSG_FLAG_EXTD;
	}
	else
	{
		message.identifier = identifier;
		message.flags = TWAI_MSG_FLAG_NONE;
	}

	if (length > 0 && buffer != nullptr)
	{
		memcpy(message.data, buffer, length);
		logmessage = bufferToString(text, buffer);
	}

	const esp_err_t error = twai_transmit(&message, pdMS_TO_TICKS(250));

	if (error != ESP_OK)
	{
		ESP_LOGE("CANbus", "Failed to send message ID:%08x, Error Code: %d Message:%s\n", identifier, error,
		         logmessage.c_str());
		uint32_t alerts = 0;
		if (error == ESP_ERR_INVALID_ARG)
		{
			ESP_LOGE("CANbus", "ESP_ERR_INVALID_ARG");
		}
		else if (error == ESP_ERR_TIMEOUT)
		{
			ESP_LOGE("CANbus", "ESP_ERR_TIMEOUT");
		}
		else if (error == ESP_ERR_INVALID_STATE)
		{
			ESP_LOGE("CANbus", "ESP_ERR_INVALID_STATE");
		}
		else if (error == ESP_ERR_NOT_SUPPORTED)
		{
			ESP_LOGE("CANbus", "ESP_ERR_NOT_SUPPORTED  only read");
		}
		else
		{
			twai_read_alerts(&alerts, pdMS_TO_TICKS(100));
		}
		Serial.printf("ID:%08x  Error Code:%d  ALERTS:%d  Message:%s\n", identifier, error, alerts, logmessage.c_str());
		String myString = "Hello, World!";
	}
}

// Function to hold the different messages you send
void sendingCANbusV1_task()
{
	const uint8_t message1_data[] = {0x41, 0x11, 0x01, 0x00, 0x00, 0x00, 0x20, 0x96};
	const uint8_t message2_data[] = {0x61, 0x15, 0x01, 0x00, 0x00, 0x00, 0x20, 0xBA};

	send_CANbus_message(0x221, message1_data, 8, false);
	delay(30); // 30ms delay
	send_CANbus_message(0x221, message2_data, 8, false);
	delay(30); // 30ms delay
}

void sendingCANbusV2_task()
{
	const uint8_t message1_data[] = {0x40, 0x41, 0x05, 0x15, 0x00, 0x50, 0x71, 0x7f};
	const uint8_t message2_data[] = {0x60, 0x55, 0x55, 0x15, 0x54, 0x51, 0xd1, 0xb8};

	send_CANbus_message(0x221, message1_data, 8, false);
	delay(30); // 30ms delay
	send_CANbus_message(0x221, message2_data, 8, false);
	delay(30); // 30ms delay
}

void setup()
{
	Serial.begin(115200);
	// Configures the IO pins.
	setupPines();

	// Activate CANbus Communication
	Serial.println("Configuring CANbus...");
	setupCANbus();
	delay(300);
}

void loop()
{
	// The main call function
	if (Mode1)
	{
		sendingCANbusV1_task();
	}
	else
	{
		sendingCANbusV2_task();
	}
}
