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

#include "MLX90614.h"
// don't pay attention to the error
#include <MQTTClientMbedOs.h>
/*
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
    // char *id = MBED_CONF_APP_MQTT_ID;
    // data.clientID.cstring = id;
    data.clientID.cstring = MBED_CONF_APP_MQTT_ID;

    char buffer[64];

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
    
    for (int i = 0; i < 20; i++) {

        // if green LED1 is on, that means temperature is being
        // read from the sensor
        myled = true; 
        
        // Read object temp first
        objectTemp = thermometer.read_temp('O');

        printf("IR Temperature: %d \r\n", int(objectTemp));
        ThisThread::sleep_for(20ms);

        // read ambient temperature second
        ambientTemp = thermometer.read_temp('A');

        printf("Ambient Temperature: %d \r\n", int(ambientTemp));

        // turn off led after temperature has been read
        myled = false;

        // send message through MQTT
        sprintf(buffer, "{\"object\":%d,\"ambient\":%d}", int(objectTemp), int(ambientTemp));
        // update payload length to be the length of the message
        // instead of being consistantly 64 bytes
        msg.payloadlen = strlen(buffer);    // <------ Muutettu
        client.publish(MBED_CONF_APP_MQTT_TOPIC, msg);
        printf("Published!\n\r");

        // wait before reading again
        ThisThread::sleep_for(2s);
    }
}