#include <inttypes.h>
#include "sdk_type.h"
#include "user_cte.h"
#include <assert.h>



#define PKT_TYPE_MD		0x00
#define PKT_TYPE_AMD	0x01

#define PKT_TYPE_ST		0x02
#define PKT_TYPE_AST	0x03

#define PKT_TYPE_RT		0x04
#define PKT_TYPE_ART	0x05

// deprecated pack type
//#define PKT_TYPE_RST	0x06
//#define PKT_TYPE_ARST	0x07

#define PKT_TYPE_FH		0x08
#define PKT_TYPE_AFH	0x09

#define PKT_TYPE_RV		0x0A	//report version



typedef union _PackBuffer
{
	uint8_t type;
	ota_packet_amd_t AMD;
	ota_packet_ast_t AST;
	ota_packet_art_t ART;
	ota_packet_afh_t AFH;

} PackBuffer;



static void CTE_OTA_TCP_connected(int sock_fd);
static void CTE_OTA_TCP_disconnected();
static void CTE_OTA_mobile_response_event(PackBuffer *pack);



////////////////////////////////////////////////////////////////////////////////
/// memory pool mechanism for response buffer

#define RB_POOL_NUM 4
struct PoolUnit
{
	struct PoolUnit *next;
	PackBuffer memory;
};

static struct PoolUnit response_buffer_pool[RB_POOL_NUM];
static struct PoolUnit *pool_head;
static SemaphoreHandle_t mutex_rb_pool;

// initialize response_buffer_pool[]
static void initialize_rb_pool()
{
	for (size_t i=0; i<RB_POOL_NUM-1; i++)
	{
		response_buffer_pool[i].next = &response_buffer_pool[i+1];
	}
	response_buffer_pool[RB_POOL_NUM-1].next = NULL;
	pool_head = &response_buffer_pool[0];

	mutex_rb_pool = xSemaphoreCreateMutex();
	configASSERT(mutex_rb_pool);
}

//allocate a response buffer
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

//free a response buffer
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
//testing if there is memory leaking
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
/// PackCollectInfo & pkc_* APIs : collect a complete packet

typedef struct _PackCollector
{
	uint8_t *front;
	ssize_t expect;
	int recv_error;
	int complete_count;

} PackCollectInfo;

//initialize PackCollector structure
static void pkc_init(PackCollectInfo *_this)
{
	_this->front = NULL;
	_this->expect = 0;
	_this->recv_error = 0;
	_this->complete_count = 0;
}

//正在等待接收PKT_TYPE_
static bool pkc_is_waiting_PType(PackCollectInfo *_this)
{
	return _this->front == NULL;
}

//設定預期接收的長度，以及buffer address
static void pkc_expect_content(PackCollectInfo *_this, void *buffer, ssize_t expect_length)
{
	_this->front = ((uint8_t *)buffer);
	_this->expect = expect_length;
}

//標記為接收完成
static void pkc_complete(PackCollectInfo *_this)
{
	_this->front = NULL;
}

/// 接收封包
/// @return true: complete pack \N
///         false: connection closed, check the recv_error field
static bool pkc_receive(PackCollectInfo *pkc, int fd, PackBuffer *pack)
{
	ssize_t recv_len = 0;

	pkc->recv_error = 0;

	do
	{
		if (pkc_is_waiting_PType(pkc))
		{
			//讀出PKT_TYPE並決定後面要怎麼接收封包

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
					CTE_DBG("invalid PKT_TYPE %02Xh\n", ptype);
					closesocket(fd);
					pkc->recv_error = 0;
					return false;
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
		else
		{
			// 填充封包內容

			recv_len = recv(fd, pkc->front, pkc->expect, 0);

			if (recv_len > 0)
			{
				pkc->expect -= recv_len;
				pkc->front += recv_len;

				if (pkc->expect <= 0)	// pack complete
				{
					pkc_complete(pkc);
					pkc->complete_count++;

					//封包後製
					//僅限印出一些資訊與進行endian轉換
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

	} while (recv_len > 0);

	pkc->recv_error = (recv_len < 0)? errno : 0;
	return false;
}


//設定 OTA Server
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
			CTE_OTA_TCP_connected(fd);

			PackCollectInfo collector;
			pkc_init(&collector);

			while (1)
			{
				if (pkc_receive(&collector, fd, pack))
				{
					//若未能取得新的空間，就沿用先前的response buffer，並丟棄剛收到的封包

					PackBuffer *new_buf = retrieve_response_buffer();

					if (new_buf != NULL)
					{
						CTE_OTA_mobile_response_event(pack);
						pack = new_buf;
					}
					else
					{
						CTE_DBG(">>> no buffer for packet <<<\n");
					}
				}
				else
				{
					CTE_DBG("TCP closed: %d\n", collector.recv_error);
					CTE_OTA_TCP_disconnected();
					break;
				}
			}
		}
	}
}


static void task_OTA_TCP_server(void *args)
{
	DA16X_UNUSED_ARG(args);

	CTE_OTA_server();	// not return if it's initialized normally

	while (1)
	{
		vTaskSuspend(NULL);
	}
}


////////////////////////////////////////////////////////////////////////////////
/// information structure for flashing NXP ROM

typedef struct
{
	uint32_t remain;
	uint8_t *source;
	uint32_t block_len;
	PackBuffer *pack;
	uint32_t retx_times;

} NxpFlashingInfo;

static void nfi_init(NxpFlashingInfo *_this)
{
	_this->remain = 0;
	_this->source = NULL;
	_this->block_len = 0;
	_this->pack = NULL;
	_this->retx_times = 0;
}

//開始這次寫入
static void nfi_begin(NxpFlashingInfo *_this, PackBuffer *pack)
{
	_this->remain = pack->ART.packet_size;
	_this->source = pack->ART.payload;
	_this->block_len = 0;
	_this->pack = pack;
	_this->retx_times = 0;
}

//寫入flash
static void nfi_write_flash(NxpFlashingInfo *_this)
{
	//限制每次送出的長度
	_this->block_len = configMIN(_this->remain, OTA_CTE_NXP_LEN);
	_this->retx_times++;

	nxp_ota_msg_data(_this->source, _this->block_len);
}

static bool nfi_tried_too_many(NxpFlashingInfo *_this)
{
	if (_this->retx_times >= CTE_FLASH_WRITE_RETX_MAX)
	{
		return true;
	}

	return false;
}

//移動指標到下次要送出的資料
static void nfi_next_block(NxpFlashingInfo *_this)
{
	_this->source += _this->block_len;
	_this->remain -= _this->block_len;
	_this->retx_times = 0;
	CTE_DBG("remain = %" PRIu32 "\n", _this->remain);
}

//釋放PackBuffer
static void nfi_cleanup(NxpFlashingInfo *_this)
{
	if(_this->pack) release_response_buffer(_this->pack);
	_this->pack = NULL;
}


////////////////////////////////////////////////////////////////////////////////
/// State Machine Events definition

#define OTA_FSM_EVENT_ENUMERATION \
	ENTRIES(TIMEOUT), \
	ENTRIES(RECV_PACK), \
	ENTRIES(LINK_START), \
	ENTRIES(LINK_BREAK), \
	ENTRIES(ERASE_OK), \
	ENTRIES(ERASE_FAIL), \
	ENTRIES(NXP_WRITE_ACK), \
	ENTRIES(NXP_WRITE_REWRITE), \
	ENTRIES(NXP_WRITE_ABORT), \
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

#endif	/* ENABLE_CTE_DBG */


////////////////////////////////////////////////////////////////////////////////
/// State definition

#define OTA_FSM_STATE_ENUMERATION \
		ENTRIES(Closed), \
		ENTRIES(SendMD), \
		ENTRIES(WaitAMD), \
		ENTRIES(WaitErase), \
		ENTRIES(SendST), \
		ENTRIES(WaitAST), \
		ENTRIES(SendRV), \
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


//每個state的超時設定
//一旦進入這個state超過以下的時間，且佇列當下沒有其他事件，就會產生TIMEOUT event
//若設為portMAX_DELAY，即使過再久都不會有TIMEOUT event
const TickType_t state_timeout_period[] = {
		/* Closed */ portMAX_DELAY,
		/* SendMD */ 0,
		/* WaitAMD */  CTE_OTA_TIMEOUT,
		/* WaitErase */ portMAX_DELAY,
		/* SendST */  0,
		/* WaitAST */  CTE_OTA_TIMEOUT,
		/* SendRV */ 0,
		/* SendRT */ 0,
		/* WaitART */  CTE_OTA_TIMEOUT,
		/* NxpWriteFlash */ 0,
		/* NxpWaitWrite */  portMAX_DELAY,
		/* NxpWaitFinale */  portMAX_DELAY,
		/* SendFH */  0,
		/* WaitAFH */  CTE_OTA_TIMEOUT,
		/* WaitNxpMD5 */  pdMS_TO_TICKS(5000),
		/* WaitAFH_OtaTypeWiFi */  CTE_OTA_TIMEOUT,
		/* Terminate */  0,
};

static_assert(OTA_FSM_STATE_NUM == sizeof(state_timeout_period) / sizeof(TickType_t), "missing timeout config in state_timeout_period[]");


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

#endif	/* ENABLE_CTE_DBG */


////////////////////////////////////////////////////////////////////////////////
/// packet send functions

static int remote_tcp_fd = -1;
static int pkt_retxs;

static void send_packet(void *data, size_t len)
{
	CTE_DBG("send PKT: %" PRIu32 "\n", (uint32_t)len);
	memory_dump(data, len);

	pkt_retxs++;

	if (remote_tcp_fd < 0)
	{
		CTE_DBG("no connection\n");
		return;
	}

	ssize_t rc = send(remote_tcp_fd, data, len, 0);

	if (rc < 0)
	{
		CTE_DBG("send error: %d\n", errno);
	}
}

static void reset_packet_retx_times()
{
	pkt_retxs = 0;
}

static bool packet_sent_too_many()
{
	return pkt_retxs >= CTE_OTA_RETX_MAX;
}


////////////////////////////////////////////////////////////////////////////////
/// TIMEOUT event functions

TickType_t toe_from, toe_period;

static void set_timeout_event(OTA_FSM_State st)
{
	toe_from = xTaskGetTickCount();
	toe_period = state_timeout_period[st];
}

//決定等待事件佇列要等多久
static TickType_t calc_TicksToWait()
{
	if (toe_period == portMAX_DELAY)
	{
		return portMAX_DELAY;
	}

	TickType_t elapsed = xTaskGetTickCount() - toe_from;
	return (toe_period > elapsed)? (toe_period - elapsed) : 0;
}


////////////////////////////////////////////////////////////////////////////////
/// OTA flow control

//事件的資料結構
typedef struct
{
	EventType type;

	union
	{
		PackBuffer *pack;
		int tcp_fd;
	};

} EventMsg;


static OTA_FSM_State _ota_state;

static OTA_FSM_State current_state()
{
	return _ota_state;
}

static void stateTransfer(OTA_FSM_State new_st)
{
	CTE_DBG_PRINT_TS();
	CTE_DBG("%s -> %s\n", strOTAState(_ota_state), strOTAState(new_st));
	_ota_state = new_st;
	set_timeout_event(new_st);
}


static QueueHandle_t event_queue;

//通知flow control TCP連線建立
static void CTE_OTA_TCP_connected(int sock_fd)
{
	EventMsg e;
	e.type = EV_LINK_START;
	e.tcp_fd = sock_fd;
	xQueueSend(event_queue, &e, 0);
}

//通知flow control TCP連線斷開
static void CTE_OTA_TCP_disconnected()
{
	EventMsg e;
	e.type = EV_LINK_BREAK;
	xQueueSend(event_queue, &e, 0);
}

//通知flow control WiFi離線
void CTE_OTA_WiFi_disassociated()
{
	// do nothing
}

//通知flow control 收到封包
static void CTE_OTA_mobile_response_event(PackBuffer *pack)
{
	EventMsg e;
	e.type = EV_RECV_PACK;
	e.pack = pack;
	xQueueSend(event_queue, &e, 0);
}

//通知flow control NXP的Flash擦除結果
void CTE_OTA_Nxp_erase_answer(bool erase_ok)
{
	EventMsg e;
	e.type = (erase_ok)? EV_ERASE_OK : EV_ERASE_FAIL;
	xQueueSend(event_queue, &e, 0);
}

//通知知flow control NXP的Flash寫入沒問題
extern void CTE_OTA_Nxp_write_block_ok()
{
	EventMsg e;
	e.type = EV_NXP_WRITE_ACK;
	xQueueSend(event_queue, &e, 0);
}

//通知知flow control NXP的Flash寫入失敗，需要重送
extern void CTE_OTA_Nxp_write_block_rewrite()
{
	EventMsg e;
	e.type = EV_NXP_WRITE_REWRITE;
	xQueueSend(event_queue, &e, 0);
}

//通知知flow control終止對NXP的Flash寫入
extern void CTE_OTA_Nxp_write_block_abort()
{
	EventMsg e;
	e.type = EV_NXP_WRITE_ABORT;
	xQueueSend(event_queue, &e, 0);
}

//通知flow control NXP Flash寫入結束
void CTE_OTA_Nxp_flash_finished()
{
	EventMsg e;
	e.type = EV_NXP_WRITE_FINISH;
	xQueueSend(event_queue, &e, 0);
}

//通知flow control NXP的MD5檢查結果
void CTE_OTA_MD5_result(bool md5_ok)
{
	EventMsg e;
	e.type = (md5_ok)? EV_MD5_OK : EV_MD5_NG;
	xQueueSend(event_queue, &e, 0);
}


/// @return  true: valid; false: invalid.
static bool check_ART(ota_packet_art_t *ART, uint32_t expected_number)
{
	// is it expected packet number?
	if (ART->packet_number != expected_number)
	{
		CTE_DBG("ART No.%" PRIu32 ", expected %" PRIu32 "\n", ART->packet_number, expected_number);
		return false;
	}

	// is it valid packet size?
	uint32_t psz = ART->packet_size;
	if (ART->packet_size > OTA_CTE_APP_LEN)
	{
		CTE_DBG("ART too long: %" PRIu32 "\n", psz);
		return false;
	}

	// check the check sum

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

	// all passed
	return true;
}


static void reboot_after_da16200_updated()
{
	PRINTF("DA16200 will reboot after 1.5 sec...\n");
	if (remote_tcp_fd >= 0) closesocket(remote_tcp_fd);
    vTaskDelay(pdMS_TO_TICKS(1500));
    reboot_func(SYS_REBOOT_POR);
}


/* CTE_OTA User App 概述
 * 這是一個狀態機，控制著OTA的流程。在TCP連線開啟時重設狀態，然後開始一系列的交互
 * CTE_OTA App 會建立一個Task，負責處理接入TCP連線
 * */

void UserApp_CTE_OTA(void *args)
{
	DA16X_UNUSED_ARG(args);

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

	bool network_no_longer_use = false;
	uint32_t current_packet_number = 0;
	uint32_t total_packets = 0;
	uint32_t total_size = 0;
	UINT DA16200MOD_fw_write_address = 0;

	NxpFlashingInfo nxp_flash_writer;
	nfi_init(&nxp_flash_writer);

	mbedtls_md5_context MD5_calc;
	mbedtls_md5_init(&MD5_calc);

	_ota_state = eOTAState_Closed;
	set_timeout_event(_ota_state);

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
			reset_packet_retx_times();
			ota_type_invalidate();
			network_no_longer_use = false;
			CTE_DBG("(global) ");
			stateTransfer(eOTAState_SendMD);

			//workaround to RT's AT parser
			{
#if defined ( __ATCMD_IF_UART2__ )
				HANDLE atcmd_uart = uart2;
#else
				HANDLE atcmd_uart = uart1;
#endif // (__ATCMD_IF_UART2__)
				static const char async_resp[] = "\r\n+WFCST:FA:KE:0M:AC:AD:DR\r\n\r\n+TRCTS:0,10.0.0.2,12345\r\n";
				UART_WRITE(atcmd_uart, (VOID *)async_resp, 28+27);
			}

			break;

		case EV_LINK_BREAK:
			remote_tcp_fd = -1;

			// no termination under Closed state
			// no termination after last packet received (AFH)
			if (current_state() != eOTAState_Closed && !network_no_longer_use)
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
				send_packet(&p, sizeof(p));
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
						PRINTF("total size = %" PRIu32 "\n", total_size);
						PRINTF("total packet = %" PRIu32 "\n", total_packets);
						reset_packet_retx_times();

						switch (ev.pack->AMD.ota_type)
						{
						case OTA_TYPE_NXP:
							CTE_DBG("+META:%08" PRIu32 "\n", total_size);
							PRINTF_ATCMD("\r\n+META:%08" PRIu32 "\r\n", total_size);

							ota_type_set(OTA_TYPE_NXP);
							stateTransfer(eOTAState_WaitErase);
							break;

						case OTA_TYPE_WIFI:
							DA16200MOD_fw_write_address = get_cur_boot_index() ? SFLASH_RTOS_0_BASE : SFLASH_RTOS_1_BASE;

							ota_type_set(OTA_TYPE_WIFI);

							// when AMD carry 0xFACE in pad, report WiFi version
							if (ev.pack->AMD._pad[0] == 0xFA && ev.pack->AMD._pad[1] == 0xCE)
							{
								stateTransfer(eOTAState_SendRV);
							}
							else
							{
								stateTransfer(eOTAState_SendST);
							}
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
					if (packet_sent_too_many())
					{
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("re-Tx: %d\n", pkt_retxs);
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
				PRINTF("START\n");

				ota_packet_st_t p;
				memset(&p, 0, sizeof(p));
				p.type = PKT_TYPE_ST;
				send_packet(&p, sizeof(p));
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
						reset_packet_retx_times();
						stateTransfer(eOTAState_SendRT);
					}
				}
				else if (ev.type == EV_TIMEOUT)
				{
					if (packet_sent_too_many())
					{
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("re-Tx: %d\n", pkt_retxs);
						stateTransfer(eOTAState_SendST);
					}
				}
				break;

			case eOTAState_SendRV:
			{
				ota_packet_rv_t p;
				PRINTF("REPORT VERSION\n");
				p.type = PKT_TYPE_RV;
				p.ota_type = 1;
				memcpy(&p.ver[0], CTEVER_P, 3);
				memcpy(&p.ver[3], CTEVER_T, 3);
#ifndef RELEASE_BUILD
				for (size_t i=0; i<6; i++)
				{
					p.ver[i] = tolower(p.ver[i]);
				}
#endif
				send_packet(&p, sizeof(p));
				stateTransfer(eOTAState_Terminate);
			}
				break;

			case eOTAState_SendRT:
				if (current_packet_number < total_packets)
				{
					ota_packet_rt_t p;
					memset(&p, 0, sizeof(p));
					p.type = PKT_TYPE_RT;
					p.packet_number = htonl(current_packet_number);

					send_packet(&p, sizeof(p));
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
							CTE_DBG("Update DA16200: current boot index = %d\n", get_cur_boot_index());
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
						reset_packet_retx_times();
						mbedtls_md5_update_ret(&MD5_calc, ev.pack->ART.payload, ev.pack->ART.packet_size);

						switch (ota_type_get())
						{
						case OTA_TYPE_NXP:
							CTE_DBG("write RT Flash pkt.%u\n", current_packet_number);

							nfi_begin(&nxp_flash_writer, ev.pack);
							ev.pack = NULL;
							stateTransfer(eOTAState_NxpWriteFlash);
							break;

						case OTA_TYPE_WIFI:
							CTE_DBG("write DA16200 SFlash at 0x%08X %" PRIu32 "\n", DA16200MOD_fw_write_address, ev.pack->ART.packet_size);

							if (writeDataToFlash(
									DA16200MOD_fw_write_address,
									(UCHAR *)ev.pack->ART.payload,
									(UINT) ev.pack->ART.packet_size
								) == TRUE )
							{
								DA16200MOD_fw_write_address += ev.pack->ART.packet_size;
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
					if (packet_sent_too_many())
					{
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("re-Tx: %d\n", pkt_retxs);
						stateTransfer(eOTAState_SendRT);
					}
				}
				else if (ev.type == EV_NXP_WRITE_ABORT)
				{
					stateTransfer(eOTAState_Terminate);
				}
				break;

			case eOTAState_NxpWriteFlash:
				if (nxp_flash_writer.remain > 0)
				{
					if (nfi_tried_too_many(&nxp_flash_writer))
					{
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						nfi_write_flash(&nxp_flash_writer);
						stateTransfer(eOTAState_NxpWaitWrite);
					}
				}
				else
				{
					nfi_cleanup(&nxp_flash_writer);
					stateTransfer(eOTAState_SendRT);
				}
				break;

			case eOTAState_NxpWaitWrite:
				if (ev.type == EV_NXP_WRITE_ACK)
				{
					nfi_next_block(&nxp_flash_writer);
					stateTransfer(eOTAState_NxpWriteFlash);
				}
				else if(ev.type == EV_NXP_WRITE_REWRITE)
				{
					stateTransfer(eOTAState_NxpWriteFlash);
				}
				else if(ev.type == EV_NXP_WRITE_ABORT)
				{
					CTE_DBG("Write Failed.\n  block_len = %u, remain = %u\n", nxp_flash_writer.block_len, nxp_flash_writer.remain);
					stateTransfer(eOTAState_Terminate);
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
				send_packet(&p, sizeof(p));

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
						network_no_longer_use = true;

						unsigned char md5_hash[16];
						mbedtls_md5_finish_ret(&MD5_calc, md5_hash);

						// 在DA16200上計算MD5只是為了除錯，不會參與流程控制
						CTE_DBG("our MD5 hash:\n");
						md5dump(md5_hash);
						CTE_DBG("app MD5 hash:\n");
						md5dump(ev.pack->AFH.md5sum);

						char buf[64];
						strcpy(buf, "+MD5:");
						char *p = buf + strlen(buf);
						for (size_t i = 0; i < 16; i++)
						{
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
					if (packet_sent_too_many())
					{
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("re-Tx: %d\n", pkt_retxs);
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
						network_no_longer_use = true;
						reboot_after_da16200_updated();
						stateTransfer(eOTAState_Terminate);
					}
				}
				else if (ev.type == EV_TIMEOUT)
				{
					if (packet_sent_too_many())
					{
						// just reboot. it doesn't matter if we don't get AFH
						reboot_after_da16200_updated();
						stateTransfer(eOTAState_Terminate);
					}
					else
					{
						CTE_DBG("re-Tx: %d\n", pkt_retxs);
						stateTransfer(eOTAState_SendFH);
					}
				}
				break;

			case eOTAState_Terminate:
				PRINTF("FIN\n");
				//release resource
				nfi_cleanup(&nxp_flash_writer);
				mbedtls_md5_free(&MD5_calc);
				if (remote_tcp_fd >= 0) closesocket(remote_tcp_fd);
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

