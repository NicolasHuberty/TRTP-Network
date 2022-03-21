#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include "log.h"
#include <sys/types.h>
#include <sys/socket.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "packet_interface.h"
void put_on_pkt(void *buf,void *payload,int seqnum){
    pkt_t *newPkt = pkt_new();
    pkt_set_tr(newPkt,0);
    pkt_set_type(newPkt,PTYPE_DATA);
    pkt_set_window(newPkt,1);
    pkt_set_timestamp(newPkt,385875968);
    pkt_set_seqnum(newPkt,seqnum);
    pkt_set_payload(newPkt,payload,sizeof(payload));
    pkt_set_length(newPkt,sizeof(payload));
    size_t len = 12;
    pkt_encode(newPkt,buf,&len);
    pkt_del(newPkt);
}