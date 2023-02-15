#include "mbed.h"
#include "MLX90614.h"

DigitalOut myled(LED1); //displays I2C wait

//IR termometer
I2C i2c(PA_10, PA_9);   //sda,scl
MLX90614 thermometer(&i2c);
float IRtemp;
int intTemp;

int main()
{
    printf("START\r\n");
    
    while (true) {

        // if green LED1 is on, that means temperature is being
        // read from the sensor
        myled = true; 
        
        // Read object temp first
        IRtemp = thermometer.read_temp('O');
        intTemp = int(IRtemp);

        printf("IR Temperature: %d \r\n", intTemp);
        ThisThread::sleep_for(20ms);

        // read ambient temperature second
        IRtemp = thermometer.read_temp('A');
        intTemp = int(IRtemp);

        printf("IR Temperature: %d \r\n", intTemp);

        // turn off led after temperature has been read
        myled = false;
        // wait before reading again
        ThisThread::sleep_for(2s);
    }
}

