/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"

// �����д���ķ��鲢�����������
// ������
// pBuffer��ָ�򱻶��������ָ��
// type�����鱻������ԭ�򣬿�ȡ����ֵ��
// STUD_IP_TEST_CHECKSUM_ERROR //IP У��ͳ���
// STUD_IP_TEST_TTL_ERROR //TTL ֵ����
// STUD_IP_TEST_VERSION_ERROR //IP �汾�Ŵ�
// STUD_IP_TEST_HEADLEN_ERROR //ͷ�����ȴ�
// STUD_IP_TEST_DESTINATION_ERROR //Ŀ�ĵ�ַ��
extern void ip_DiscardPkt(char* pBuffer,int type);

// ���ͷ���
// ������
// pBuffer��ָ������͵� IPv4 ����ͷ����ָ��
// length�������͵� IPv4 ���鳤��
// �ѷ��齻���²���ɷ���
extern void ip_SendtoLower(char*pBuffer,int length);

// �ϲ����
// ������
// pBuffer��ָ��Ҫ�Ͻ����ϲ�Э�鱨��ͷ����ָ��
// length���Ͻ����ĳ���
extern void ip_SendtoUp(char *pBuffer,int length);

// ��ȡ���� IPv4 ��ַ
// unsigned int getIpv4Address( )
// ��������
extern unsigned int getIpv4Address();


struct Ipv4
{
	// �汾��+�ײ�����
	char Version_Header;
	// ��������
	char TOS;
	// �ܳ���
	short TotalLength;
	// ��ʶ
	short Identification;
	// ��־λ+Ƭƫ��
	short Fragment_Offset;
	// ����ʱ��(TTL)
	char TTL;
	// Э��
	char Protocol;
	// �ײ�У���
	short HeaderChecksum;
	// ԴIP��ַ
	unsigned int SourceAddress;
	// Ŀ��IP��ַ
	unsigned int DestinationAddress;
	Ipv4() {
		memset(this,0,sizeof(Ipv4));
	}

	Ipv4(unsigned int len,unsigned int srcAddr,unsigned int dstAddr,byte protocol,byte ttl) {
		memset(this,0,sizeof(Ipv4));
		// �汾��Ϊ0100 �ײ�����Ϊ0101
		// ���Ϊ01000101-> 0x45
		Version_Header = 0x45;
		// ���ݰ�����+�ײ�����Ϊ�ܳ���
		TotalLength = htons(len+20);
		// ����ttl
		TTL = ttl;
		// ����Э��
		Protocol = protocol;
		// ����ԴIP
		SourceAddress = htonl(srcAddr);
		// ����Ŀ��IP
		DestinationAddress = htonl(dstAddr);

		char *pBuffer = new char[sizeof(Ipv4)];
		memcpy(pBuffer,this,sizeof(Ipv4));
		// ����У���
		int sum = 0;
		for(int i = 0; i < 10; i++) {
			if(i != 5) {
				sum += (int)((unsigned char)pBuffer[i*2] << 8);
				sum += (int)((unsigned char)pBuffer[i*2+1]);
			}
		}
		while((sum & 0xffff0000) != 0) {
			sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
		}
		unsigned short int headerchecksum = sum;
		HeaderChecksum = htons(~headerchecksum);
	}
};



// implemented by students
// ����Ҫ�ϲ�Э���һ���������Ϣ�ύ���ϲ�Э��
// (1)�����յ��� IPv4 ����ͷ�����ֶΣ������汾�ţ�Version����ͷ�����ȣ�IP Head length��������ʱ�䣨Time to live���Լ�ͷУ��ͣ�Header checksum���ֶΡ�
// ���ڳ���ķ������ ip_DiscardPkt( )��������˵���������͡�
// (2)��� IPv4 �����Ƿ�Ӧ���ɱ������ա�
// ��������Ŀ�ĵ�ַ�Ǳ�����ַ��㲥��ַ����˵���˷����Ƿ��͸������ģ�
// �������ip_DiscardPkt( )��������˵���������͡�
// (3)��� IPV4 ����Ӧ���ɱ������գ�����ȡ�õ��ϲ�Э�����ͣ�����ip_SendtoUp( )�ӿں���������ϵͳ���к������մ���
int stud_ip_recv(char *pBuffer,unsigned short length)
{
	// struct Ipv4
	Ipv4 *ipv4 = (Ipv4*)pBuffer;
	int version = 0xf & ((ipv4->Version_Header)>> 4);
	// �ж��Ƿ�Ϊipv4����
	if(version != 4)  
	{
    	ip_DiscardPkt(pBuffer,STUD_IP_TEST_VERSION_ERROR);
    	return 1;
	}
	int headerLength = 0xf & ipv4->Version_Header;
	// �ж��Ƿ�С��5�������ײ���Ҫ5*4�ֽڣ������Ҫ���ڵ���5
	if(headerLength < 5)
	{
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_HEADLEN_ERROR);
    	return 1;
	}
	// �ж����������Ƿ�Ϊ0
	int ttl = (int)ipv4->TTL;
	if(ttl == 0) 
	{
    	ip_DiscardPkt(pBuffer,STUD_IP_TEST_TTL_ERROR);
    	return 1;
	}
	// �ж�Ŀ�ĵ�ַ�Ƿ�Ϊ����
	int destination_address = ntohl(ipv4->DestinationAddress);  
	if(destination_address != getIpv4Address() && destination_address != 0xffffffff) 
	{
    	ip_DiscardPkt(pBuffer,STUD_IP_TEST_DESTINATION_ERROR);
    	return 1;
	}
	// �ж�У����Ƿ���ȷ
	int header_checksum = ntohs(ipv4->HeaderChecksum);
	int sum = 0;
  	for(int i = 0; i < headerLength*2; i++) {
    	if(i!=5)
    	{
      	sum += (int)((unsigned char)pBuffer[i*2] << 8);
      	sum += (int)((unsigned char)pBuffer[i*2+1]);
    	}
	}
	while((sum & 0xffff0000) != 0) {
    	sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
	}
	unsigned short int headerchecksum = (~sum) & 0xffff;
	if(headerchecksum != header_checksum) {
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}
	// �������ͱ���
	ip_SendtoUp(pBuffer,length);
	return 0;
}

// ���ͽӿں���
// (1)�������������������ݴ�С������ȷ������Ĵ洢�ռ�Ĵ�С���������Ĵ洢�ռ䡣
// (2)���� IPv4 Э���׼��д IPv4 ����ͷ�����ֶΣ���ʶ����Identification���ֶο���ʹ��һ�����������д��
// ��ע�⣺�����ֶ�������Ҫת���������ֽ���
// (3)��� IPv4 ����ķ�װ�󣬵��� ip_SendtoLower( )�ӿں�����ɺ����ķ��ʹ����������ս����鷢�͵������С�
int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,unsigned int dstAddr,byte protocol,byte ttl)
{
	char *pack_to_sent = new char[len+20];
	memset(pack_to_sent,0,len+20);
	*((Ipv4*)pack_to_sent) = Ipv4(len,srcAddr,dstAddr,protocol,ttl);
	memcpy(pack_to_sent+20,pBuffer,len);
	ip_SendtoLower(pack_to_sent,len+20);
	delete[] pack_to_sent;
	return 0;
}
