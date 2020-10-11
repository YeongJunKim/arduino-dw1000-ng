#include <DW1000Ng.hpp>
#include <DW1000NgUtils.hpp>
#include <DW1000NgRanging.hpp>
#include <queue.hpp>

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
#define LOCAL_ADDRESS 0xCCCC

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





byte data[LEN_DATA];

Queue sendQueue;


uint32_t lastActivity;
uint32_t resetPeriod = 100;
uint16_t replyDelayTimeUS = 3000;
uint16_t successRangingCount = 0;
uint32_t rangingCountPeriod = 0;
float samplingRate = 0;

typedef struct _dev
{
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

    uint32_t lastActivity;
    uint32_t resetPeriod = 100;

    uint16_t replyDelayTimeUS = 3000;

    uint16_t successRangingCount = 0;
    uint32_t rangingCountPeriod = 0;
    float samplingRate = 0;
}t_dev;




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

t_dev t_d[2]; 

void setup() {
    
    t_d[0].id = 0xCCCA;
    t_d[1].id = 0xCCCB;
    // DEBUG monitoring
    Serial.begin(115200);
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
    if (!t_d[0].sentAck && !t_d[0].receivedAck) {
        // check if inactive
        if (curMillis - lastActivity > resetPeriod) {
            resetInactive();
        }
        return;
    }
    // continue on any success confirmation
    if (t_d[0].sentAck) {
        t_d[0].sentAck = false;
        byte msgId = data[0];
        if (msgId == POLL_ACK) {
            if (data[16] = 0xCA)
            {
                t_d[0].timePollAckSent = DW1000Ng::getTransmitTimestamp();
            }
            else if (data[16] = 0xCB)
            {
                t_d[1].timePollAckSent = DW1000Ng::getTransmitTimestamp();
            }
            noteActivity();
        }
        DW1000Ng::startReceive();
    }
    if (t_d[0].receivedAck) {
        t_d[0].receivedAck = false;
        // get message and parse
        DW1000Ng::getReceivedData(data, LEN_DATA);
        
        byte msgId = data[0];
        if (msgId != t_d[0].expectedMsgId) {
            // unexpected message, start over again (except if already POLL)
            t_d[0].protocolFailed = true;
            Serial.println("protocolFaild");
        }
        if (msgId == POLL) {
            // on POLL we (re-)start, so no protocol failure
            t_d[0].protocolFailed = false;
            t_d[0].timePollReceived = DW1000Ng::getReceiveTimestamp();
            t_d[0].expectedMsgId = RANGE;
            transmitPollAck();
            noteActivity();
        }
        else if (msgId == RANGE) {
            t_d[0].timeRangeReceived = DW1000Ng::getReceiveTimestamp();
            t_d[0].expectedMsgId = POLL;
            if (!t_d[0].protocolFailed) {
                t_d[0].timePollSent = DW1000NgUtils::bytesAsValue(data + 1, LENGTH_TIMESTAMP);
                t_d[0].timePollAckReceived = DW1000NgUtils::bytesAsValue(data + 6, LENGTH_TIMESTAMP);
                t_d[0].timeRangeSent = DW1000NgUtils::bytesAsValue(data + 11, LENGTH_TIMESTAMP);
                // (re-)compute range as two-way ranging is done
                double distance = DW1000NgRanging::computeRangeAsymmetric(t_d[0].timePollSent,
                                                            t_d[0].timePollReceived, 
                                                            t_d[0].timePollAckSent, 
                                                            t_d[0].timePollAckReceived, 
                                                            t_d[0].timeRangeSent, 
                                                            t_d[0].timeRangeReceived);
                /* Apply simple bias correction */
                distance = DW1000NgRanging::correctRange(distance);
                if(distance < 0.000001)
                {
                  distance = 0;
                }
                byte id[2] = {0,};
                id[0] = data[16];
                id[1] = data[17];    
                String sid = "";
                String sid0 = String(id[1], HEX);
                String sid1 = String(id[0], HEX);
                sid += sid0; sid += sid1;
                
                String rangeString = "Range: "; rangeString += distance; rangeString += " m";
                // rangeString += "\t RX power: "; rangeString += DW1000Ng::getReceivePower(); rangeString += " dBm";
            //    rangeString += "\t Sampling: "; rangeString += samplingRate; rangeString += " Hz";
                // rangeString += samplingRate;
               rangeString += "\t ID: "; rangeString += sid;
                Serial.println(rangeString);
                // Serial.println(distance);
                //Serial.print("FP power is [dBm]: "); Serial.print(DW1000Ng::getFirstPathPower());
                //Serial.print("RX power is [dBm]: "); Serial.println(DW1000Ng::getReceivePower());
                //Serial.print("Receive quality: "); Serial.println(DW1000Ng::getReceiveQuality());
                // update sampling rate (each second)
                transmitRangeReport(distance * DISTANCE_OF_RADIO_INV);
                t_d[0].successRangingCount++;
                if (curMillis - rangingCountPeriod > 1000) {
                    samplingRate = (1000.0f * successRangingCount) / (curMillis - rangingCountPeriod);
                    t_d[0].rangingCountPeriod = curMillis;
                    successRangingCount = 0;
                }
            }
            else {
                transmitRangeFailed();
                Serial.println("transmitRangeFailed");
            }

            noteActivity();
        }
    }
}

void noteActivity() {
    // update activity timestamp, so that we do not reach "resetPeriod"
    lastActivity = millis();
}

void resetInactive() {
    // anchor listens for POLL
    t_d[0].expectedMsgId = POLL;
    receiver();
    noteActivity();
}

void handleSent() {
    // status change on sent success
    // Serial.println("handleSent");
    t_d[0].sentAck = true;
}

void handleReceived() {
    // status change on received success
    
    t_d[0].receivedAck = true;
    // Serial.println("handleReceived");
}

void transmitPollAck() {
    data[0] = POLL_ACK;
    data[16] = LOCAL_ADDRESS & 0xFF;
    data[17] = (LOCAL_ADDRESS  >> 8)& 0xFF;
    DW1000Ng::setTransmitData(data, LEN_DATA);
    DW1000Ng::startTransmit();
    // Serial.println("trpollack");
}

void transmitRangeReport(float curRange) {
    data[0] = RANGE_REPORT;
    // write final ranging result
    memcpy(data + 1, &curRange, 4);
    data[16] = LOCAL_ADDRESS & 0xFF;
    data[17] = (LOCAL_ADDRESS  >> 8)& 0xFF;
    DW1000Ng::setTransmitData(data, LEN_DATA);
    DW1000Ng::startTransmit();
    // Serial.println("trrngreport");
}

void transmitRangeFailed() {
    data[0] = RANGE_FAILED;
    data[16] = LOCAL_ADDRESS & 0xFF;
    data[17] = (LOCAL_ADDRESS  >> 8)& 0xFF;
    DW1000Ng::setTransmitData(data, LEN_DATA);
    DW1000Ng::startTransmit();
}

void receiver() {
    DW1000Ng::forceTRxOff();
    // so we don't need to restart the receiver manually
    DW1000Ng::startReceive();
}

void send_frame() {
    
}
