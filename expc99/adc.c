#include <avr/io.h>
#include "adc.h"


/* remembers last conversion done */
static adc_channel last_conversion;

/*
 * adc_channel internal representation:
 * - bits [0-2] hardware channel number
 * - bits [3-5] reserved
 * - bits [6-7] voltage reference
 *
 *  bit  7 6 5 4 3 2 1 0
 *       R R - - - C C C
 */

/*
 * adc_channel utility macros 
 */
#define G_CH(x) (x & 07)                 /* get channel number */
#define G_RE(x) (x>>6)                   /* get reference voltage */
#define M_CH(x) (x & 07)                 /* masked channel number */
#define M_RE(x) (x & 0300)               /* masked reference voltage */
#define C_ADC(c,r) (c|r<<6)              /* adc_channel constructor */





adc_channel adc_bind(uint8_t ch, adc_ref ref) {
  /* disable digital port if needed */
  if (ch < 6)
    DIDR0 |= _BV(ch);
  /* return external repr of adc channel */
  return C_ADC(ch,ref);
}


void adc_unbind(adc_channel *const ch) {
  /* enable digital port if needed */
  if (*ch < 6)
    DIDR0 &= ~_BV(*ch);
}


void adc_start_conversion(adc_channel ch) {
  /* test if same enviroment that last conversion */
  if (ch != last_conversion) {
    if (M_RE(ch) != M_RE(last_conversion)) {
      /* must change reference source */
      ADMUX = (ADMUX & 077) | M_RE(ch);
      /* wait if needed */
    }
    if (M_CH(ch) != M_CH(last_conversion)) {
      /* must change physical channel */
      ADMUX = (ADMUX &  0370) | M_CH(ch);
      /* wait enough */
    } 
    /* update last conversion */
    last_conversion = ch;
  }
  
  /* avoid overreads */
  while (ADCSRA & _BV(ADSC));

  /* start single convertion: write ’1′ to ADSC */
  ADCSRA |= _BV(ADSC);
}

 
bool adc_converting(void) {
  return ADCSRA & _BV(ADSC);
}


uint8_t adc_get(void) {
  return ADCH;  // only 8 higher bits
}
 

uint8_t adc_wait_get(adc_channel ch)
{
  adc_start_conversion(ch);
  while (adc_converting());
  return adc_get();
}






void adc_setup(void) {
  /* Registre DIDR0 s'ha de usar en els canals emprats */
  /* Mes estable 3.3V que els Vcc per que alimentacio no estable */
  /* potenciometre usa Vcc -> com canviar-ho per canal? */
  
  /* disable power reduction for ADC */
  PRR &= ~_BV(PRADC);
  /* ADC Enable and prescaler of 64
   * 16000000/32 = 500000 kHz. 
   * typical conversion time 13 cycles = 13*500000^-1 = 26 us
   */
  ADCSRA = _BV(ADPS2) | _BV(ADPS0) ;
  /* want only 8 bits resolution: shift reading left */
  ADMUX = _BV(ADLAR);
  /* ADC enable */
  ADCSRA |= _BV(ADEN);
  /* set module state: last adc_channel converted is none */
  last_conversion = 0x0;
  /* force and discard very first read.  This sets up
   * `last_conversion`, and waits for first unusually long reading
   * time (25 cycles, pp. 208 datasheet).  Force to select an
   * yet unused channel and reference voltage.
   */
  (void)adc_wait_get(C_ADC(1,1));
}








