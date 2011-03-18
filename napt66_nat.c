#include "napt66_nat.h"


#ifdef SPRINTF_CHAR
# define SPRINTF(x) strlen(sprintf x)
#else
# define SPRINTF(x) ((size_t)sprintf x)
#endif

extern long time(void* ptr);
extern struct conn_entry* hash_search_ct(int direc,struct conn_entry* p_entry);
extern int analysis_eprt(struct sk_buff *skb,struct conn_entry *entry);
extern int in_cksum(u_int16_t *addr, int len);
extern struct conn_entry* get_free_ct(struct conn_entry* pkt_entry);

//第二个参数特意设置成32bit整数，以便与函数体内的32bit数协同运算时，正确处理符号位与进位。
//因为16bit整数的符号位与32bit整数的符号位不是对齐的。
u_int16_t adjust_checksum(u_int16_t old_checksum,u_int32_t delta)
{
	u_int32_t new_checksum;

	new_checksum = old_checksum - delta;
	
	if(new_checksum >> 31 != 0){//负数
		new_checksum--;
	}

	while(new_checksum >> 16){
		new_checksum = (new_checksum & 0xffff) + (new_checksum >> 16);
	}

	return new_checksum;
}

struct in6_addr inet6_addr_ntohs(struct in6_addr *net)
{
	struct in6_addr addr6;
	int i;
	memcpy(&addr6,net,sizeof(struct in6_addr));	
	

	for(i=0;i<8;i++){
		addr6.s6_addr16[i] = ntohs(addr6.s6_addr16[i]);
	}
	
	return addr6;
}


struct in6_addr inet6_addr_htons(struct in6_addr *net)
{
	struct in6_addr addr6;
	int i;
	
	memcpy(&addr6,net,sizeof(struct in6_addr));	
	

	for(i=0;i<8;i++){
		addr6.s6_addr16[i] = htons(addr6.s6_addr16[i]);
	}
	
	return addr6;
}

int nat(struct sk_buff *skb,struct conn_entry* entry,int direc)
{
	/*POSTROUTING点为SNAT，direc为1*/
	/*PREROUTING点为RSNAT，direc为0*/
	struct ipv6hdr* ipv6_header;
	u_int8_t proto;
	u_int8_t pl_proto;
	struct tcphdr* tcp_header;
	struct udphdr* udp_header;
	struct icmp6hdr* icmpv6_header;
	struct ipv6hdr* pl_ipv6_header;
	struct tcphdr* pl_tcp_header;
	struct icmp6hdr* pl_icmpv6_header;
	
	int sum;
	int len;
/*	int flag;*/
/*	int hlen;*/
	
	/*定位IPv6头部*/	
	ipv6_header = ipv6_hdr(skb);		
	proto = ipv6_header->nexthdr;	
	
	/*SNAT*/
	if(direc == 1){
		memcpy(&(ipv6_header->saddr),&(entry->wan_ipv6),sizeof(struct in6_addr));		
		switch (proto){
			case IPPROTO_TCP:
				tcp_header = (struct tcphdr*)((char*)ipv6_header + entry->proto_offset);
				tcp_header->source = entry->wan_port;
				//非FTP报文，正常处理
				if(entry->dport != 0x1500){
					tcp_header->check = adjust_checksum(tcp_header->check,entry->sub_sum);
				}
				else if(entry->eprt_len_change != 0){
					tcp_header->seq = htonl(ntohl(tcp_header->seq) + entry->eprt_len_change);
					
					len = htons(ipv6_header->payload_len);					
					tcp_header->check = 0;
					sum = in_cksum((u_int16_t *)&ipv6_header->saddr,32);
					sum += ntohs(0x6 + len);
					sum += in_cksum((u_int16_t *)tcp_header, len);
					tcp_header->check = CKSUM_CARRY(sum);
				}
				else if(analysis_eprt(skb,entry) != 1){
					tcp_header->check = adjust_checksum(tcp_header->check,entry->sub_sum);
				}	
								
				//EPRT报文			
				else {					
					ipv6_header->payload_len = htons(ntohs(ipv6_header->payload_len) + entry->eprt_len_change);					
					len = htons(ipv6_header->payload_len);					
					tcp_header->check = 0;
					sum = in_cksum((u_int16_t *)&ipv6_header->saddr,32);
					sum += ntohs(0x6 + len);
					sum += in_cksum((u_int16_t *)tcp_header, len);
					tcp_header->check = CKSUM_CARRY(sum);			
				}				
				break;
			case IPPROTO_UDP:
				udp_header = (struct udphdr*)((char*)ipv6_header + entry->proto_offset);
				udp_header->source = entry->wan_port;
				udp_header->check = adjust_checksum(udp_header->check,entry->sub_sum);	
				break;
			case IPPROTO_ICMPV6:
				icmpv6_header = (struct icmp6hdr*)((char*)ipv6_header + entry->proto_offset);
				icmpv6_header->icmp6_identifier = entry->wan_id;
				icmpv6_header->icmp6_cksum = adjust_checksum(icmpv6_header->icmp6_cksum,entry->sub_sum);
				break;
			default:
				break;
		}
	}
	
	/*RSNAT*/
	else {
		memcpy(&(ipv6_header->daddr),&(entry->lan_ipv6),sizeof(struct in6_addr));		
		switch (proto){
			case IPPROTO_TCP:				
				tcp_header = (struct tcphdr*)((char*)ipv6_header + entry->proto_offset);			
				tcp_header->dest = entry->lan_port;
							
				/*处理seq的改变，将ftp server发回的包的ack改为next seq*/
				/*为简化起见，我们处理所有源端口为21的包*/
				/*next seq的算法为client发送包（即上一个包）的seq+len，此处的len即为old eprt命令的长度*/
				/*我们利用改变量来处理这个问题，即上文中用到的len_change*/
				/*new_ack = old_ack - len_change*/			
				
				if(entry->eprt_len_change == 0){
					tcp_header->check = adjust_checksum(tcp_header->check,-entry->sub_sum);
				}
			
				else {
					tcp_header->ack_seq = htonl(ntohl(tcp_header->ack_seq) - entry->eprt_len_change);					

					len = htons(ipv6_header->payload_len);					
					tcp_header->check = 0;
					sum = in_cksum((u_int16_t *)&ipv6_header->saddr,32);
					sum += ntohs(0x6 + len);
					sum += in_cksum((u_int16_t *)tcp_header, len);
					tcp_header->check = CKSUM_CARRY(sum);
				}		
				break;
				
			case IPPROTO_UDP:
				udp_header = (struct udphdr*)((char*)ipv6_header + entry->proto_offset);
				udp_header->dest = entry->lan_port;
				udp_header->check = adjust_checksum(udp_header->check,-entry->sub_sum);
				break;
				
			case IPPROTO_ICMPV6:
				//判断差错、信息报文
				//信息报文
				icmpv6_header = (struct icmp6hdr*)((char*)ipv6_header + entry->proto_offset);				
				
				/*echo报文，正常传输*/
				if(icmpv6_header->icmp6_type == ICMPV6_ECHO_REQUEST || icmpv6_header->icmp6_type == ICMPV6_ECHO_REPLY){
					icmpv6_header->icmp6_identifier = entry->lan_id; 
					icmpv6_header->icmp6_cksum = adjust_checksum(icmpv6_header->icmp6_cksum,-entry->sub_sum);
				}
				
				/*错误报文*/
				/*负载中如果是ICMPv6，则将负载中的源地址和id填入wan表项*/
				/*负载如果是tcp/udp，则将负载中的源地址和端口填入wan表项*/
				else {
					pl_ipv6_header = (struct ipv6hdr *)((char *)icmpv6_header + 8);

					memcpy(&(pl_ipv6_header->saddr),&(entry->lan_ipv6),sizeof(struct in6_addr));
					pl_proto = pl_ipv6_header->nexthdr;
					
					if(pl_proto == IPPROTO_ICMPV6){
						pl_icmpv6_header = (struct icmp6hdr*)((char *)pl_ipv6_header + 40);
						pl_icmpv6_header->icmp6_identifier = entry->lan_id;

						len = htons(pl_ipv6_header->payload_len);	
						pl_icmpv6_header->icmp6_cksum = 0;
				      sum = in_cksum((u_int16_t *)&pl_ipv6_header->saddr, 32);
				      sum += ntohs(IPPROTO_ICMPV6 + len);
				      sum += in_cksum((u_int16_t *)pl_icmpv6_header, len);
				      pl_icmpv6_header->icmp6_cksum = CKSUM_CARRY(sum);												
					}
					else if(pl_proto == IPPROTO_TCP || pl_proto == IPPROTO_UDP){
						pl_tcp_header = (struct tcphdr*)((char *)pl_ipv6_header + 40);
						pl_tcp_header->source = entry->lan_port;
						
						len = htons(pl_ipv6_header->payload_len);	
						pl_tcp_header->check = 0;
				      sum = in_cksum((u_int16_t *)&pl_ipv6_header->saddr, 32);
				      sum += ntohs(pl_proto + len);
				      sum += in_cksum((u_int16_t *)pl_tcp_header, len);
				      pl_tcp_header->check = CKSUM_CARRY(sum);
					}
						
					/*icmp6报文本身校验和*/
					len = htons(ipv6_header->payload_len);	
					icmpv6_header->icmp6_cksum = 0;
		         sum = in_cksum((u_int16_t *)&ipv6_header->saddr, 32);
		         sum += ntohs(IPPROTO_ICMPV6 + len);
		         sum += in_cksum((u_int16_t *)icmpv6_header, len);
		         icmpv6_header->icmp6_cksum = CKSUM_CARRY(sum);
				}			
				
				break;
			default:
				break;
		}
	}
	
	return 1;
}








