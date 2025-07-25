#include <ucx.h>
#include <device.h>
#include <gpio.h>
#include <stdbool.h>
#include <usart.h>
#include <gpio_ll.h> 	

// A2 = TX 
// A3 = RX
// A4 = SET
// A5 = LED Externo de 5v(resistente) + resistor de 220 Ohms
//      LED Externo: Corrente limitada em 9,5 mA *brilho baixo, mas OK --> seguro para a placa

long BAUDS[4] = {9600, 38400, 115200,57600};
short i = 0;
struct sem_s *mutex;
const char *mensagem = "ola\n";
const struct gpio_config_s gpio_config = {
    .config_values.port   =   GPIO_PORTA,
    .config_values.pinsel =   GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5,
    .config_values.mode   =   GPIO_OUTPUT << GPIO_PIN2_OPT |
                              GPIO_INPUT << GPIO_PIN3_OPT |
                              GPIO_OUTPUT << GPIO_PIN4_OPT |
                              GPIO_OUTPUT << GPIO_PIN5_OPT,
    .config_values.pull = GPIO_NOPULL << GPIO_PIN2_OPT |
                          GPIO_PULLUP << GPIO_PIN3_OPT |
                          GPIO_NOPULL << GPIO_PIN4_OPT |
                          GPIO_NOPULL << GPIO_PIN5_OPT
};

/* device driver instantiation */
const struct device_s gpio_device = {
	.name = "gpio_device",
	.config = &gpio_config,
	.custom_api = &gpio_api
};


const struct gpio_config_s led_config = {
    .config_values.port   =   GPIO_PORTC,
    .config_values.pinsel =   GPIO_PIN13,
    .config_values.mode   =   GPIO_OUTPUT << GPIO_PIN13_OPT
};

const struct device_s led_device = {
	.name = "led_device",
	.config = &led_config,
	.custom_api = &gpio_api
};

void usart_write_debug(int port, char *str){
  while (*str)
    usart_tx(port, *str++);
  _delay_ms(100);
}
void read_toFind_barN(int port_rx){
  while(usart_rxsize(port_rx)){
    printf("%c", usart_rx(port_rx));
  }
}


const struct device_s *gpio = &gpio_device;
const struct gpio_api_s *gpio_dev_api = (const struct gpio_api_s *)(&gpio_device)->custom_api;
const struct device_s *led = &led_device;
const struct gpio_api_s *led_dev_api = (const struct gpio_api_s *)(&led_device)->custom_api;


void task_idle(void){for (;;);}


//Inicio das funções da aplicação
void CONFIG(void){
  ucx_sem_wait(mutex);
  printf("Entrando em modo AT\n");
  gpio_dev_api->gpio_clear(gpio, GPIO_PIN5);//SET o pino SET do HC12;
  usart_write_debug(2, "AT\n"); //inicialização
  read_toFind_barN(2);
  usart_write_debug(2, "AT+B9600\n"); // baudrate = 9600
  read_toFind_barN(2);
  usart_write_debug(2, "AT+C001\n"); // channel 1
  read_toFind_barN(2);
  usart_write_debug(2, "AT+FU1\n"); // FU4=long distance mode, FU1= ultra low power ,FU2 = power saving mode ,FU3 = default;
  read_toFind_barN(2);
  usart_write_debug(2, "AT+P5\n"); 
  read_toFind_barN(2);
  gpio_dev_api->gpio_set(gpio, GPIO_PIN5);//RESET o pino SET do HC12;
  led_dev_api->gpio_setup(led);
  ucx_sem_signal(mutex);
}

void TRANSMIT(void){
  ucx_sem_wait(mutex);
  while(1){
        led_dev_api->gpio_toggle(led, GPIO_PIN13);//inverte o estado logico, liga ou desliga, depende do estado anterior....
        usart_write_debug(2, mensagem); // Envia a 1 caracter, RX possui uma tratativa para detecta-lo;
        _delay_ms(250);
  }
  ucx_sem_signal(mutex);
}

int32_t app_main(void)
{
        usart_init(2, 9600, 0); //porta a2, velocidade 9600, pooled ==0    INICIAMOS A PORTA
	ucx_task_spawn(CONFIG, DEFAULT_STACK_SIZE);
	ucx_task_spawn(TRANSMIT, DEFAULT_STACK_SIZE);
	
	mutex = ucx_sem_create(2, 1);
	
	return 1;
}
