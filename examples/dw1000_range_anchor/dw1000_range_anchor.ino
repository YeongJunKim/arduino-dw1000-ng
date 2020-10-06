#include <DW1000Ng.hpp>
#include <DW1000NgUtils.hpp>
#include <DW1000NgRanging.hpp>

// connection pins
const uint8_t PIN_RST = 7; // reset pin
const uint8_t PIN_IRQ = 8; // irq pin
const uint8_t PIN_SS = SS; // spi select pin

// messages used in the ranging protocol
// TODO replace by enum
#define POLL 0
#define POLL_ACK 1
#define RANGE 2
#define RANGE_REPORT 3
#define RANGE_FAILED 255
// message flow state
byte msgId; int i=0;
volatile byte expectedMsgId = POLL;
// message sent/received state
volatile boolean sentAck = false;
volatile boolean receivedAck = false;
// protocol error state
boolean protocolFailed = false;
// timestamps to remember
uint64_t timePollSent;
uint64_t timePollReceived;
uint64_t timePollAckSent;
uint64_t timePollAckReceived;
uint64_t timeRangeSent;
uint64_t timeRangeReceived;

uint64_t timeComputedRange;
// last computed range/time
// data buffer
#define LEN_DATA 16
byte data_tx[LEN_DATA];
byte data_rx[LEN_DATA];
// watchdog and reset period
uint32_t lastActivity;
uint32_t resetPeriod = 250;
// reply times (same on both sides for symm. ranging)
uint16_t replyDelayTimeUS = 3000;
// ranging counter (per second)
uint16_t successRangingCount = 0;
uint32_t rangingCountPeriod = 0;
float samplingRate = 0;

device_configuration_t DEFAULT_CONFIG = {
    false,
    true,
    true,
    true,
    false,
    SFDMode::STANDARD_SFD,
    Channel::CHANNEL_5,
    DataRate::RATE_850KBPS,
    PulseFrequency::FREQ_16MHZ,
    PreambleLength::LEN_256,
    PreambleCode::CODE_3
};

interrupt_configuration_t DEFAULT_INTERRUPT_CONFIG = {
    true,
    true,
    true,
    false,
    true
};

void setup() {
    // DEBUG monitoring
    Serial.begin(9600);
    delay(1000);
    Serial.println(F("### DW1000Ng-arduino-ranging-anchor ###"));
    // initialize the driver
    DW1000Ng::initialize(PIN_SS, PIN_IRQ, PIN_RST);
    Serial.println(F("DW1000Ng initialized ..."));
    // general configuration
    DW1000Ng::applyConfiguration(DEFAULT_CONFIG);
    DW1000Ng::applyInterruptConfiguration(DEFAULT_INTERRUPT_CONFIG);

    DW1000Ng::setDeviceAddress(1);
  
    DW1000Ng::setAntennaDelay(16436);
    
    Serial.println(F("Committed configuration ..."));
    // DEBUG chip info and registers pretty printed
    char msg[128];
    DW1000Ng::getPrintableDeviceIdentifier(msg);
    Serial.print("Device ID: "); Serial.println(msg);
    DW1000Ng::getPrintableExtendedUniqueIdentifier(msg);
    Serial.print("Unique ID: "); Serial.println(msg);
    DW1000Ng::getPrintableNetworkIdAndShortAddress(msg);
    Serial.print("Network ID & Device Address: "); Serial.println(msg);
    DW1000Ng::getPrintableDeviceMode(msg);
    Serial.print("Device mode: "); Serial.println(msg);
    // attach callback for (successfully) sent and received messages

    // anchor starts in receiving mode, awaiting a ranging poll message
    receiver();
    noteActivity();
    // for first time ranging frequency computation
    rangingCountPeriod = millis();
}

void noteActivity() {
    // update activity timestamp, so that we do not reach "resetPeriod"
    lastActivity = millis();
}

void resetInactive() {
    // anchor listens for POLL
    expectedMsgId = POLL;
    receiver();
    noteActivity();
}

void transmitPollAck() {
    data_tx[0] = POLL_ACK;
    DW1000Ng::setTransmitData(data_tx, LEN_DATA);
    DW1000Ng::startTransmit(TransmitMode::IMMEDIATE);
    while(!DW1000Ng::isTransmitDone()) {
     #if defined(ESP8266)
     yield();
     #endif
    }
    DW1000Ng::clearTransmitStatus();
}

void transmitRangeReport(float curRange) {
    data_tx[0] = RANGE_REPORT;
    // write final ranging result
    memcpy(data_tx + 1, &curRange, 4);
    DW1000Ng::setTransmitData(data_tx, LEN_DATA);
    DW1000Ng::startTransmit(TransmitMode::IMMEDIATE);
      while(!DW1000Ng::isTransmitDone()) {
    #if defined(ESP8266)
    yield();
    #endif
  }
  DW1000Ng::clearTransmitStatus();
}

void transmitRangeFailed() {
    data_tx[0] = RANGE_FAILED;
    DW1000Ng::setTransmitData(data_tx, LEN_DATA);
    DW1000Ng::startTransmit(TransmitMode::IMMEDIATE);
      while(!DW1000Ng::isTransmitDone()) {
    #if defined(ESP8266)
    yield();
    #endif
  }
  DW1000Ng::clearTransmitStatus();
}

void receiver() {
    //DW1000Ng::forceTRxOff();
    // so we don't need to restart the receiver manually
    DW1000Ng::startReceive();
}

void loop() {
    int32_t curMillis = millis();

    // get message and parse
    DW1000Ng::startReceive();
    DW1000Ng::getReceivedData(data_rx, LEN_DATA);
    DW1000Ng::clearReceiveStatus();
    msgId = data_rx[0];
    //Serial.println(msgId);
    if(data_tx[0]==POLL_ACK){
      Serial.print("Time Poll Ack Sent"); Serial.print(", ");
      timePollAckSent = DW1000Ng::getTransmitTimestamp();
    }

    if(msgId==POLL){
      timePollReceived = DW1000Ng::getReceiveTimestamp();
      transmitPollAck();
    }

    else if(msgId==RANGE){
      timeRangeReceived = DW1000Ng::getReceiveTimestamp();
      timePollSent = DW1000NgUtils::bytesAsValue(data_rx + 1, LENGTH_TIMESTAMP);
      timePollAckReceived = DW1000NgUtils::bytesAsValue(data_rx + 6, LENGTH_TIMESTAMP);
      timeRangeSent = DW1000NgUtils::bytesAsValue(data_rx + 11, LENGTH_TIMESTAMP);
      // (re-)compute range as two-way ranging is done
      double distance = DW1000NgRanging::computeRangeAsymmetric(timePollSent,
                                                            timePollReceived, 
                                                            timePollAckSent, 
                                                            timePollAckReceived, 
                                                            timeRangeSent, 
                                                            timeRangeReceived);
      /* Apply simple bias correction */
      distance = DW1000NgRanging::correctRange(distance);
              
      String rangeString = "Range: "; rangeString += distance; rangeString += " m";
      rangeString += "\t RX power: "; rangeString += DW1000Ng::getReceivePower(); rangeString += " dBm";
      rangeString += "\t Sampling: "; rangeString += samplingRate; rangeString += " Hz";
      Serial.println(rangeString);
      //Serial.print("FP power is [dBm]: "); Serial.print(DW1000Ng::getFirstPathPower());
      //Serial.print("RX power is [dBm]: "); Serial.println(DW1000Ng::getReceivePower());
      //Serial.print("Receive quality: "); Serial.println(DW1000Ng::getReceiveQuality());
      // update sampling rate (each second)
      transmitRangeReport(distance * DISTANCE_OF_RADIO_INV);
      successRangingCount++;
      if (curMillis - rangingCountPeriod > 1000) {
             samplingRate = (1000.0f * successRangingCount) / (curMillis - rangingCountPeriod);
             rangingCountPeriod = curMillis;
             successRangingCount = 0;
      }
    }
    else
      transmitRangeFailed();
}
