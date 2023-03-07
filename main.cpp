/*
    Code and idea by Jarkko Niemi and Saku Karttunen
    for the HAMK Riihimäki's IoT Project course in February 2023
    INTIPA216 - Information and Communications Technologies Engineering
    ---
    The libraries that were used as basis will be linked
    in the README.md file as well as in the code below. The MLX90614 library
    ! DO NOT WORK ! without small changes we made to them during development
    so remember to modify them to work before running the program
    ---
    Links to the repositories used in the project:
    Micro controller files: https://github.com/sakuexe/iot-project-2023-microcontroller
    Frontend UI and documentation: https://github.com/sakuexe/iot-project-2023
    For the user interface visit http://iot.jarkon.fi/
*/

#include "mbed.h"
#include "ESP8266Interface.h"

#include <stdio.h>

/* 
    in the MLX90614, we changed the library's thermometer.read_temp() -function
    into using a single char datatype variable as a parameter, instead of an int
    this was done so that we could use 'A' for ambient and 'O' for object temp
    ! So if the program doesn't work, check your library files or change the lines
    of the program to use 0 for ambient temp and 1 for object temperature
*/
#include "MLX90614.h"
// don't pay attention to the error
#include <MQTTClientMbedOs.h>
/*
    Library credits
    ---
    MLX90614 Library is based on Jens Strümper's library (minor changes)
    https://os.mbed.com/users/jensstruemper/code/MLX90614/
    MBED-MQTT library is the ARMmbed's own library (no changes)
    https://github.com/ARMmbed/mbed-mqtt
*/  

int main()
{
    // MQTT Settings

    ESP8266Interface esp(MBED_CONF_APP_ESP_TX_PIN, MBED_CONF_APP_ESP_RX_PIN);
    
    //Store device IP
    SocketAddress deviceIP;
    //Store broker IP
    SocketAddress MQTTBroker;
    
    TCPSocket socket;
    MQTTClient client(&socket);
    
    printf("\nConnecting wifi..\n");

    int ret = esp.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);

    if(ret != 0)
    {
        printf("\nConnection error\n");
        printf("\nShutting down...\n");
        return 0;
    }
    else
    {
        printf("\nConnection success\n");
    }

    esp.get_ip_address(&deviceIP);
    printf("IP via DHCP: %s\n", deviceIP.get_ip_address());
    
    // Use with DNS
    esp.gethostbyname(MBED_CONF_APP_MQTT_BROKER_HOSTNAME, &MQTTBroker, NSAPI_IPv4, "esp");
    MQTTBroker.set_port(MBED_CONF_APP_MQTT_BROKER_PORT);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
    data.MQTTVersion = 3;
    data.clientID.cstring = MBED_CONF_APP_MQTT_ID;

    // initialize the buffer to be 64 bytes
    // the length will have to get resized after assigning payload
    char buffer[64];

    // initialize MQTT message
    MQTT::Message msg;
    msg.qos = MQTT::QOS0;
    msg.retained = false;
    msg.dup = false;
    msg.payload = (void*)buffer;

    socket.open(&esp);
    socket.connect(MQTTBroker);
    client.connect(data);

    // for displaying when the temperature is being read
    DigitalOut myled(LED1);

    //IR termometer
    I2C i2c(PA_10, PA_9);   //sda,scl
    MLX90614 thermometer(&i2c);

    // variables for storing the temperature values
    float objectTemp;
    float ambientTemp;
    
    printf("START\r\n");
    
    // choose for how long one cycle of the program runs for
    // there will be a delay of 2 seconds between each iteration
    // we use a 5 minute cycle, this was counted with the function of
    // (60 * desired_minutes) / 2
    for (int i = 0; i < 150; i++) {

        // if green LED1 is on, that means temperature is being
        // read from the sensor
        myled = true; 
        
        // Read object temp first
        objectTemp = thermometer.read_temp('O');

        // for making sure that the floating point printing works,
        // the following line was added to the mbed_app.json
        // "target.printf_lib": "std"
        // this disables Mbed OS 6.0's 'minimal printf and snprintf'
        printf("IR Temperature: %.2f \r\n", objectTemp);
        // the datasheet for MLx90614 recommends 20 ms delay between calculations
        // Page 17/57 - 8.3.3.1. ERPROMwritesequence
        ThisThread::sleep_for(20ms);

        // read ambient temperature second
        ambientTemp = thermometer.read_temp('A');

        printf("IR Temperature: %.2f \r\n", ambientTemp);

        // turn off led after temperature has been read
        myled = false;

        // send message through MQTT
        sprintf(buffer, "{\"object\":%.2f,\"ambient\":%.2f}", objectTemp, ambientTemp);
        // update payload length to be the length of the message
        // instead of being consistantly 64 bytes
        msg.payloadlen = strlen(buffer);
        client.publish(MBED_CONF_APP_MQTT_TOPIC, msg);
        // print that the topic was published
        printf("Published on topic: %s\n\r", MBED_CONF_APP_MQTT_TOPIC);

        // wait 2 seconds before reading again
        ThisThread::sleep_for(2s);
    }
    printf("Loop is done, now stopping.\n\r");
    return 0;
}