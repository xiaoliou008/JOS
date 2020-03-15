#include <kern/e1000.h>

// LAB 6: Your driver code here
volatile uint32_t *e1000;
struct tx_desc td_desc[E1000_TD_NUM];		// aligned on 16-byte 
struct rx_desc rd_desc[E1000_RD_NUM];
char e1000_td_buf[E1000_TD_NUM][PACKET_MAX_SIZE];
char e1000_rd_buf[E1000_RD_NUM][RECV_BUF_SIZE];

int pci_e1000_attach(struct pci_func *pcif)
{
	// enable and map memory
	pci_func_enable(pcif);
	e1000 = (uint32_t *)mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("status register: 0x%x\n", e1000[E1000_STATUS>>2]);
	// initialize transmit
	for(int i=0;i<E1000_TD_NUM;i++){
		td_desc[i].addr = (uint64_t) PADDR(e1000_td_buf[i]);
		td_desc[i].status = E1000_TXD_STAT_DD;	//release the descriptor
	}
	e1000[E1000_TDBAL>>2] = PADDR(td_desc);
	e1000[E1000_TDBAH>>2] = 0;
	e1000[E1000_TDLEN>>2] = sizeof(td_desc);
	e1000[E1000_TDH>>2] = e1000[E1000_TDT>>2] = 0;
	e1000[E1000_TCTL>>2] = E1000_TCTL_EN | E1000_TCTL_PSP;
	e1000[E1000_TCTL>>2] |= 0x10 << 4;	// TCTL.CT = 10hE1000_RCTL_EN
	e1000[E1000_TCTL>>2] |= 0x40 << 12;	// TCTL.COLD = 40h
	e1000[E1000_TIPG>>2] = 10;			// IPGT = 10
	e1000[E1000_TIPG>>2] |= 8 << 10;	// IPGR1 = 8
	e1000[E1000_TIPG>>2] |= 6 << 20;	// IPGR2 = 6
	// initialize receive
	for(int i=0;i<E1000_RD_NUM;i++){
		rd_desc[i].addr = (uint64_t) PADDR(e1000_rd_buf[i]);
		rd_desc[i].status = 0;
	}
	e1000[E1000_RAL>>2] = DEFAULT_MAC_LOW;	// MAC address
	e1000[E1000_RAH>>2] = DEFAULT_MAC_HIGH | (1<<31);	// don't forget address valid
	e1000[E1000_RDBAL>>2] = PADDR(rd_desc);	
	e1000[E1000_RDBAH>>2] = 0;	
	e1000[E1000_RDLEN>>2] = sizeof(rd_desc);
	e1000[E1000_RDH>>2] = 1;		// bug if init 0
	e1000[E1000_RDT>>2] = 0;
	e1000[E1000_RCTL>>2] = E1000_RCTL_SECRC | E1000_RCTL_BAM;
	e1000[E1000_RCTL>>2] |= E1000_RCTL_EN;	// should be left until end
	return 0;
}

int pci_e1000_try_send(char *buf, size_t size)
{
	if(size > PACKET_MAX_SIZE){
		cprintf("error: pci_e1000_try_send too large packet\n");
		return -E_INVAL;
	}
	int i = e1000[E1000_TDT>>2];
	if(td_desc[i].status & E1000_TXD_STAT_DD){
		memcpy((void *)KADDR((uint32_t)td_desc[i].addr), buf, size);
		td_desc[i].cmd |= (E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP) >> 24;
		td_desc[i].status &= ~E1000_TXD_STAT_DD;
		td_desc[i].length = (uint16_t)size;
		e1000[E1000_TDT>>2] = (e1000[E1000_TDT>>2] + 1) % E1000_TD_NUM;
	} else {	// full
//		cprintf("pci_e1000_try_send: ring is full\n");
		return -E_NO_MEM;
	}
	return 0;
}

int pci_e1000_receive(char *buf, size_t *size)
{
	int i = (e1000[E1000_RDT>>2] + 1) % E1000_RD_NUM;
	if(rd_desc[i].status & E1000_RXD_STAT_DD){
//		size_t sz = MIN(size, rd_desc[i].length);
		memcpy(buf, (void *)KADDR((uint32_t)rd_desc[i].addr), rd_desc[i].length);
		if(size != NULL) *size = rd_desc[i].length;
		rd_desc[i].status = 0;
		e1000[E1000_RDT>>2] = i;
	} else {
		return -E_NO_MEM;
	}
	return 0;
}

/*
int pci_e1000_receive(char *buf, size_t *size)
{
	int i = e1000[E1000_RDT>>2];
	if(rd_desc[i].status & E1000_RXD_STAT_DD){
//		size_t sz = MIN(size, rd_desc[i].length);
		memcpy(buf, (void *)KADDR((uint32_t)rd_desc[i].addr), rd_desc[i].length);
		if(size != NULL) *size = rd_desc[i].length;
		e1000[E1000_RDT>>2] = (i + 1) % E1000_RD_NUM;
	} else {
		return -E_NO_MEM;
	}
	return 0;
}
*/
