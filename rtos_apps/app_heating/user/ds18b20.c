/*
 * Adaptation of Paul Stoffregen's One wire library to the ESP8266 and
 * Necromant's Frankenstein firmware by Erland Lewin <erland@lewin.nu>
 * 
 * Paul's original library site:
 *   http://www.pjrc.com/teensy/td_libs_OneWire.html
 * 
 * See also http://playground.arduino.cc/Learning/OneWire
 * 
 */
#include "esp_common.h"
#include "driver/easygpio.h"
#include "driver/espmissingincludes.h"
#include "ds18b20.h"

/*
 * static function prototypes
 */
static int ds_search( uint8_t *addr );
static void reset_search();
static void write_bit( int v );
static void write( uint8_t v, int parasitePower );
static inline int read_bit(void);
static inline void write_bit( int v );
static void select(const uint8_t rom[8]);
void ds_init( int gpio );
static uint8_t reset(void);
static uint8_t read();
static uint8_t crc8(const uint8_t *addr, uint8_t len);

void convert(char* dest, const uint8_t* data, char type) {
    // float arithmetic isn't really necessary, tVal and tFract are in 1/10 °C
    uint16_t tVal, tFract;
    char tSign;

    tVal = (data[1] << 8) | data[0];
    if (tVal & 0x8000) {
	    tVal = (tVal ^ 0xffff) + 1;				// 2's complement
	    tSign = '-';
    } else
	    tSign = '+';

    // datasize differs between DS18S20 and DS18B20 - 9bit vs 12bit
    if (type == DS18S20) {
	    tFract = (tVal & 0x01) ? 50 : 0;		// 1bit Fract for DS18S20
	    tVal >>= 1;
    } else {
	    tFract = (tVal & 0x0f) * 100 / 16;		// 4bit Fract for DS18B20
	    tVal >>= 4;
    }
    sprintf(dest, "%c%d.%02d", tSign, tVal-2, tFract);
}

// global search state
static unsigned char ROM_NO[8];
static uint8_t LastDiscrepancy;
static uint8_t LastFamilyDiscrepancy;
static uint8_t LastDeviceFlag;
static int gpioPin;

void ds_init( int gpio )
{
    easygpio_pinMode(gpio, EASYGPIO_PULLUP, EASYGPIO_OUTPUT);
    easygpio_outputDisable(gpio);
    gpioPin = gpio;
}

static void reset_search()
{
	// reset the search state
	LastDiscrepancy = 0;
	LastDeviceFlag = FALSE;
	LastFamilyDiscrepancy = 0;
	for(int i = 7; ; i--) {
		ROM_NO[i] = 0;
		if ( i == 0) break;
	}
}

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
static uint8_t reset(void)
{
	//	IO_REG_TYPE mask = bitmask;
	//	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	int r;
	uint8_t retries = 125;

	// noInterrupts();
	// DIRECT_MODE_INPUT(reg, mask);
	easygpio_outputDisable( gpioPin );
	
	// interrupts();
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return 0;
		os_delay_us(2);
	} while ( !easygpio_inputGet( gpioPin ));

	// noInterrupts();
	easygpio_outputSet( gpioPin, 0 );
	// DIRECT_WRITE_LOW(reg, mask);
	// DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
	// interrupts();
	os_delay_us(480);
	// noInterrupts();
	easygpio_outputDisable( gpioPin );
	// DIRECT_MODE_INPUT(reg, mask);	// allow it to float
	os_delay_us(70);
	// r = !DIRECT_READ(reg, mask);
	r = !easygpio_inputGet( gpioPin );
	// interrupts();
	os_delay_us(410);

	return r;
}

/* pass array of 8 bytes in */
static int ds_search( uint8_t *newAddr )
{
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number;
	uint8_t id_bit, cmp_id_bit;
	int search_result;

	unsigned char rom_byte_mask, search_direction;

	// initialize for search
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;

	// if the last call was not the last one
	if (!LastDeviceFlag)
	{
		// 1-Wire reset
		if (!reset())
		{
			// reset the search
			LastDiscrepancy = 0;
			LastDeviceFlag = FALSE;
			LastFamilyDiscrepancy = 0;
			return FALSE;
		}

		// issue the search command
		write(DS1820_SEARCHROM, 0);

		// loop to do the search
		do
		{
			// read a bit and its complement
			id_bit = read_bit();
			cmp_id_bit = read_bit();
	 
			// check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1))
				break;
			else
			{
				// all devices coupled have 0 or 1
				if (id_bit != cmp_id_bit)
					search_direction = id_bit;  // bit write value for search
				else
				{
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < LastDiscrepancy)
						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					else
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == LastDiscrepancy);

					// if 0 was picked then record its position in LastZero
					if (search_direction == 0)
					{
						last_zero = id_bit_number;

						// check for Last discrepancy in family
						if (last_zero < 9)
							LastFamilyDiscrepancy = last_zero;
					}
				}

				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if (search_direction == 1)
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				else
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;

				// serial number search direction write bit
				write_bit(search_direction);

				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;

				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_byte_mask == 0)
				{
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		}
		while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

		// if the search was successful then
		if (!(id_bit_number < 65))
		{
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;

			// check for last device
			if (LastDiscrepancy == 0)
				LastDeviceFlag = TRUE;

			search_result = TRUE;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !ROM_NO[0])
	{
		LastDiscrepancy = 0;
		LastDeviceFlag = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result = FALSE;
	}
	for (int i = 0; i < 8; i++) newAddr[i] = ROM_NO[i];
	return search_result;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
static void write( uint8_t v, int power ) {
	uint8_t bitMask;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		write_bit( (bitMask & v)?1:0);
	}
	if ( !power) {
		// noInterrupts();
		easygpio_outputDisable( gpioPin );
		easygpio_outputSet( gpioPin, 0 );
		// DIRECT_MODE_INPUT(baseReg, bitmask);
		// DIRECT_WRITE_LOW(baseReg, bitmask);
		// interrupts();
	}
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline void write_bit( int v )
{
	// IO_REG_TYPE mask=bitmask;
	//	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;

	easygpio_outputSet( gpioPin, 0 );
	if( v ) {
		// noInterrupts();
		//	DIRECT_WRITE_LOW(reg, mask);
		//	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		os_delay_us(10);
		easygpio_outputSet( gpioPin, 1 );
		// DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		// interrupts();
		os_delay_us(55);
	} else {
		// noInterrupts();
		//	DIRECT_WRITE_LOW(reg, mask);
		//	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		os_delay_us(65);
		easygpio_outputSet( gpioPin, 1 );
		//	DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		//		interrupts();
		os_delay_us(5);
	}
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline int read_bit(void)
{
	//IO_REG_TYPE mask=bitmask;
	//volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	int r;
  
	// noInterrupts();
	easygpio_outputSet( gpioPin, 0 );
	// DIRECT_MODE_OUTPUT(reg, mask);
	// DIRECT_WRITE_LOW(reg, mask);
	os_delay_us(3);
	easygpio_outputDisable( gpioPin );
	// DIRECT_MODE_INPUT(reg, mask);	// let pin float, pull up will raise
	os_delay_us(10);
	// r = DIRECT_READ(reg, mask);
	r = easygpio_inputGet( gpioPin );
	// interrupts();
	os_delay_us(53);

	return r;
}

//
// Do a ROM select
//
static void select(const uint8_t *rom)
{
	uint8_t i;

	write(DS1820_MATCHROM, 0);           // Choose ROM

	for (i = 0; i < 8; i++) write(rom[i], 0);
}

//
// Read a byte
//
static uint8_t read() {
	uint8_t bitMask;
	uint8_t r = 0;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		if ( read_bit()) r |= bitMask;
	}
	return r;
}

//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
static uint8_t crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	
	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}


static os_timer_t timer_update_temp;
uint8_t device_addr[8];

void update_temp(char* result) {
    uint8_t data[12];

    reset();
    select( device_addr );
    write( DS1820_READ_SCRATCHPAD, 0 );
    for( uint8 i = 0; i < 9; i++ ) data[i] = read();

    convert(result, data, device_addr[0]);
}

static void timeout_func(void *arg)
{
    update_temp((char*)arg);
}

void ICACHE_FLASH_ATTR print_device_type() {
      switch( device_addr[0] ) {
	case DS18S20:
		printf( "Device is DS18S20 family.\n" );
		break;

	case DS18B20:
		printf( "Device is DS18B20 family.\n" );
		break;

	default:
		printf( "Device is unknown family.\n" );
		return;
      }
}

#define sleepms(x) os_delay_us(x*1000);

void ICACHE_FLASH_ATTR init_ds18b20_timer(char* result, int gpio) {
    ds_init( gpio );
    reset();
    write( DS1820_SKIP_ROM, 1 );
    write( DS1820_CONVERT_T, 1 );

    //750ms 1x, 375ms 0.5x, 188ms 0.25x, 94ms 0.12x
    volatile uint32 v = system_get_time() + 750 * 1000;
    while (v < system_get_time()) { wdt_feed(); }
    //sleepms( 150 );
    //wdt_feed();

    reset_search();
    
    int r = ds_search( device_addr );
    if( r && crc8( device_addr, 7 ) == device_addr[7] ) {
      
    print_device_type();
	
      //Disarm timer
      os_timer_disarm(&timer_update_temp);

      //Setup timer
      os_timer_setfn(&timer_update_temp, (os_timer_func_t *)timeout_func, result);

      //Arm the timer
      //&some_timer is the pointer
      //1000 is the fire time in ms
      //0 for once and 1 for repeating
      os_timer_arm(&timer_update_temp, 60000, 1);
      
      timeout_func(result);
    }
}
