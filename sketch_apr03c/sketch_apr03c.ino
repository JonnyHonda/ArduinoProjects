// wiki file http:// ?
// Receive / decode FS20, (K)S300, and EM10* signals using a RFM12B as an 868 MHz OOK receiver 
// 2009-04-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// The basic idea is to measure pulse widths between 0/1 and 1/0 transitions,
// and to keep track of pulse width sequences in a state machine.
// receiver signal is connected to AIO1
// 2010-15 okt JGJ  
// changed RFM12 init OOK-mode
// Tested with Arduino 0017

#define RXDATA 14

// ***** SPI + RFM12 Registers ***************************************************************************
#define DDR_SPI     DDRB
#define PORT_SPI    PORTB
#define DD_SS       2
#define DD_MOSI     3
#define DD_MISO     4
#define DD_SCK      5

#define BAND        1 // 1 = 433, 2 = 868, 3 = 915 MHz
#define GROUP       212
#define NODEID      24

#define RF_STATUS       0x0000
#define RF_XMIT_REG     0xB800
#define RF_READY        0x8000
#define RF_XMITTER_ON   0x8239
#define RF_IDLE_MODE    0x8209
static uint16_t rf12_crc;

static void rf12_enable (void);
static void rf12_disable (void); 
static void spi_init (void); 
static uint8_t rf12_byte (uint8_t out);
static uint16_t rf12_xfer (uint16_t cmd);
static void rf12_init (void);
static void rf12_send (uint8_t out);
// ***********************************************************************************************

enum { UNKNOWN, T0, T1, OK, DONE };

// hardware connections input output

#define RXDATA 14     // FSK/DATA/nFSS RFM12 signal is connected to analog input   AI0 PC0 Port1  jeenode connector
                      // ATMEGA 328 = pin 23

// track bit-by-bit reception using multiple independent state machines
struct { char bits; byte state; uint32_t prev; uint64_t data; } FS20;
struct { char bits; byte state, pos, data[16]; } S300;
struct { char bits; byte state, pos, seq; word data[10]; } EM10;

static byte FS20_bit (char value) {
    FS20.data = (FS20.data << 1) | value;
    if (FS20.bits < 0 && (byte) FS20.data != 0x01)
        return OK;
    if (++FS20.bits == 45 && ((FS20.data >> 15) & 1) == 0 ||
          FS20.bits == 54 && ((FS20.data >> 24) & 1))
        return DONE;
    return OK;
}

static byte S300_bit (char value) {
    byte *ptr = S300.data + S300.pos;
    *ptr = (*ptr >> 1) | (value << 4);
    
    if (S300.bits < 0 && *ptr != 0x10)
        return OK; // not yet synchronized

    if (++S300.bits == 4) {
        if (S300.pos >= 9 && S300.data[0] == 0x11 || S300.pos >= 15) {
            *ptr >>= 1;
            return DONE;
        }
    } else if (S300.bits >= 5) {
        ++S300.pos;
        S300.bits = 0;
    }

    return OK;
}

static byte EM10_bit (char value) {
    word *ptr = EM10.data + EM10.pos;
    *ptr = (*ptr >> 1) | (value << 8);
    
    if (EM10.bits < 0 && *ptr != 0x100)
        return OK; // not yet synchronized

    if (++EM10.bits == 8) {
        if (EM10.pos >= 9) {
            *ptr >>= 1;
            return DONE;
        }
    } else if (EM10.bits >= 9) {
        ++EM10.pos;
        EM10.bits = 0;
    }

    return OK;
}

static void ook868interrupt () {
    // count is the pulse length in units of 4 usecs
    byte count = TCNT2;
    TCNT2 = 0;

    // FS20 pulse widths are 400 and 600 usec (split on 300, 500, and 700)
    // see http://fhz4linux.info/tiki-index.php?page=FS20%20Protocol
    if (FS20.state != DONE)
        switch ((count - 25) / 50) {
            case 1:  FS20.state = FS20.state == T0 ? FS20_bit(0) : T0; break;
            case 2:  FS20.state = FS20.state == T1 ? FS20_bit(1) : T1; break;
            default: FS20.state = UNKNOWN;
        }

    // (K)S300 pulse widths are 400 and 800 usec (split on 200, 600, and 1000)
    // see http://www.dc3yc.homepage.t-online.de/protocol.htm
    if (S300.state != DONE)
        switch ((count + 50) / 100) {
            case 1:  S300.state = S300.state == T1 ? S300_bit(0) : T0; break;
            case 2:  S300.state = S300.state == T0 ? S300_bit(1) : T1; break;
            default: S300.state = UNKNOWN;
        }

    // EM10 pulse widths are 400 and 800 usec (split on 200, 600, and 1000)
    // see http://fhz4linux.info/tiki-index.php?page=EM+Protocol
    if (EM10.state != DONE) {
        switch ((count + 50) / 100) {
            case 1:  EM10.state = EM10.state == T0 ? EM10_bit(0) : T0; break;
            case 2:  if (EM10.state == T0) {
                        EM10.state = EM10_bit(1);
                        break;
                     } // else fall through
            default: EM10.state = UNKNOWN;
        }
    }
}

static void reset_FS20 () {
    FS20.bits = -1;
    FS20.data = 0xFF;
    FS20.state = UNKNOWN;
}

static void reset_S300 () {
    S300.bits = -1;
    S300.pos = 0;
    S300.data[0] = 0x1F;
    S300.state = UNKNOWN;
}

static void reset_EM10 () {
    EM10.bits = -1;
    EM10.pos = 0;
    EM10.data[0] = 0x1FF;
    EM10.state = UNKNOWN;
}

static void ook868timeout () {
    if (FS20.state != DONE) 
        reset_FS20();
    if (S300.state != DONE) 
        reset_S300();
    if (EM10.state != DONE) 
        reset_EM10();
}

static void ook868setup () {
    pinMode(RXDATA, INPUT);
    digitalWrite(RXDATA, 1); // pull-up

    /* enable analog comparator with fixed voltage reference */
    ACSR = _BV(ACBG) | _BV(ACI) | _BV(ACIE);
    ADCSRA &= ~ _BV(ADEN);
    ADCSRB |= _BV(ACME);
    ADMUX = 0; // ADC0

    /* prescaler 64 -> 250 KHz = 4 usec/count, max 1.024 msec (16 MHz clock) */
    TCNT2 = 0;
    TCCR2A = 0;
    TCCR2B = _BV(CS22);
    TIMSK2 = _BV(TOIE2);
    
    reset_FS20();
    reset_S300();
    reset_EM10();
}

// Poll and return a count > 0 when a valid packet has been received:
//  4 = S300
//  5 = FS20
//  6 = FS20 extended
//  7 = KS300
//  9 = EM10*

static byte ook868poll (byte* buf) {
    if (FS20.state == DONE) {  
        uint32_t since = millis() - FS20.prev;
        if (since > 150) {          
            byte len = FS20.bits / 9;
            byte sum = 6;
            for (byte i = 0; i < len; ++i) {
                byte b = FS20.data >> (1 + 9 * i);
                buf[len-i-1] = b;
                if (i > 0)
                    sum += b;
            }
            if (sum == buf[len-1]) {
                FS20.prev = millis();
                reset_FS20();
                return len;
            }
        }
        
        reset_FS20();
    }
  
    if (S300.state == DONE) {    
        byte n = S300.pos, ones = 0, sum = 37 - S300.data[n], chk = 0;
        for (byte i = 0; i < n; ++i) {
            byte b = S300.data[i];
            ones += b >> 4;
            sum += b;
            chk ^= b;
            
            if (i & 1)
                buf[i>>1] |= b << 4;
            else
                buf[i>>1] = b & 0x0F;
        }
        
        reset_S300();
        
        if (ones == n && (chk & 0x0F) == 0 && (sum & 0x0F) == 0)
            return n >> 1;
    }
  
    if (EM10.state == DONE) {    
        if ((byte) EM10.data[2] != EM10.seq) {
            byte ones = 0, chk = 0;
            for (byte i = 0; i < 10; ++i) {
                word v = EM10.data[i];
                ones += v >> 8;
                chk ^= v;
                buf[i] = v;
            }
            
            if (ones == 9 && chk == 0) {
                EM10.seq = EM10.data[2];
                reset_EM10();
                return 9;
            }
        }
        
        reset_EM10();
    }
    
    return 0;
}

ISR(ANALOG_COMP_vect) {
    ook868interrupt();
}

ISR(TIMER2_OVF_vect) {
    ook868timeout();
}


// ******** SPI + RFM 12B functies   ************************************************************************

static void rf12_enable () {
    PORT_SPI &= ~ _BV(DD_SS);
}

static void rf12_disable () {
    PORT_SPI |= _BV(DD_SS);
}

static void spi_init () {
    rf12_disable();
    DDR_SPI |= _BV(DD_MOSI) | _BV(DD_SCK) | _BV(DD_SS);
    DDR_SPI &= ~ _BV(DD_MISO);
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR0);
    SPSR |= _BV(SPI2X);
}

static uint8_t rf12_byte (uint8_t out) {
    SPDR = out;
    loop_until_bit_is_set(SPSR, SPIF);
    return SPDR;
}

static uint16_t rf12_xfer (uint16_t cmd) {
    rf12_enable();
    uint8_t r1 = rf12_byte(cmd >> 8);
    uint8_t r2 = rf12_byte(cmd);
    rf12_disable();
    return (r1 << 8) | r2;
}

static void rf12_init_OOK () 
{
rf12_xfer(0x8027); // 8027    868 Mhz;disabel tx register; disabel RX fifo buffer; xtal cap 12pf, same as transmitter
rf12_xfer(0x82c0); // 82c0    enable receiver ; enable basebandblock 
rf12_xfer(0xA68a); // A68a    868.2500 MHz
rf12_xfer(0xc691); // c691    c691 datarate 2395 kbps 0xc647 = 4.8kbps 
rf12_xfer(0x9489); // 9489    VDI; FAST;200khz;GAIn -6db ;DRSSI 97dbm 
rf12_xfer(0xC220); // c220    datafiltercommand ; ** not documented command 
rf12_xfer(0xCA00); // ca00    FiFo and resetmode command ; FIFO fill disabeld
rf12_xfer(0xC473); // c473    AFC run only once ; enable AFC ;enable frequency offset register ; +3 -4
rf12_xfer(0xCC67); // cc67    pll settings command
rf12_xfer(0xB800); // TX register write command not used
rf12_xfer(0xC800); // disable low dutycycle 
rf12_xfer(0xC040); // 1.66MHz,2.2V not used see 82c0  
}




// ********* End SPI + RFM 12B functies *******************************************************

void setup () {
    Serial.begin(57600);
    Serial.println("\nSerial port is  ready ");
    spi_init();Serial.println("spi_init"); 
    rf12_init_OOK();Serial.println("rf12_init_OOK_mode"); 
    Serial.println("\n[recv868ook]");

    ook868setup();
}

void loop () {
    byte buf[10];
    byte n = ook868poll(buf);
    
    if (n) {
        Serial.print("RFM12B_receiving_OOK868 ");
        Serial.print(n, DEC);
        for (byte i = 0; i < n; ++i) {
            Serial.print(' ');
            Serial.print(buf[i], DEC);
        }
        Serial.println();
    }
}
