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
#include <time.h>
pkt_t *put_on_pkt(void *buf,void *payload,int seqnum,int window,int length,int type,size_t *len,uint32_t timestamp){
    pkt_t *pkt = pkt_new();
    pkt_set_tr(pkt,0);
    pkt_set_window(pkt, window);
    pkt_set_length(pkt, length);
    pkt_set_timestamp(pkt,timestamp);
    if(type == 0){
        pkt_set_type(pkt, PTYPE_DATA);
        if(length > 0){
            pkt_set_payload(pkt, payload, length);
        }
    }
    if(type == 1){
        pkt_set_type(pkt, PTYPE_ACK);
    }
    if(type == 2){
        pkt_set_type(pkt,PTYPE_NACK);
    }
    pkt_set_seqnum(pkt, seqnum);
    pkt_encode(pkt,buf,len);
    return pkt;
}