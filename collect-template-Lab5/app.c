#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "leds.h"
#include "net/netstack.h"
#include <stdio.h>
#include "core/net/linkaddr.h"
#include "my_collect.h"
/*---------------------------------------------------------------------------*/
#define MSG_PERIOD (30 * CLOCK_SECOND) /* Send a packet every 30 seconds */
#define COLLECT_CHANNEL 0xAA
#define DATA_FORWARDING 0              /* Set to 1 to enable data forwarding (Lab 6) */ 
/*---------------------------------------------------------------------------*/
#ifndef CONTIKI_TARGET_SKY
linkaddr_t sink = {{0xF7, 0x9C}}; /* Firefly (testbed): node 1 will be our sink */
#else
linkaddr_t sink = {{0x01, 0x00}}; /* TMote Sky (Cooja): node 1 will be our sink */
#endif
/*---------------------------------------------------------------------------*/
PROCESS(app_process, "App process");
AUTOSTART_PROCESSES(&app_process);
/*---------------------------------------------------------------------------*/
/* Application packet */
typedef struct {
  uint16_t seqn;
}
__attribute__((packed))
test_msg_t;
/*---------------------------------------------------------------------------*/
static struct my_collect_conn my_collect; // Initialize our Rime collection primitive
static void recv_cb(const linkaddr_t *originator, uint8_t hops);
struct my_collect_callbacks cb = {.recv = recv_cb};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data) 
{
  static struct etimer periodic;
  static struct etimer rnd;
  static int    snd_outcome; // Value returned by my_collect_send
  static test_msg_t msg = {.seqn=0};

  PROCESS_BEGIN();

  if (linkaddr_cmp(&sink, &linkaddr_node_addr)) {
    /* Sink node: open my_collect connection and do nothing,
     * simply listen for data collection packets */
    printf("App: I am the sink %02x:%02x\n",
      linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
    my_collect_open(&my_collect, COLLECT_CHANNEL, true, &cb);
  }
  else { /* Non-sink node */
    printf("App: I am a normal node %02x:%02x\n",
      linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
    my_collect_open(&my_collect, COLLECT_CHANNEL, false, NULL);
#if DATA_FORWARDING
    etimer_set(&periodic, MSG_PERIOD);
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic));
      /* Fixed interval */
      etimer_reset(&periodic);
      /* Random shift within the first half of the interval */
      etimer_set(&rnd, random_rand() % (MSG_PERIOD / 2));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&rnd));

      /* Send a data collection packet to the sink */
      packetbuf_clear();
      memcpy(packetbuf_dataptr(), &msg, sizeof(msg));
      packetbuf_set_datalen(sizeof(msg));
      printf("App: Send seqn %d\n", msg.seqn);
      snd_outcome = my_collect_send(&my_collect);
      
      /* Interpret my_collect_send output and print some 
       * debug messages */
      switch (snd_outcome) {
        case 0:
        printf("App: ERROR, unicast send outcome = 0!\n");
        break;
        case -1:
        printf("App: ERROR, node %02x:%02x is currently disconnected!\n",
            linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
        break;
        case -2:
        printf("App: ERROR, packetbuf_hdralloc() was unable to allocate "
            "sufficient header space!\n");
        break;
        default:
        printf("App: Message seqn %d sent correctly!\n", msg.seqn);
      }

      /* Update the app payload before the next transmission */
      msg.seqn++;
    }
#endif
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
recv_cb(const linkaddr_t *originator, uint8_t hops) {
  test_msg_t msg;

  /* Check if the received message looks legitimate */
  if (packetbuf_datalen() != sizeof(msg)) {
    printf("App: wrong length: %d\n", packetbuf_datalen());
    return;
  }
  memcpy(&msg, packetbuf_dataptr(), sizeof(msg));
  printf("App: Recv from %02x:%02x seqn %d hops %d\n",
    originator->u8[0], originator->u8[1], msg.seqn, hops);
}
/*---------------------------------------------------------------------------*/
