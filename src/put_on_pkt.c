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
void put_on_pkt(void *buf,void *payload,int seqnum,int window,int end){
    pkt_t *newPkt = pkt_new();
    pkt_set_tr(newPkt,0);
    pkt_set_type(newPkt,PTYPE_DATA);
    if(end == 0){
        pkt_set_length(newPkt,sizeof(payload));
        pkt_set_payload(newPkt,payload,sizeof(payload));
    }else if(end==1){
        pkt_set_length(newPkt,0);
        pkt_set_payload(newPkt,NULL,0);
    }else{
        pkt_set_type(newPkt,PTYPE_NACK);
    }
    time_t sent_moment = clock();
    pkt_set_timestamp(newPkt,(uint32_t)sent_moment);
    pkt_set_seqnum(newPkt,seqnum);
    pkt_set_window(newPkt,window);
    
    
    size_t len = 512;
    pkt_encode(newPkt,buf,&len);
    pkt_del(newPkt);
}