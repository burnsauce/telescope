#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <avr32/io.h>

#include "conf_tc_irq.h"

// asf
#include "compiler.h"
#include "delay.h"
#include "gpio.h"
#include "intc.h"
#include "pm.h"
#include "preprocessor.h"
#include "print_funcs.h"
#include "spi.h"
#include "sysclk.h"
#include "usb_protocol_hid.h"
#include "tc.h"
#include "intc.h"

// system
#include "adc.h"
#include "font.h"
#include "interrupts.h"
#include "kbd.h"
#include "region.h"
#include "util.h"

// this
#include "conf_board.h"
#include "display.h"
#include "scope.h"
#include "telescope.h"

static uint16_t adc[4];

static void init_spi (void) {

	sysclk_enable_pba_module(SYSCLK_SPI);

	static const gpio_map_t SPI_GPIO_MAP = {
		{SPI_SCK_PIN,  SPI_SCK_FUNCTION },
		{SPI_MISO_PIN, SPI_MISO_FUNCTION},
		{SPI_MOSI_PIN, SPI_MOSI_FUNCTION},
		{SPI_NPCS0_PIN,  SPI_NPCS0_FUNCTION },
		{SPI_NPCS1_PIN,  SPI_NPCS1_FUNCTION },
		{SPI_NPCS2_PIN,  SPI_NPCS2_FUNCTION },
	};

    // Assign GPIO to SPI.
	gpio_enable_module(SPI_GPIO_MAP, sizeof(SPI_GPIO_MAP) / sizeof(SPI_GPIO_MAP[0]));


	spi_options_t spiOptions = {
		.reg = DAC_SPI_NPCS,
		.baudrate = 2000000,
		.bits = 8,
		.trans_delay = 0,
		.spck_delay = 0,
		.stay_act = 1,
		.spi_mode = 1,
		.modfdis = 1
	};

  // Initialize as master.
	spi_initMaster(SPI, &spiOptions);
  // Set SPI selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(SPI, 0, 0, 0);
  // Enable SPI module.
	spi_enable(SPI);

  // spi_setupChipReg( SPI, &spiOptions, FPBA_HZ );
	spi_setupChipReg(SPI, &spiOptions, sysclk_get_pba_hz() );


  // add ADC chip register
	spiOptions.reg          = ADC_SPI_NPCS;
	spiOptions.baudrate     = 20000000;
	spiOptions.bits         = 16;
	spiOptions.spi_mode     = 2;
	spiOptions.spck_delay   = 0;
	spiOptions.trans_delay  = 5;
	spiOptions.stay_act     = 0;
	spiOptions.modfdis      = 0;

	spi_setupChipReg( SPI, &spiOptions, FPBA_HZ );


  // add OLED chip register
	spiOptions.reg          = OLED_SPI_NPCS;
	spiOptions.baudrate     = 40000000;
	spiOptions.bits         = 8;
	spiOptions.spi_mode     = 3;
	spiOptions.spck_delay   = 0;
	spiOptions.trans_delay  = 0;
	spiOptions.stay_act     = 1;
	spiOptions.modfdis      = 1;

	spi_setupChipReg( SPI, &spiOptions, FPBA_HZ );
}


#ifdef TWO_TIMERS
static volatile bool in_display = false;


__attribute__((__interrupt__))
static void display_callback(void) {
    in_display = true;
    scope_draw();
    tc_read_sr(APP_TC, 1);
    in_display = false;
}
#endif

static uint16_t last_knob = 0;

__attribute__((__interrupt__))
static void sample_callback(void) {
#ifdef TWO_TIMERS
    if (in_display)
        d_end();
#endif
    adc_convert(&adc);
    scope_process_sample(adc[0]);

    uint16_t knob = adc[1] / 205;

    if (knob != last_knob) {
        print_dbg("\r\nNew knob: ");

        last_knob = knob;

        if (knob < 10) {
            scope_zoom(-(int16_t)(1 << (10 - knob)));
            print_dbg_ulong((1 << (10 - knob)));
        }
        else {
            scope_zoom((int16_t)(1 << (knob - 10)));
            print_dbg_ulong((1 << (knob - 10)));
        }
        print_dbg("\r\n");
        delay_ms(50);
    }

#ifndef TWO_TIMERS
    static uint32_t count = 0;
    count = (count + 1) % DISPLAY_DIVISOR;
    if (count == 0)
        scope_draw();
#endif

    tc_read_sr(APP_TC, 0);
}


void timer_init(void);
void timer_init(void) {
    static const tc_interrupt_t tc_interrupt_1 = {
        .etrgs = 0,
        .ldrbs = 0,
        .ldras = 0,
        .cpcs  = 1,
        .cpbs  = 0,
        .cpas  = 0,
        .lovrs = 0,
        .covfs = 0
    };

    tc_waveform_opt_t waveform_opt_1 = {
        .channel = 0,
        .bswtrg  = TC_EVT_EFFECT_NOOP,
        .beevt   = TC_EVT_EFFECT_NOOP,
        .bcpc    = TC_EVT_EFFECT_NOOP,
        .bcpb    = TC_EVT_EFFECT_NOOP,
        .aswtrg  = TC_EVT_EFFECT_NOOP,
        .aeevt   = TC_EVT_EFFECT_NOOP,
        .acpc    = TC_EVT_EFFECT_NOOP,
        .acpa    = TC_EVT_EFFECT_NOOP,
        .wavsel  = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
        .enetrg  = false,
        .eevt    = 0,
        .eevtedg = TC_SEL_NO_EDGE,
        .cpcdis  = false,
        .cpcstop = false,
        .burst   = false,
        .tcclks  = TC_CLOCK_SOURCE_TC2
    };

    sysclk_enable_peripheral_clock(APP_TC);

    INTC_register_interrupt(&sample_callback, AVR32_TC_IRQ0, AVR32_INTC_INT0);
    tc_init_waveform(APP_TC, &waveform_opt_1);
    tc_write_rc(APP_TC, 0, (FCPU_HZ / SAMPLE_RATE));
    tc_configure_interrupts(&AVR32_TC, 0, &tc_interrupt_1);
    tc_start(APP_TC, 0);
#ifdef TWO_TIMERS
    static const tc_interrupt_t tc_interrupt_2 = {
        .etrgs = 0,
        .ldrbs = 0,
        .ldras = 0,
        .cpcs  = 1,
        .cpbs  = 0,
        .cpas  = 0,
        .lovrs = 0,
        .covfs = 0
    };
    tc_waveform_opt_t waveform_opt_2 = {
        .channel = 1,
        .bswtrg  = TC_EVT_EFFECT_NOOP,
        .beevt   = TC_EVT_EFFECT_NOOP,
        .bcpc    = TC_EVT_EFFECT_NOOP,
        .bcpb    = TC_EVT_EFFECT_NOOP,
        .aswtrg  = TC_EVT_EFFECT_NOOP,
        .aeevt   = TC_EVT_EFFECT_NOOP,
        .acpc    = TC_EVT_EFFECT_NOOP,
        .acpa    = TC_EVT_EFFECT_NOOP,
        .wavsel  = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
        .enetrg  = false,
        .eevt    = 0,
        .eevtedg = TC_SEL_NO_EDGE,
        .cpcdis  = false,
        .cpcstop = false,
        .burst   = false,
        .tcclks  = TC_CLOCK_SOURCE_TC2
    };
    INTC_register_interrupt(&display_callback, AVR32_TC_IRQ1, AVR32_INTC_INT1);
    tc_init_waveform(APP_TC, &waveform_opt_2);
    tc_write_rc(APP_TC, 1, (sysclk_get_pba_hz() / DISPLAY_RATE));
    tc_configure_interrupts(&AVR32_TC, 1, &tc_interrupt_2);
    tc_start(APP_TC, 1);
#endif
}
        
int main(void) {
    sysclk_init();

    init_dbg_rs232(FMCK_HZ);
    init_spi();
    d_init();
    d_clear();

    print_dbg("\r\n\r\n// telescope! /////////////////////////////// ");

    // setup daisy chain for two dacs
#if 0
    spi_selectChip(DAC_SPI, DAC_SPI_NPCS);
    spi_write(DAC_SPI, 0x80);
    spi_write(DAC_SPI, 0xff);
    spi_write(DAC_SPI, 0xff);
    spi_unselectChip(DAC_SPI, DAC_SPI_NPCS);
#endif


    scope_init();

    irq_initialize_vectors();
    Disable_global_interrupt();
    timer_init();
    Enable_global_interrupt();
    cpu_irq_enable();

    while(1) {
    }
}
