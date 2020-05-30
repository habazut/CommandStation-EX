#include <Arduino.h>
#include <CommandStation.h>
#include <ArduinoTimers.h>

#define DCC_IRQ_MICROSECONDS 29

#if defined(ARDUINO_ARCH_SAMD)

Uart mainRailcomUART(&sercom2, 5, 2, SERCOM_RX_PAD_3, UART_TX_PAD_2);

void SERCOM2_Handler()
{   
    mainRailcomUART.IrqHandler();
}
#endif

////////////////////////////////////////////////////////////////
// Motor driver selection:
// Comment out all but the two lines that you want to use

DCC* mainTrack = DCC::Create_WSM_SAMCommandStation_Main(50);
DCC* progTrack = DCC::Create_WSM_SAMCommandStation_Prog(2);

// DCC* mainTrack = DCC::Create_Arduino_L298Shield_Main(50);
// DCC* progTrack = DCC::Create_Arduino_L298Shield_Prog(2);
    
// DCC* mainTrack = DCC::Create_Pololu_MC33926Shield_Main(50);
// DCC* progTrack = DCC::Create_Pololu_MC33926Shield_Prog(2);

////////////////////////////////////////////////////////////////

void main_IrqHandler() {
    mainTrack->interruptHandler();
    progTrack->interruptHandler();
}

void setup() {
    mainTrack->hdw.init();
    progTrack->hdw.init();
    
    // TimerA is TCC0 on SAMD21, Timer1 on MEGA2560, and Timer1 on MEGA328
    // We will fire an interrupt every 58us to generate the signal on the track 
    TimerA.initialize();
    TimerA.setPeriod(DCC_IRQ_MICROSECONDS);
    TimerA.attachInterrupt(main_IrqHandler);
    TimerA.start();

    mainRailcomUART.begin(250000);

#if defined (ARDUINO_ARCH_SAMD)
    CommManager::registerInterface(new USBInterface(SerialUSB));     // Register SerialUSB as an interface
    Wire.begin();       // Needed for EEPROM to work
#elif defined(ARDUINO_ARCH_AVR)
    CommManager::registerInterface(new SerialInterface(Serial));        // Register Serial (USB port on mega/uno) as an interface
#endif

    EEStore::init();

	JMRIParser::init(mainTrack, progTrack);       // Set up the string parser to accept commands from the interfaces
	CommManager::showInitInfo();                
}

void loop() {
    CommManager::update();
	mainTrack->loop();
	progTrack->loop();
}
