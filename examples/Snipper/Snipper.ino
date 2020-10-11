#include <DW1000Ng.hpp>
#include <DW1000NgUtils.hpp>
#include <DW1000NgRanging.hpp>

// connection pins
const uint8_t PIN_RST   = 7;  // reset pin
const uint8_t PIN_IRQ   = 2;  // irq pin
const uint8_t PIN_SS    = SS; // spi select pin

const uint8_t PIN_SS_TAG    = 6;
const uint8_t PIN_IRQ_TAG   = 3;
const uint8_t PIN_RST_TAG       = 5;

// messages used in the ranging protocol
// TODO replace by enum
#define POLL 0
#define POLL_ACK 1
#define POLL_ACK_LEN 5
#define RANGE 2
#define RANGE_REPORT 3
#define BROADCASTING 4

#define RANGE_FAILED 255


#define NETWORK_ID    0x000A
#define LOCAL_ADDRESS 0x1111

/*
**MESSAGE PACKET**
| bits |<-                       for calculating distance           ->|   IDENTIFIER  | sender IDENTIFIER |    local position  |
|  0   |   1 2 3 4 5   |      6 7 8 9 10      |     11 12 13 14 15    |     16 17     |        18 19      |  20 21  |   22 23  |
| TYPE |  timePollSent |  timePollAckReceived |     timeRangeSent     | LOCAL_ADDRESS |    LOCAL_ADDRESS  |  X_POS  |   Y_POS  | 

POLL MESSAGE 
| bits |<-                       for calculating distance           ->|   IDENTIFIER  | sender IDENTIFIER |    local position  |
|  0   |   1 2 3 4 5   |      6 7 8 9 10      |     11 12 13 14 15    |     16 17     |        18 19      |  20 21  |   22 23  |
| TYPE |               |                      |                       | LOCAL_ADDRESS |    LOCAL_ADDRESS  |  X_POS  |   Y_POS  | 

POLL_ACK MESSAGE 
| bits |<-                       for calculating distance           ->|   IDENTIFIER  | sender IDENTIFIER |    local position  |
|  0   |   1 2 3 4 5   |      6 7 8 9 10      |     11 12 13 14 15    |     16 17     |        18 19      |  20 21  |   22 23  |
|  1   |               |                      |                       | LOCAL_ADDRESS |    LOCAL_ADDRESS  |  X_POS  |   Y_POS  | 

RANGE MESSAGE 
| bits |<-                       for calculating distance           ->|   IDENTIFIER  | sender IDENTIFIER |    local position  |
|  0   |   1 2 3 4 5   |      6 7 8 9 10      |     11 12 13 14 15    |     16 17     |        18 19      |  20 21  |   22 23  |
|  2   |  timePollSent |  timePollAckReceived |     timeRangeSent     | LOCAL_ADDRESS |    LOCAL_ADDRESS  |  X_POS  |   Y_POS  |

RANGE_REPORT MESSAGE 
| bits |<-                       for calculating distance           ->|   IDENTIFIER  | sender IDENTIFIER |    local position  |
|  0   |   1 2 3 4 5   |      6 7 8 9 10      |     11 12 13 14 15    |     16 17     |        18 19      |  20 21  |   22 23  |
|  2   |   distance    |                      |                       |               |                   |         |          |

BROADCASTING MESSAGE 
| bits |<-                       for calculating distance           ->|   IDENTIFIER  | sender IDENTIFIER |    local position  |
|  0   |   1 2 3 4 5   |      6 7 8 9 10      |     11 12 13 14 15    |     16 17     |        18 19      |  20 21  |   22 23  |
|  2   |               |                      |                       | LOCAL_ADDRESS |    LOCAL_ADDRESS  |  X_POS  |   Y_POS  |


TYPE:
0 : POLL
1 : POLL_ACK
2 : RANGE
3 : RANGE_REPORT
4 : BROADCASTING

If LOCAL_ADDRESS == 0xFFFF -> broadcast

*/



#define LEN_DATA 24




uint32_t lastActivity;
uint32_t resetPeriod = 100;
uint16_t replyDelayTimeUS = 3000;
uint16_t successRangingCount = 0;
uint32_t rangingCountPeriod = 0;
float samplingRate = 0;

uint16_t id = 0x0000;
volatile byte expectedMsgId = POLL;
volatile boolean sentAck = false;
volatile boolean receivedAck = false;
boolean protocolFailed = false;

uint64_t timePollSent;
uint64_t timePollReceived;
uint64_t timePollAckSent;
uint64_t timePollAckReceived;
uint64_t timeRangeSent;
uint64_t timeRangeReceived;
uint64_t timeComputedRange;

byte data[LEN_DATA];





device_configuration_t DEFAULT_CONFIG = {
    false,
    true,
    true,
    true,
    false,
    SFDMode::STANDARD_SFD,
    Channel::CHANNEL_3,
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
    Serial.begin(230400);
    delay(1000);
    Serial.println(F("### DW1000Ng-arduino-ranging-anchor ###"));
    // initialize the driver
    DW1000Ng::initialize(PIN_SS, PIN_IRQ, PIN_RST);
    Serial.println(F("DW1000Ng initialized ..."));
    // general configuration
    DW1000Ng::applyConfiguration(DEFAULT_CONFIG);
    DW1000Ng::applyInterruptConfiguration(DEFAULT_INTERRUPT_CONFIG);

    DW1000Ng::setNetworkId(NETWORK_ID);
    DW1000Ng::setDeviceAddress(LOCAL_ADDRESS);
  
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
    DW1000Ng::attachSentHandler(handleSent);
    DW1000Ng::attachReceivedHandler(handleReceived);
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

void handleSent() {
    // status change on sent success
    sentAck = true;
}

void handleReceived() {
    // status change on received success
    
    receivedAck = true;
}
void receiver() {
    DW1000Ng::forceTRxOff();
    // so we don't need to restart the receiver manually
    DW1000Ng::startReceive();
}




uint32_t nowCnt = 0;
uint32_t pastCnt = 0;
void loop() {
    nowCnt = millis();
    if(nowCnt - pastCnt > 1000)
    {
        Serial.println("run");
        pastCnt = nowCnt;
    }
    
    int32_t curMillis = millis();
    if (!receivedAck) {
        // check if inactive
        if (curMillis - lastActivity > resetPeriod) {
            resetInactive();
        }
        return;
    }
    if (receivedAck) {
        receivedAck = false;
        // get message and parse
        DW1000Ng::getReceivedData(data, LEN_DATA);
        
        byte msgId = data[0];
//#define POLL 0
//#define POLL_ACK 1
//#define POLL_ACK_LEN 5
//#define RANGE 2
//#define RANGE_REPORT 3
//#define BROADCASTING 4
        String sid = ""; String sid2 = "";
        String sid0 = String(data[17], HEX);
        String sid1 = String(data[16], HEX);
        sid += sid0; sid += sid1;
        String sid2_2 = String(data[18], HEX);
        String sid2_3 = String(data[19], HEX);
        sid2 += sid2_2; sid2 += sid2_3;

        Serial.print("[snipping] :");
        switch(data[0])
        {
          case POLL:
          Serial.print(", TYPE = "); Serial.print("POLL");
          break;
          case POLL_ACK:
          Serial.print(", TYPE = "); Serial.print("POLL_ACK");
          break;
          case POLL_ACK_LEN:
          Serial.print(", TYPE = "); Serial.print("POLL_ACK_LEN");
          break;
          case RANGE:
          Serial.print(", TYPE = "); Serial.print("RANGE");
          break;
          case RANGE_REPORT:
          Serial.print(", TYPE = "); Serial.print("RANGE_REP");
          break;
          case BROADCASTING:
          Serial.print(", TYPE = "); Serial.print("BROADCASTING");
          break;
        }
        Serial.print(",\t START ID = "); Serial.print(sid);
        Serial.print(",\t END ID = "); Serial.print(sid2);
        Serial.println("");
        
        noteActivity();
    }
}
