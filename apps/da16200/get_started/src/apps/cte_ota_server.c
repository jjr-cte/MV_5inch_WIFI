#include <inttypes.h>
#include "sdk_type.h"
#include "user_cte.h"


#define PKT_TYPE_MD		0x00
#define PKT_TYPE_AMD	0x01

#define PKT_TYPE_ST		0x02
#define PKT_TYPE_AST	0x03

#define PKT_TYPE_RT		0x04
#define PKT_TYPE_ART	0x05

// removed
//#define PKT_TYPE_RST	0x06
//#define PKT_TYPE_ARST	0x07

#define PKT_TYPE_FH		0x08
#define PKT_TYPE_AFH	0x09



typedef union _PackBuffer
{
	uint8_t type;
	ota_packet_amd_t AMD;
	ota_packet_ast_t AST;
	ota_packet_art_t ART;
	ota_packet_afh_t AFH;

} PackBuffer;



static bool check_ART(ota_packet_art_t *ART, uint32_t expected_number)
{
	if (ART->packet_number != expected_number)
	{
		CTE_DBG("ART No.%" PRIu32 ", expected %" PRIu32 "\n", ART->packet_number, expected_number);
		return false;
	}

	uint32_t psz = ART->packet_size;
	if (ART->packet_size > OTA_CTE_APP_LEN)
	{
		CTE_DBG("ART too long: %" PRIu32 "\n", psz);
		return false;
	}

	uint8_t checksum = 0;
	for (uint32_t i = 0; i < psz; i++)
	{
		checksum ^= ART->payload[i];
	}

	if (checksum != ART->checksum)
	{
		CTE_DBG("ART checksum: %02Xh, expected %02Xh\n", checksum, ART->checksum);
		return false;
	}

	return true;
}



static void CTE_OTA_TCP_connect_event(int sock_fd);
static void CTE_OTA_TCP_disconnect_event();
static void CTE_OTA_mobile_response_event(PackBuffer *pack);



////////////////////////////////////////////////////////////////////////////////


#define RB_POOL_NUM 4
struct PoolUnit
{
	struct PoolUnit *next;
	PackBuffer memory;
};
static struct PoolUnit response_buffer_pool[RB_POOL_NUM];
static struct PoolUnit *pool_head;

static SemaphoreHandle_t mutex_rb_pool;

static void initialize_rb_pool()
{
	for (size_t i=0; i<RB_POOL_NUM-1; i++)
	{
		response_buffer_pool[i].next = &response_buffer_pool[i+1];
	}
	response_buffer_pool[RB_POOL_NUM-1].next = NULL;
	pool_head = &response_buffer_pool[0];
}

static PackBuffer* retrieve_response_buffer()
{
	xSemaphoreTake(mutex_rb_pool, portMAX_DELAY);

	if (pool_head == NULL)	// run out of memory pool
	{
		CTE_DBG(YELLOW_COLOR "out of memory\n" CLEAR_COLOR);
		return NULL;
	}

	PackBuffer* buf = &pool_head->memory;
	struct PoolUnit *next = pool_head->next;
	pool_head->next = pool_head;	//mark as used
	pool_head = next;

	xSemaphoreGive(mutex_rb_pool);

	return buf;
}

static void release_response_buffer(PackBuffer *buf)
{
	if (buf == NULL) return;

	xSemaphoreTake(mutex_rb_pool, portMAX_DELAY);
	struct PoolUnit **base = (struct PoolUnit **) buf;
	base[-1] = pool_head;
	pool_head = (struct PoolUnit *)&base[-1];
	xSemaphoreGive(mutex_rb_pool);
}

#ifdef ENABLE_CTE_DBG
static void rb_pool_debug()
{
	CTE_DBG("response buffer pool:\n");
	for (unsigned i = 0; i < RB_POOL_NUM; i++)
	{
		if (&response_buffer_pool[i] == response_buffer_pool[i].next)
		{
			CTE_DBG("[%2u] %08X <allocated>\n", i, &response_buffer_pool[i]);
		}
		else
		{
			CTE_DBG("[%2u] %08X %08X\n", i, &response_buffer_pool[i], response_buffer_pool[i].next);
		}
	}
}
#endif



////////////////////////////////////////////////////////////////////////////////



typedef struct _PackCollector
{
	uint8_t *front;
	ssize_t expect;
	int recv_error;
	int complete_count;

} PackCollector;

static void pkc_init(PackCollector *_this)
{
	_this->front = NULL;
	_this->expect = 0;
	_this->recv_error = 0;
	_this->complete_count = 0;
}

static bool pkc_is_waiting_type(PackCollector *_this)
{
	return _this->front == NULL;
}

static void pkc_expect_content(PackCollector *_this, void *buffer, ssize_t expect_length)
{
	_this->front = ((uint8_t *)buffer);
	_this->expect = expect_length;
}

static void pkc_stop_waiting(PackCollector *_this)
{
	_this->front = NULL;
}

/// @return non-null: complete pack \n
///         NULL: incomplete \n
/// 		if incomplete, check PackCollector.recv_error
bool pkc_receive(PackCollector *pkc, int fd, PackBuffer *pack)
{
	ssize_t recv_len = 0;

	pkc->recv_error = 0;

	do
	{
		if (pkc_is_waiting_type(pkc))
		{
			uint8_t ptype;
			recv_len = recv(fd, &ptype, 1, 0);

			if (recv_len == 1)
			{
				switch (ptype)
				{
				case PKT_TYPE_AMD:
					pkc_expect_content(pkc, &pack->AMD, 12);
					break;

				case PKT_TYPE_AST:
					pkc_expect_content(pkc, &pack->AST, 2);
					break;

				case PKT_TYPE_ART:
					pkc_expect_content(pkc, &pack->ART, 12);
					break;

				case PKT_TYPE_AFH:
					pkc_expect_content(pkc, &pack->AFH, 20);
					break;

				default:
					CTE_DBG("%s() invalid ptype = %02Xh\n", __func__, ptype);
					pkc_stop_waiting(pkc);
					pkc->expect = 0;
				}

				if (pkc->expect > 0)
				{
					pack->type = ptype;
					pkc->front++;
					pkc->expect--;
					pkc->complete_count = 0;
				}
			}
		}
		// fill pack buffer
		else
		{
			recv_len = recv(fd, pkc->front, pkc->expect, 0);

			if (recv_len > 0)
			{
				pkc->expect -= recv_len;
				pkc->front += recv_len;
				if (pkc->expect <= 0)	// pack complete
				{
					pkc_stop_waiting(pkc);
					pkc->complete_count++;

					//post-processing for packs
					switch (pack->type)
					{
					case PKT_TYPE_AMD:
						CTE_DBG("recv AMD:\n");
						memory_dump(pack, 12);
						pack->AMD.total_size = ntohl(pack->AMD.total_size);
						pack->AMD.total_packet = ntohl(pack->AMD.total_packet);
						return true;

					case PKT_TYPE_AST:
						CTE_DBG("recv AST:\n");
						memory_dump(pack, 2);
						return true;

					case PKT_TYPE_ART:
						if (pkc->complete_count == 1)	// first time complete
						{
							pack->ART.packet_number = ntohl(pack->ART.packet_number);
							pack->ART.packet_size = ntohl(pack->ART.packet_size);
							pkc_expect_content(pkc, &pack->ART.payload, pack->ART.packet_size);

							CTE_DBG("recv ART(head): P/N=%u, len=%u\n", pack->ART.packet_number, pack->ART.packet_size);
							memory_dump(pack, 12);
						}
						else
						{
							CTE_DBG("recv ART(body)\n");
							return true;
						}
						break;

					case PKT_TYPE_AFH:
						CTE_DBG("recv AFH:\n");
						memory_dump(pack, 20);
						return true;

					default:
						configASSERT(0);
					}
				}
			}
		}

	} while (recv_len >= 0);

	pkc->recv_error = errno;
	return false;
}


static int OTA_server_configuration()
{
	int sock_server = socket(PF_INET, SOCK_STREAM, 0);
	if (sock_server < 0)
	{
		CTE_DBG_FATAL("socket() error: %d\n", errno);
		return -1;
	}

	int sockopt_reuse = 1;
	if (setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, &sockopt_reuse, sizeof(sockopt_reuse)) != 0)
	{
		CTE_DBG_FATAL("setsockopt error - SO_REUSEADDR: %d\n", errno);
		return -1;
	}

	struct sockaddr_in addr_server;
	memset(&addr_server, 0, sizeof(addr_server));
	addr_server.sin_family = AF_INET;
	addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_server.sin_port = htons(CTE_OTA_SERVER_PORT);
	if (bind(sock_server, (struct sockaddr *)&addr_server, sizeof(struct sockaddr_in)) != 0)
	{
		CTE_DBG_FATAL("bind error: %d\n", errno);
		return -1;
	}

	if (listen(sock_server, 1) != 0)
	{
		CTE_DBG_FATAL("listen error: %d\n", errno);
		return -1;
	}

	return sock_server;
}


static void CTE_OTA_server()
{
	PackBuffer *pack = retrieve_response_buffer();
	if (!pack)
	{
		CTE_DBG_FATAL("initial response buffer unavailable\n");
		return;
	}

	int sock_server = OTA_server_configuration();
	if (sock_server < 0) return;

	while (1)
	{
		int fd = accept(sock_server, NULL, NULL);
		if (fd >= 0)
		{
			CTE_OTA_TCP_connect_event(fd);

			PackCollector collector;
			pkc_init(&collector);

			while (1)
			{
				if (pkc_receive(&collector, fd, pack))
				{
					PackBuffer *new_buf = retrieve_response_buffer();

					if (new_buf != NULL)
					{
						CTE_OTA_mobile_response_event(pack);
						pack = new_buf;
					}
					else
					{
						// do nothing here.
						//若未能取得新的空間，就沿用先前的response buffer，並放棄剛收到的封包
						CTE_DBG(">>> no buffer for packet <<<\n");
					}
				}
				else
				{
					if (collector.recv_error != 0)
					{
						CTE_DBG("TCP closed: %d\n", collector.recv_error);
						CTE_OTA_TCP_disconnect_event();
						break;
					}
				}
			}
		}
	}
}


static void task_OTA_TCP_server(void *args)
{
	DA16X_UNUSED_ARG(args);

	CTE_OTA_server();	// not return if it initialized normally

	while (1)
	{
		vTaskSuspend(NULL);
	}
}



////////////////////////////////////////////////////////////////////////////////


static QueueHandle_t event_queue;


#define OTA_FSM_EVENT_ENUMERATION \
	ENTRIES(TIMEOUT), \
	ENTRIES(RECV_PACK), \
	ENTRIES(LINK_START), \
	ENTRIES(LINK_BREAK), \
	ENTRIES(ERASE_OK), \
	ENTRIES(ERASE_FAIL), \
	ENTRIES(NXP_WRITE_OK), \
	ENTRIES(NXP_WRITE_FAIL), \
	ENTRIES(NXP_WRITE_FINISH), \
	ENTRIES(MD5_OK), \
	ENTRIES(MD5_NG), \
// leave this line blank

#define ENTRIES(x)   EV_ ## x
typedef enum OTAutomataEvent
{
	OTA_FSM_EVENT_ENUMERATION
	OTA_FSM_EVENT_NUM

} EventType;
#undef ENTRIES


#ifdef ENABLE_CTE_DBG

#define ENTRIES(x)   "ev(" #x ")"
const char *text_ota_event[] =
{
	OTA_FSM_EVENT_ENUMERATION
};
#undef ENTRIES

static const char *strOTAEvent(EventType e)
{
	return text_ota_event[e];
}

#endif



typedef struct
{
	EventType type;

	union
	{
		PackBuffer *pack;
		int tcp_fd;
	};

} EventMsg;



#define OTA_FSM_STATE_ENUMERATION \
		ENTRIES(Closed), \
		ENTRIES(SendMD), \
		ENTRIES(WaitAMD), \
		ENTRIES(WaitErase), \
		ENTRIES(SendST), \
		ENTRIES(WaitAST), \
		ENTRIES(SendRT), \
		ENTRIES(WaitART), \
		ENTRIES(NxpWriteFlash), \
		ENTRIES(NxpWaitWrite), \
		ENTRIES(NxpWaitFinale), \
		ENTRIES(SendFH), \
		ENTRIES(WaitAFH), \
		ENTRIES(WaitNxpMD5), \
		ENTRIES(WaitAFH_OtaTypeWiFi), \
		ENTRIES(Terminate), \
// leave this line blank


#define ENTRIES(x)   eOTAState_ ## x
typedef enum OTAutomataStates
{
	OTA_FSM_STATE_ENUMERATION
	OTA_FSM_STATE_NUM

} OTA_FSM_State;
#undef ENTRIES


static const TickType_t state_timeout_setting[OTA_FSM_STATE_NUM] = {
		/* Closed */ portMAX_DELAY,
		/* SendMD */ 0,
		/* WaitAMD */  CTE_OTA_TIMEOUT,
		/* WaitErase */ portMAX_DELAY,
		/* SendST */  0,
		/* WaitAST */  CTE_OTA_TIMEOUT,
		/* SendRT */ 0,
		/* WaitART */  CTE_OTA_TIMEOUT,
		/* NxpWriteFlash */ 0,
		/* NxpWaitWrite */  portMAX_DELAY,
		/* NxpWaitFinale */  portMAX_DELAY,
		/* SendFH */  0,
		/* WaitAFH */  CTE_OTA_TIMEOUT,
		/* WaitNxpMD5 */  pdMS_TO_TICKS(1000),
		/* WaitAFH_OtaTypeWiFi */  CTE_OTA_TIMEOUT,
		/* Terminate */  0,
};


#ifdef ENABLE_CTE_DBG

#define ENTRIES(x)   "OTAState_" #x
const char *text_ota_state[] =
{
	OTA_FSM_STATE_ENUMERATION
};
#undef ENTRIES


static const char *strOTAState(OTA_FSM_State st)
{
	return text_ota_state[st];
}

#endif



TickType_t toe_from, toe_period;

/// @period: portMAX_DELAY means no time-out event
static void set_timeout_event(TickType_t period)
{
	toe_from = xTaskGetTickCount();
	toe_period = period;
}

static TickType_t calc_TicksToWait()
{
	if (toe_period == portMAX_DELAY)
	{
		return portMAX_DELAY;
	}

	TickType_t elapsed = xTaskGetTickCount() - toe_from;
	return (toe_period > elapsed)? (toe_period - elapsed) : 0;
}


typedef struct
{
	uint32_t remain;
	uint8_t *block_addr;
	uint32_t block_len;
	PackBuffer *pack;

} NxpFlashingInfo;

static void nfi_zero_initialize(NxpFlashingInfo *_this)
{
	_this->remain = 0;
	_this->block_addr = NULL;
	_this->block_len = 0;
	_this->pack = NULL;
}

static void nfi_start(NxpFlashingInfo *_this, PackBuffer *pack)
{
	_this->remain = pack->ART.packet_size;
	_this->block_addr = pack->ART.payload;
	_this->block_len = 0;
	_this->pack = pack;
}

static void nfi_write_flash(NxpFlashingInfo *_this)
{
	_this->block_len = configMIN(_this->remain, OTA_CTE_NXP_LEN);
	nxp_ota_msg_data(_this->block_addr, _this->block_len);
}

static void nfi_next_block(NxpFlashingInfo *_this)
{
	_this->block_addr += _this->block_len;
	_this->remain -= _this->block_len;

	CTE_DBG("block remain length = %u\n", _this->remain);
}

static void nfi_cleanup(NxpFlashingInfo *_this)
{
	if(_this->pack) release_response_buffer(_this->pack);
	_this->pack = NULL;
}



static void CTE_OTA_TCP_connect_event(int sock_fd)
{
	EventMsg e;
	e.type = EV_LINK_START;
	e.tcp_fd = sock_fd;
	xQueueSend(event_queue, &e, 0);
}


static void CTE_OTA_TCP_disconnect_event()
{
	EventMsg e;
	e.type = EV_LINK_BREAK;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_WiFi_disassociated()
{
	EventMsg e;
	e.type = EV_LINK_BREAK;
	xQueueSend(event_queue, &e, 0);
}


static void CTE_OTA_mobile_response_event(PackBuffer *pack)
{
	EventMsg e;
	e.type = EV_RECV_PACK;
	e.pack = pack;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_Nxp_erase_answer(bool erase_ok)
{
	EventMsg e;
	e.type = (erase_ok)? EV_ERASE_OK : EV_ERASE_FAIL;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_Nxp_write_block_result(bool write_ok)
{
	EventMsg e;
	e.type = (write_ok)? EV_NXP_WRITE_OK : EV_NXP_WRITE_FAIL;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_Nxp_flash_finished()
{
	EventMsg e;
	e.type = EV_NXP_WRITE_FINISH;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_MD5_result(bool md5_ok)
{
	EventMsg e;
	e.type = (md5_ok)? EV_MD5_OK : EV_MD5_NG;
	xQueueSend(event_queue, &e, 0);
}



static int remote_tcp_fd;
static int request_retry;

static void send_by_wifi(void *data, size_t len)
{
	CTE_DBG("send pkt:\n");
	memory_dump(data, len);

	ssize_t rc = send(remote_tcp_fd, data, len, 0);
	request_retry++;

	if (rc < 0)
	{
		CTE_DBG(">> error: %d\n", errno);
	}
}

static void reset_retx_times()
{
	request_retry = 0;
}

static bool request_sent_too_many()
{
	return request_retry >= CTE_OTA_RETX_MAX;
}



static OTA_FSM_State g_ota_state;

static OTA_FSM_State current_state()
{
	return g_ota_state;
}

static void stateTransfer(OTA_FSM_State new_st)
{
	CTE_DBG("%s -> %s\n", strOTAState(g_ota_state), strOTAState(new_st));
	g_ota_state = new_st;
	set_timeout_event(state_timeout_setting[new_st]);
}


static void reboot_after_da16200_updated()
{
	PRINTF("DA16200 will reboot after 1.5 sec...\n");
	closesocket(remote_tcp_fd);
    vTaskDelay(pdMS_TO_TICKS(1500));
    reboot_func(SYS_REBOOT_POR);
}


void UserApp_CTE_OTA(void *args)
{
	DA16X_UNUSED_ARG(args);

	mutex_rb_pool = xSemaphoreCreateMutex();
	configASSERT(mutex_rb_pool);

	event_queue = xQueueCreate(16, sizeof(EventMsg));
	configASSERT(event_queue);

	initialize_rb_pool();

	cte_generate_crc_table();

	xTaskCreate(
		task_OTA_TCP_server,
		"OTA Server",
		256,
		NULL,
		(OS_TASK_PRIORITY_USER+4),
		NULL
	);

	uint32_t current_packet_number = 0;
	uint32_t total_packets = 0;
	uint32_t total_size = 0;
	UINT DA16200MOD_current_fw_address = 0;
	NxpFlashingInfo nxp_flash_writer;
	mbedtls_md5_context MD5_calc;

	nfi_zero_initialize(&nxp_flash_writer);
	mbedtls_md5_init(&MD5_calc);

	g_ota_state = eOTAState_Closed;
	set_timeout_event(portMAX_DELAY);

	while (1)
	{
		EventMsg ev;

		if (xQueueReceive(event_queue, &ev, calc_TicksToWait()) != pdTRUE )
		{
#ifdef ENABLE_CTE_DBG
			if (toe_period != 0)
			{
				CTE_DBG("> TIME_OUT %" PRIu32 "\n", (uint32_t)toe_period);
			}
#endif
			ev.type = EV_TIMEOUT;
		}
#ifdef ENABLE_CTE_DBG
		else
		{
			if (ev.type == EV_RECV_PACK)
			{
				CTE_DBG("> %s: %02Xh\n", strOTAEvent(ev.type), ev.pack->type);
			}
			else
			{
				CTE_DBG("> %s\n", strOTAEvent(ev.type));
			}
		}
#endif

		switch (ev.type)
		{
		case EV_LINK_START:
			remote_tcp_fd = ev.tcp_fd;
			reset_retx_times();
			ota_type_invalidate();
			CTE_DBG("(global) ");
			stateTransfer(eOTAState_SendMD);
			break;

		case EV_LINK_BREAK:
			// no termination under Closed state
			if (current_state() != eOTAState_Closed)
			{
				CTE_DBG("(global) ");
				stateTransfer(eOTAState_Terminate);
			}
			break;


		default:

			switch (current_state())
			{
			case eOTAState_Closed:
				break;

			case eOTAState_SendMD:
			{
				ota_packet_md_t p;
				memset(&p, 0, sizeof(p));
				p.type = PKT_TYPE_MD;
				send_by_wifi(&p, sizeof(p));
				stateTransfer(eOTAState_WaitAMD);
			}
				break;

			case eOTAState_WaitAMD:
				if (ev.type == EV_RECV_PACK)
				{
					if (ev.pack->type == PKT_TYPE_AMD)
					{
						total_size = ev.pack->AMD.total_size;
						total_packets = ev.pack->AMD.total_packet;
						CTE_DBG("total size = %" PRIu32 "\n", total_size);
						CTE_DBG("total packet = %" PRIu32 "\n", total_packets);
						reset_retx_times();

						switch (ev.pack->AMD.ota_type)
						{
						case OTA_TYPE_NXP:
							CTE_DBG("+META:%08" PRIu32 "\n", total_size);
							PRINTF_ATCMD("\r\n+META:%08" PRIu32 "\r\n", total_size);

							ota_type_set(OTA_TYPE_NXP);
							stateTransfer(eOTAState_WaitErase);
							break;

						case OTA_TYPE_WIFI:
							DA16200MOD_current_fw_address = get_cur_boot_index() ? SFLASH_RTOS_0_BASE : SFLASH_RTOS_1_BASE;

							ota_type_set(OTA_TYPE_WIFI);
							reset_retx_times();
							stateTransfer(eOTAState_SendST);
							break;

						default:
							CTE_DBG_XXX();
							CTE_DBG("invalid OTA type in AMD : %d\n", ev.pack->AMD.ota_type);
							stateTransfer(eOTAState_Terminate);
						}
					}
				}
				else if (ev.type == EV_TIMEOUT)
				{
					if (request_sent_too_many())
					{
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("request retry = %u\n", request_retry);
						stateTransfer(eOTAState_SendMD);
					}
				}
				break;

			case eOTAState_WaitErase:
				if (ev.type == EV_ERASE_OK)
				{
					stateTransfer(eOTAState_SendST);
				}
				else if (ev.type == EV_ERASE_FAIL)
				{
					stateTransfer(eOTAState_Terminate);
				}
				break;

			case eOTAState_SendST:
			{
				ota_packet_st_t p;
				memset(&p, 0, sizeof(p));
				p.type = PKT_TYPE_ST;
				send_by_wifi(&p, sizeof(p));
				stateTransfer(eOTAState_WaitAST);
			}
				break;

			case eOTAState_WaitAST:
				if (ev.type == EV_RECV_PACK)
				{
					if (ev.pack->type == PKT_TYPE_AST)
					{
			            mbedtls_md5_init(&MD5_calc);
			            mbedtls_md5_starts_ret(&MD5_calc);
						current_packet_number = 0;
						reset_retx_times();
						stateTransfer(eOTAState_SendRT);
					}
				}
				else if (ev.type == EV_TIMEOUT)
				{
					if (request_sent_too_many())
					{
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("request retry = %u\n", request_retry);
						stateTransfer(eOTAState_SendST);
					}
				}
				break;

			case eOTAState_SendRT:
				if (current_packet_number < total_packets)
				{
					ota_packet_rt_t p;
					memset(&p, 0, sizeof(p));
					p.type = PKT_TYPE_RT;
					p.packet_number = htonl(current_packet_number);

					send_by_wifi(&p, sizeof(p));
					stateTransfer(eOTAState_WaitART);
				}
				else
				{
					switch (ota_type_get())
					{
					case OTA_TYPE_NXP:
						stateTransfer(eOTAState_NxpWaitFinale);
						break;

					case OTA_TYPE_WIFI:
						if (ota_update_sflash_crc(get_cur_boot_index()? SFLASH_RTOS_0_BASE : SFLASH_RTOS_1_BASE))
						{
							CTE_DBG("Update DA16200: toggle boot index (now: %d)\n", get_cur_boot_index());
							ota_update_toggle_boot_index();
							stateTransfer(eOTAState_SendFH);
						}
						else
						{
							CTE_DBG("Update DA16200: bad CRC\n");
							stateTransfer(eOTAState_Terminate);
						}
						break;

					default:
						configASSERT(0);
						stateTransfer(eOTAState_Terminate);
					}
				}
				break;

			case eOTAState_WaitART:
				if (ev.type == EV_RECV_PACK)
				{
					if (ev.pack->type == PKT_TYPE_ART && check_ART(&ev.pack->ART, current_packet_number))
					{
						current_packet_number++;
						reset_retx_times();
						mbedtls_md5_update_ret(&MD5_calc, ev.pack->ART.payload, ev.pack->ART.packet_size);

						switch (ota_type_get())
						{
						case OTA_TYPE_NXP:
							nfi_start(&nxp_flash_writer, ev.pack);
							ev.pack = NULL;
							stateTransfer(eOTAState_NxpWriteFlash);
							break;

						case OTA_TYPE_WIFI:
							CTE_DBG("write DA16200 SFlash: 0x%08X %" PRIu32 "\n", DA16200MOD_current_fw_address, ev.pack->ART.packet_size);

							if (writeDataToFlash(
									DA16200MOD_current_fw_address,
									(UCHAR *)ev.pack->ART.payload,
									(UINT) ev.pack->ART.packet_size
								) == TRUE )
							{
								DA16200MOD_current_fw_address += ev.pack->ART.packet_size;
								stateTransfer(eOTAState_SendRT);
							}
							else
							{
								stateTransfer(eOTAState_Terminate);
							}
							break;

						default:
							configASSERT(0);
							stateTransfer(eOTAState_Terminate);
						}
					}
				}
				else if (ev.type == EV_TIMEOUT)
				{
					if (request_sent_too_many())
					{
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("request retry = %u\n", request_retry);
						stateTransfer(eOTAState_SendRT);
					}
				}
				break;

			case eOTAState_NxpWriteFlash:
				if (nxp_flash_writer.remain > 0)
				{
					nfi_write_flash(&nxp_flash_writer);
					stateTransfer(eOTAState_NxpWaitWrite);
				}
				else
				{
					nfi_cleanup(&nxp_flash_writer);
					stateTransfer(eOTAState_SendRT);
				}
				break;

			case eOTAState_NxpWaitWrite:
				if (ev.type == EV_NXP_WRITE_OK)
				{
					nfi_next_block(&nxp_flash_writer);
					stateTransfer(eOTAState_NxpWriteFlash);
				}
				else if(ev.type == EV_NXP_WRITE_FAIL)
				{
					CTE_DBG("retry Nxp Write Flash\n");
					stateTransfer(eOTAState_NxpWriteFlash);
				}
				break;

			case eOTAState_NxpWaitFinale:
				if (ev.type == EV_NXP_WRITE_FINISH)
				{
					nxp_answer_ack();
					stateTransfer(eOTAState_SendFH);
				}
				break;

			case eOTAState_SendFH:
			{
				ota_packet_fh_t p;
				memset(&p, 0, sizeof(p));
				p.type = PKT_TYPE_FH;
				send_by_wifi(&p, sizeof(p));

				switch (ota_type_get())
				{
				case OTA_TYPE_NXP:
					stateTransfer(eOTAState_WaitAFH);
					break;

				case OTA_TYPE_WIFI:
					stateTransfer(eOTAState_WaitAFH_OtaTypeWiFi);
					break;

				default:
					configASSERT(0);
					stateTransfer(eOTAState_Terminate);
				}
			}
				break;

			case eOTAState_WaitAFH:
				if (ev.type == EV_RECV_PACK)
				{
					if (ev.pack->type == PKT_TYPE_AFH)
					{
						unsigned char md5_hash[16];
						mbedtls_md5_finish_ret(&MD5_calc, md5_hash);

						// 在DA上計算MD5只是為了除錯，不參與實際流程
						CTE_DBG("our MD5 hash:\n");
						md5dump(md5_hash);
						CTE_DBG("app MD5 hash:\n");
						md5dump(ev.pack->AFH.md5sum);

						char buf[64];
						strcpy(buf, "+MD5:");
						char *p = buf + strlen(buf);
						for (size_t i = 0; i < 16; i++)
						{
							//lower case hexadecimal
							sprintf(p, "%02x", ev.pack->AFH.md5sum[i]);
							p += 2;
						}

						CTE_DBG("%s\n", buf);
						PRINTF_ATCMD("\r\n%s\r\n", buf);

						stateTransfer(eOTAState_WaitNxpMD5);
					}
				}
				else if (ev.type == EV_TIMEOUT)
				{
					if (request_sent_too_many())
					{
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("request retry = %u\n", request_retry);
						stateTransfer(eOTAState_SendFH);
					}
				}
				break;

			case eOTAState_WaitNxpMD5:
				if (ev.type == EV_TIMEOUT || ev.type == EV_MD5_OK || ev.type == EV_MD5_NG)
				{
					stateTransfer(eOTAState_Terminate);
				}
				break;

			case eOTAState_WaitAFH_OtaTypeWiFi:
				if (ev.type == EV_RECV_PACK)
				{
					if (ev.pack->type == PKT_TYPE_AFH)
					{
						reboot_after_da16200_updated();
						stateTransfer(eOTAState_Terminate);
					}
				}
				else if (ev.type == EV_TIMEOUT)
				{
					if (request_sent_too_many())
					{
						// just reboot. it doesn't matter if we don't get AFH
						reboot_after_da16200_updated();
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("request retry = %u\n", request_retry);
						stateTransfer(eOTAState_SendFH);
					}
				}
				break;

			case eOTAState_Terminate:
				//release resource
				nfi_cleanup(&nxp_flash_writer);
				mbedtls_md5_free(&MD5_calc);
				closesocket(remote_tcp_fd);

#ifdef ENABLE_CTE_DBG
				rb_pool_debug();
#endif

				stateTransfer(eOTAState_Closed);
				break;

			default:
				CTE_DBG_FATAL("Unknown state: %d\n", ev.type);
				stateTransfer(eOTAState_Terminate);
			}
		}


		// clean up event message
		switch (ev.type)
		{
		case EV_RECV_PACK:
			if (ev.pack) release_response_buffer(ev.pack);
			break;

		default:
			; //do nothing
		}
	}
}

