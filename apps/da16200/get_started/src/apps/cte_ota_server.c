#include <inttypes.h>
#include "sdk_type.h"
#include "user_cte.h"
#include <assert.h>



#ifdef ENABLE_CTE_DBG

static void _u8_hex_string(char s[3], uint8_t value)
{
	const char *glyph = "0123456789ABCDEF";
	s[0] = glyph[(value >> 4) & 15];
	s[1] = glyph[value & 15];
	s[2] = '\0';
}

#define GENERIC_SMALL_ENUM_DBG_TEXT(lim, txt) do { \
	if (e < lim) { \
		return txt[e]; \
	} else { \
		static char buf[3]; \
		_u8_hex_string(buf, e); \
		return buf; \
	} } while(0)

#endif	/* ENABLE_CTE_DBG */



////////////////////////////////////////////////////////////////////////////////

// MCUBOOT abstraction

static void write_data_packet_to_MCU(const void *data, uint32_t length)
{
	nxp_mcuboot_send_data_packet(data, length);
}

static void acknowledge_finish(void)
{
	nxp_mcuboot_send_ack();
}


////////////////////////////////////////////////////////////////////////////////

//John_9999 command start
#define PTYPE_GetMeta			0x20
#define PTYPE_AnswerMeta		0x21
#define PTYPE_RequestData		0x22
#define PTYPE_DataPackage		0x23
#define PTYPE_FIN				0x24
#define PTYPE_DISCON            0x25  //JJR
#define PTYPE_KeepAlive			0x30

#define OTA_TYPE_MAIN		0
#define OTA_TYPE_WIFI		1
#define OTA_TYPE_BT         2
#define OTA_TYPE_BLE        3
#define OTA_TYPE_EXIT       4
//John_9999 command end


static bool OneButton = false; //JJR
typedef uint8_t PacketType_t;
typedef uint8_t OtaType_t;
typedef uint16_t BodyLen_t;



typedef struct _packet_header
{
	PacketType_t ptype;
	OtaType_t ota_type;
	BodyLen_t body_length;

} OTA_packet_header_t;


typedef struct _packet_body_GetMeta
{
    uint8_t wifi_firmware_version[3];

} OTA_packet_body_GetMeta_t;


typedef struct _packet_body_AnswerMeta
{
    uint32_t total_size;
    uint32_t total_data_packages;
    uint8_t md5[16];

} OTA_packet_body_AnswerMeta_t;


typedef struct _packet_body_RequestData
{
	uint32_t data_package_number;

} OTA_packet_body_RequestData_t;


typedef struct _packet_body_DataPackage
{
	uint32_t data_package_number;
	uint8_t data[OTA_CTE_APP_LEN];

} OTA_packet_body_DataPackage_t;

// 改 OTA_packet_body_DataPackage_t 的時候要記得改這個啊
#define DATAPACKAGE_DATA_OFFSET			4

// 改 OTA_packet_body_DataPackage_t 的時候這個不用動
#define DATAPACKAGE_BODY_MINIMUM_LENGTH	(DATAPACKAGE_DATA_OFFSET + 1)


typedef struct _packet_body_FIN
{
	uint8_t result_code;

} OTA_packet_body_FIN_t;



////////////////////////////////////////////////////////////////////////////////

// OTA Update Result definition

#define ENUMERATION__OTA_UPDATE_RESULT \
	ENTRIES(DONE, 0), \
	ENTRIES(MD5_ERROR, 1), \
	ENTRIES(MCU_NO_ANS, 2), /* 警告：MCU_NO_ANS比較特別，轉移到它一定時間後DA16200就會向MCU發出一次RST訊號 */ \
	ENTRIES(ERASE_FAIL, 3), \
	ENTRIES(WIFI_FW_CRC, 4), \
	ENTRIES(UNKNOWN_OTA_TYPE, 5), \
	ENTRIES(BAD_METADATA, 6), \
	ENTRIES(BUFFER_OVERRUN, 7), \
	ENTRIES(INCONSIST_OTA_TYPE, 8), \
	ENTRIES(WIFI_UPDATE_TIMEOUT, 9), \
	ENTRIES(DEBUG_2, 126), \
	ENTRIES(DEBUG_1, 127), \
	ENTRIES(MCU_MALFORMED, 162), \
	ENTRIES(MCU_CRC_ERR, 163), \
	ENTRIES(MCU_LOSS, 254), \
	ENTRIES(MCU_TIMEOUT, 255), \
// leave this line blank


enum OtaUpdateResultEnum
{
#define ENTRIES(x, y)   OTA_RESULT_ ## x
	ENUMERATION__OTA_UPDATE_RESULT
	ENUM_COUNT_OTA_UPDATE_RESULT
#undef ENTRIES
};
typedef enum OtaUpdateResultEnum OTAUpdateResult_t;


static const uint8_t ota_update_result_code[ENUM_COUNT_OTA_UPDATE_RESULT] =
{
#define ENTRIES(x, y)    y
	ENUMERATION__OTA_UPDATE_RESULT
#undef ENTRIES
};


#ifdef ENABLE_CTE_DBG

static const char *txt_ota_update_result[ENUM_COUNT_OTA_UPDATE_RESULT] =
{
#define ENTRIES(x, y)    #x
	ENUMERATION__OTA_UPDATE_RESULT
#undef ENTRIES
};

static const char *strOtaResult(uint8_t e)
{
	GENERIC_SMALL_ENUM_DBG_TEXT(ENUM_COUNT_OTA_UPDATE_RESULT, txt_ota_update_result);
}

#endif	/* ENABLE_CTE_DBG */



////////////////////////////////////////////////////////////////////////////////

// OTA Update Automata Event definition

#define ENUMERATION__OTA_FSM_EVENT \
	ENTRIES(ENTER), \
	ENTRIES(TIMEOUT), \
	ENTRIES(LINK_START), \
	ENTRIES(LINK_BREAK), \
	ENTRIES(RECV_METADATA), \
	ENTRIES(ERASE_OK), \
	ENTRIES(ERASE_FAIL), \
	ENTRIES(RECV_DATAPACK), \
	ENTRIES(NXP_WRITE_ACK), \
	ENTRIES(NXP_WRITE_ABORT), \
	ENTRIES(NXP_WRITE_FINISH), \
	ENTRIES(MD5_OK), \
	ENTRIES(MD5_NG), \
	ENTRIES(KEEPALIVE), \
	ENTRIES(DASHBOARD_RECOVERY), \
	ENTRIES(RESET_AUTOMATA), \
	ENTRIES(BAD_OTA_TYPE), \
// leave this line blank


enum OTAutomataEvent
{
#define ENTRIES(x)   EV_ ## x
	ENUMERATION__OTA_FSM_EVENT
	ENUM_COUNT_OTA_FSM_EVENT
#undef ENTRIES
};
typedef enum OTAutomataEvent EventType;


#ifdef ENABLE_CTE_DBG

static const char *txt_ota_event[ENUM_COUNT_OTA_FSM_EVENT] =
{
#define ENTRIES(x)   "ev(" #x ")"
	ENUMERATION__OTA_FSM_EVENT
#undef ENTRIES
};

static const char *strOtaEvent(uint8_t e)
{
	GENERIC_SMALL_ENUM_DBG_TEXT(ENUM_COUNT_OTA_FSM_EVENT, txt_ota_event);
}

#endif	/* ENABLE_CTE_DBG */



////////////////////////////////////////////////////////////////////////////////

// OTA Update Automata State definition

#define ENUMERATION__OTA_FSM_STATE \
	ENTRIES(INIT), \
	ENTRIES(GetMetadata), \
	ENTRIES(WaitErase), \
	ENTRIES(MCU_Programing), \
	ENTRIES(MCU_Verification), \
	ENTRIES(WIFI_Programing), \
	ENTRIES(BT_Programing), \
	ENTRIES(BLE_Programing), \
	ENTRIES(DONE), \
	ENTRIES(Terminated), \
// leave this line blank


enum OTAutomataStates
{
#define ENTRIES(x)   eOTAState_ ## x
	ENUMERATION__OTA_FSM_STATE
	ENUM_COUNT_OTA_FSM_STATE
#undef ENTRIES
};
typedef enum OTAutomataStates OTA_FSM_State;


#ifdef ENABLE_CTE_DBG

static const char *txt_ota_state[ENUM_COUNT_OTA_FSM_STATE] =
{
#define ENTRIES(x)   "OTAState_" #x
	ENUMERATION__OTA_FSM_STATE
#undef ENTRIES
};

static const char *strOtaState(uint8_t e)
{
	GENERIC_SMALL_ENUM_DBG_TEXT(ENUM_COUNT_OTA_FSM_STATE, txt_ota_state);
}

#endif



////////////////////////////////////////////////////////////////////////////////

struct OTAUpdateMetaData
{
	bool ready;

	OtaType_t ota_type;
	uint32_t total_size;
	uint32_t total_packages;
	uint8_t md5[16];
};

static struct OTAUpdateMetaData metadata;


static void initialize_ota_update_metadata(void)
{
	metadata.ready = false;
	metadata.ota_type = OTA_TYPE_MAIN;
}


static bool metadata_is_ready(void)
{
	return metadata.ready;
}


static void write_ota_update_metadata(OtaType_t ota_type, uint32_t total_size, uint32_t total_packages, uint8_t md5[16])
{
	if (metadata.ready)
	{
		return;
	}

	metadata.ota_type = ota_type;
	metadata.total_size = total_size;
	metadata.total_packages = total_packages;
	memcpy(metadata.md5, md5, sizeof(metadata.md5));

	metadata.ready = true;
}



////////////////////////////////////////////////////////////////////////////////

// data structure for DataPackage's body

struct DataPackage
{
	OTA_packet_body_DataPackage_t body;
	BodyLen_t data_len;
};

typedef struct
{
	struct DataPackage *ptr;
} DataPackageBuffer;


static void DPB_init(DataPackageBuffer *_this)
{
	_this->ptr = NULL;
}

static bool DPB_is_null(const DataPackageBuffer *_this)
{
	return _this->ptr == NULL;
}

static void DPB_set_null(DataPackageBuffer *_this)
{
	_this->ptr = NULL;
}

static OTA_packet_body_DataPackage_t* DPB_get_body(DataPackageBuffer *_this)
{
	return &_this->ptr->body;
}

static BodyLen_t DPB_data_length(const DataPackageBuffer *_this)
{
	return _this->ptr->data_len;
}

static void DPB_set_data_length(DataPackageBuffer *_this, BodyLen_t len)
{
	_this->ptr->data_len = len;
}

/// 傳入的DataPackageBuffer物件放棄它的記憶體控制權，並轉交給出現在回傳值中的物件
/// 必須要有另一個DataPackageBuffer物件承接DPB_move()回傳值，否則就會記憶體洩漏!
/// 概念有點像C++11的std::move()，所以這樣命名
static DataPackageBuffer DPB_move(DataPackageBuffer *_this)
{
	DataPackageBuffer tmp;
	tmp.ptr = _this->ptr;
	_this->ptr = NULL;
	return tmp;
}



////////////////////////////////////////////////////////////////////////////////

// Memory pool for DataPackage's body

struct PoolUnit
{
	struct PoolUnit *next;
	union {
		struct DataPackage dp;
	} memory;
};

#define DATA_PACKAGE_BUFFER_POOL_NUM	4
static struct PoolUnit response_buffer_pool[DATA_PACKAGE_BUFFER_POOL_NUM];
static struct PoolUnit *pool_head;
static SemaphoreHandle_t mutex_mem_pool;


static void initialize_DataPackageBuffer_pool(void)
{
	for (size_t i = 0; i < DATA_PACKAGE_BUFFER_POOL_NUM-1; i++)
	{
		response_buffer_pool[i].next = &response_buffer_pool[i+1];
	}
	response_buffer_pool[DATA_PACKAGE_BUFFER_POOL_NUM-1].next = NULL;
	pool_head = &response_buffer_pool[0];

	static StaticQueue_t instance;
	mutex_mem_pool = xSemaphoreCreateMutexStatic(&instance);
}


static DataPackageBuffer allocate_DataPackageBuffer(void)
{
	DataPackageBuffer buf;

	xSemaphoreTake(mutex_mem_pool, portMAX_DELAY);

	if (pool_head == NULL)	// no available unit
	{
		CTE_DBG_FATAL("run out of memory pool\n");
		buf.ptr = NULL;
	}
	else
	{
		buf.ptr = (struct DataPackage *)&pool_head->memory;

		struct PoolUnit *next = pool_head->next;
		pool_head = next;
	}

	xSemaphoreGive(mutex_mem_pool);

	return buf;
}


static void release_DataPackageBuffer(DataPackageBuffer *pBuf)
{
	if (DPB_is_null(pBuf)) return;

	xSemaphoreTake(mutex_mem_pool, portMAX_DELAY);
	struct PoolUnit **base = (struct PoolUnit **) pBuf->ptr;
	base[-1] = pool_head;
	pool_head = (struct PoolUnit *)&base[-1];
	xSemaphoreGive(mutex_mem_pool);

	DPB_set_null(pBuf);
}



////////////////////////////////////////////////////////////////////////////////

// class PacketCollectHelper: collect a complete Control Packet
typedef struct _PackCollector
{
	uint8_t *front;
	ssize_t expect;

} PacketCollectHelper;

// initialize PackCollector structure
static void pch_init(PacketCollectHelper *_this)
{
	_this->front = NULL;
	_this->expect = 0;
}

// set the expected receive data length, and receiving buffer address
static void pch_expected_length(PacketCollectHelper *_this, void *buffer, ssize_t expected_length)
{
	_this->front = ((uint8_t *)buffer);
	_this->expect = expected_length;
}

/// @param [out] p_recv_len : equal to recv() return value
/// @return true if expected length is received
static bool pch_receive(PacketCollectHelper *_this, int sock, ssize_t *p_recv_len)
{
	ssize_t recv_len = recv(sock, _this->front, _this->expect, 0);
	*p_recv_len = recv_len;

	if (recv_len > 0)
	{
		_this->expect -= recv_len;
		_this->front += recv_len;

		if (_this->expect <= 0)
		{
			return true;
		}
	}

	return false;
}



////////////////////////////////////////////////////////////////////////////////


//事件的資料結構
typedef struct
{
	EventType type;

	union
	{
		DataPackageBuffer data_package_buffer;
		int sock_client;
		uint8_t nxp_write_err_code;
	};

} Event_t;


static QueueHandle_t queue_socket;
static QueueHandle_t event_queue;


static void OTAutomata_event_TCP_connected(int sock_client)
{
	Event_t e;
	e.type = EV_LINK_START;
	e.sock_client = sock_client;
	xQueueSend(event_queue, &e, 0);
}


static void OTAutomata_event_close_connection(int sock_client)
{
	Event_t e;
	e.type = EV_LINK_BREAK;
	e.sock_client = sock_client;
	xQueueSend(event_queue, &e, 0);
}


static void OTAutomata_event_recv_metadata(void)
{
	Event_t e;
	e.type = EV_RECV_METADATA;
	xQueueSend(event_queue, &e, 0);
}


static void OTAutomata_event_recv_datapack(DataPackageBuffer data_package_buffer)
{
	Event_t e;
	e.type = EV_RECV_DATAPACK;
	e.data_package_buffer = data_package_buffer;
	xQueueSend(event_queue, &e, 0);
}


static void OTAutomata_event_keepalive_timeout(TimerHandle_t xTimer)
{
	DA16X_UNUSED_ARG(xTimer);

	Event_t e;
	e.type = EV_KEEPALIVE;
	xQueueSend(event_queue, &e, 0);
}


static void OTAutomata_event_inconsistent_ota_type(void)
{
	Event_t e;
	e.type = EV_BAD_OTA_TYPE;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_Nxp_erase_result(bool erase_ok)
{
	Event_t e;
	e.type = (erase_ok)? EV_ERASE_OK : EV_ERASE_FAIL;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_Nxp_write_block_ok(void)
{
	Event_t e;
	e.type = EV_NXP_WRITE_ACK;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_Nxp_write_block_abort(uint8_t err_code)
{
	Event_t e;
	e.type = EV_NXP_WRITE_ABORT;
	e.nxp_write_err_code = err_code;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_Nxp_flash_finished(void)
{
	Event_t e;
	e.type = EV_NXP_WRITE_FINISH;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_Nxp_MD5_check_result(bool md5_ok)
{
	Event_t e;
	e.type = (md5_ok)? EV_MD5_OK : EV_MD5_NG;
	xQueueSend(event_queue, &e, 0);
}


void CTE_Enter_Dashboard_Recovery_Mode(void)
{
	Event_t e;
	e.type = EV_DASHBOARD_RECOVERY;
	xQueueSend(event_queue, &e, 0);
}


void CTE_OTA_restart(void)
{
	Event_t e;
	e.type = EV_RESET_AUTOMATA;
	xQueueSend(event_queue, &e, 0);
}



////////////////////////////////////////////////////////////////////////////////

// class NxpFlashingInfo: information structure for flashing NXP ROM

typedef struct _NxpFlashingInfo
{
	uint32_t remain;
	uint8_t *source;
	uint32_t block_len;
	DataPackageBuffer data_package_buffer;

} NxpFlashingInfo;

// construct NxpFlashingInfo object
static void nfi_init(NxpFlashingInfo *_this)
{
	_this->remain = 0;
	_this->source = NULL;
	_this->block_len = 0;
	DPB_init(&_this->data_package_buffer);
}

// 開始這份 DataPackage 的寫入
static void nfi_begin(NxpFlashingInfo *_this, DataPackageBuffer buf)
{
	_this->remain = DPB_data_length(&buf);
	_this->source = DPB_get_body(&buf)->data;
	_this->block_len = 0;
	_this->data_package_buffer = buf;
}

// 主要是清除手上還有的 DataPackageBuffer
static void nfi_cleanup(NxpFlashingInfo *_this)
{
	release_DataPackageBuffer(&_this->data_package_buffer);
}

// 向MCU寫入(最多OTA_CTE_NXP_LEN bytes)
// @return 沒有資料可寫時回傳false，並自動呼叫nfi_cleanup()
static bool nfi_write_flash(NxpFlashingInfo *_this)
{
	if (_this->remain > 0)
	{
		//限制每次送出的長度
		_this->block_len = configMIN(_this->remain, OTA_CTE_NXP_LEN);

		write_data_packet_to_MCU(_this->source, _this->block_len);
		return true;
	}
	else
	{
		nfi_cleanup(_this);
		return false;
	}
}

// 移動指標到下次要送出的資料
static void nfi_next_block(NxpFlashingInfo *_this)
{
	_this->source += _this->block_len;
	_this->remain -= _this->block_len;
	CTE_DBG("remain = %" PRIu32 "\n", _this->remain);
}



////////////////////////////////////////////////////////////////////////////////

// TIMEOUT event functions

static TickType_t toe_from = 0;
static TickType_t toe_period = portMAX_DELAY;

static void set_timeout_event(TickType_t period)
{
	toe_from = xTaskGetTickCount();
	toe_period = period;
}

static void unset_timeout_event(void)
{
	toe_period = portMAX_DELAY;
}

//決定等待事件佇列要等多久
static TickType_t calc_TicksToWait(void)
{
	if (toe_period == portMAX_DELAY)
	{
		return portMAX_DELAY;
	}

	TickType_t elapsed = xTaskGetTickCount() - toe_from;
	return (toe_period > elapsed)? (toe_period - elapsed) : 0;
}



////////////////////////////////////////////////////////////////////////////////


static OTA_FSM_State g_ota_server_state;
static bool g_trigger_enter_state_event;
static OTAUpdateResult_t g_ota_result;
static bool g_waiting_response;
static bool g_pending_keep_alive;

static int sock_remote_app;

static TimerHandle_t keep_alive_timer;

#ifdef ENABLE_CTE_DBG
static uint32_t profiling_total_tcp_connect_times = 0;
static TickType_t profiling_OTA_start_time = 0;

static void ota_profiling_report(void)
{
	CTE_DBG(YELLOW_COLOR "[END] " CLEAR_COLOR "OTA Update result: %s\n", strOtaResult(g_ota_result));
	CTE_DBG(YELLOW_COLOR "[END] " CLEAR_COLOR "Total Connections: %" PRIu32 "\n", profiling_total_tcp_connect_times);
	CTE_DBG(YELLOW_COLOR "[END] " CLEAR_COLOR "Elapsed Time: %" PRIu32 "s\n", (xTaskGetTickCount() - profiling_OTA_start_time) / 100);
}
#endif



static void init_automata(void)
{
	PRINTF("RESET OTA PROCESS\n");

	g_ota_server_state = eOTAState_INIT;
	g_trigger_enter_state_event = false;
	g_ota_result = OTA_RESULT_DEBUG_1;
	g_waiting_response = false;
	g_pending_keep_alive = false;
	if (sock_remote_app >= 0)
	{
		closesocket(sock_remote_app);
		sock_remote_app = -1;
	}

	xTimerStop(keep_alive_timer, pdMS_TO_TICKS(10));

	unset_timeout_event();

	initialize_ota_update_metadata();
}

static void init_automata_OneButton(void) //JJR
{
	PRINTF("RESET OTA PROCESS\n");

	g_ota_server_state = eOTAState_GetMetadata;
	g_trigger_enter_state_event = false;
	g_ota_result = OTA_RESULT_DEBUG_1;
	g_waiting_response = false;
	g_pending_keep_alive = false;
/*
	if (sock_remote_app >= 0)
	{
		closesocket(sock_remote_app);
		sock_remote_app = -1;
	}

	xTimerStop(keep_alive_timer, pdMS_TO_TICKS(10));
*/
	unset_timeout_event();

	initialize_ota_update_metadata();

}


static void reset_keep_alive_timer(void)
{
	xTimerStart(keep_alive_timer, portMAX_DELAY);
	g_pending_keep_alive = false;
}


static void response_received(void)
{
	g_waiting_response = false;
}


static void stateTransfer(OTA_FSM_State new_st)
{
	CTE_DBG_PRINT_TS();
	CTE_DBG("%s -> %s\n", strOtaState(g_ota_server_state), strOtaState(new_st));

	g_ota_server_state = new_st;
	g_trigger_enter_state_event = true;
	unset_timeout_event();
}


static void Terminate(enum OtaUpdateResultEnum result)
{
	g_ota_result = result;
	stateTransfer(eOTAState_Terminated);
}


static void send_control_packet(int sock, void *data, size_t len, bool wait_resp)
{
	CTE_DBG_PRINT_TS();
	CTE_DBG("send %02Xh\n", *(uint8_t*)data);

	if (sock < 0)
	{
		CTE_DBG("> cancel: no connection\n");
		return;
	}

	memory_dump(data, len);

	if (send(sock, data, len, 0) != (ssize_t)len)
	{
		CTE_DBG("send error: %d\n", errno);
		OTAutomata_event_close_connection(sock);
		return;
	}

	reset_keep_alive_timer();

	if (wait_resp)
	{
		g_waiting_response = true;
	}
}


// if OTAType < 0, FILL_HEADER() will get it from metadata
#define FILL_HEADER(Header, PType, BodyLength) do { \
		Header.ptype = PType; \
		Header.ota_type = metadata.ota_type; \
		Header.body_length = htons(BodyLength); \
} while(0)


static void send_KeepAlive(void)
{
	struct {
		OTA_packet_header_t header;
	} packet;

	_Static_assert( sizeof(packet) == 4, "Size Error");

	FILL_HEADER(packet.header, PTYPE_KeepAlive, 0);

	send_control_packet(sock_remote_app, &packet, sizeof(packet), false);
}


static void send_GetMeta(void)
{
#pragma pack(push, 1)
	struct {
		OTA_packet_header_t header;
		OTA_packet_body_GetMeta_t body;
	} packet;
#pragma pack(pop)

	_Static_assert( sizeof(packet) == 7, "Size Error");

	FILL_HEADER(packet.header, PTYPE_GetMeta, sizeof(packet.body));

	memcpy(packet.body.wifi_firmware_version, CTEVER_P, 3);

	send_control_packet(sock_remote_app, &packet, sizeof(packet), true);
}


static void send_RequestData(uint32_t data_package_number)
{
#pragma pack(push, 1)
	struct {
		OTA_packet_header_t header;
		OTA_packet_body_RequestData_t body;
	} packet;
#pragma pack(pop)

	_Static_assert( sizeof(packet) == 8, "Size Error");

	FILL_HEADER(packet.header, PTYPE_RequestData, sizeof(packet.body));

	packet.body.data_package_number = htonl(data_package_number);

	send_control_packet(sock_remote_app, &packet, sizeof(packet), true);
}


static void send_FIN(void) 
{
#pragma pack(push, 1)
	struct {
		OTA_packet_header_t header;
		OTA_packet_body_FIN_t body;
	} packet;
#pragma pack(pop)

	_Static_assert( sizeof(packet) == 5, "Size Error");

	FILL_HEADER(packet.header, PTYPE_FIN, sizeof(packet.body));

	packet.body.result_code = ota_update_result_code[g_ota_result];

	send_control_packet(sock_remote_app, &packet, sizeof(packet), false);
}

static void send_DISCON(void) /*JJR create*/
{
#pragma pack(push, 1)
	struct {
		OTA_packet_header_t header;
	} packet;
#pragma pack(pop)

	_Static_assert( sizeof(packet) == 4, "Size Error");

	FILL_HEADER(packet.header, PTYPE_DISCON, 0);

	send_control_packet(sock_remote_app, &packet, sizeof(packet), false);
}

static void output_AT_style_async_msg(char *s)
{
	CTE_DBG("%s\n", s);
	PRINTF_ATCMD("\r\n%s\r\n", s);
}


static void write_fw_size(void)
{
	//workaround for MainProgram's AT parser
#if 1
	PRINTF_ATCMD("\r\n+WFCST:FA:KE:0M:AC:AD:DR\r\n\r\n+TRCTS:0,10.0.0.2,12345\r\n");
	vTaskDelay(pdMS_TO_TICKS(150));
#endif

	char buf[64];
	/*
		snprintf(buf, sizeof(buf), "+META:%08" PRIu32, metadata.total_size);//JJR
	*/
    if(metadata.ota_type == OTA_TYPE_MAIN) //JJR
	{
		snprintf(buf, sizeof(buf), "+METAa%08" PRIu32, metadata.total_size);
	}
	else if(metadata.ota_type == OTA_TYPE_WIFI)
	{
		snprintf(buf, sizeof(buf), "+METAb%08" PRIu32, metadata.total_size);
	}
	else if(metadata.ota_type == OTA_TYPE_BT)
	{
		snprintf(buf, sizeof(buf), "+METAc%08" PRIu32, metadata.total_size);
	}
	else if(metadata.ota_type == OTA_TYPE_BLE)
	{
		snprintf(buf, sizeof(buf), "+METAd%08" PRIu32, metadata.total_size);
	}
	output_AT_style_async_msg(buf);
}


static void write_MD5(void)
{
	char buf[64];
	char *p = buf + sprintf(buf, "+MD5:");
	for (size_t i = 0; i < 16; i++)
	{
		sprintf(p, "%02x", metadata.md5[i]);
		p += 2;
	}
	output_AT_style_async_msg(buf);
}


static void handle_nxp_write_abort(uint8_t nxp_write_err_code) //John_9999 what is this?
{
	switch (nxp_write_err_code)
	{
	case 0xA2:
		Terminate(OTA_RESULT_MCU_MALFORMED);
		break;

	case 0xA3:
		Terminate(OTA_RESULT_MCU_CRC_ERR);
		break;

	case 0xFE:
		Terminate(OTA_RESULT_MCU_LOSS);
		break;

	case 0xFF:
		Terminate(OTA_RESULT_MCU_TIMEOUT);
		break;

	default:
		CTE_DBG_FATAL("unknown nxp_write_err_code: %02X\n", nxp_write_err_code);
	}
}


static bool check_packages_amount(void)
{
	uint32_t pkg = metadata.total_size / OTA_CTE_APP_LEN;
	if (metadata.total_size % OTA_CTE_APP_LEN != 0)
	{
		pkg++;
	}

	return pkg == metadata.total_packages;
}


static UINT get_new_firmware_address(void)
{
	if (get_cur_boot_index() == 0)
	{
		// current boot_idx = 0, choose RTOS Area 1
		return SFLASH_RTOS_1_BASE;
	}
	else
	{
		// current boot_idx = 1, choose RTOS Area 0
		return SFLASH_RTOS_0_BASE;
	}
}


static void Task_OtaUpdateAutomata(void *args)
{
	DA16X_UNUSED_ARG(args);

	uint32_t req_package_number = 0;
	uint32_t ack_packages = 0;

	mbedtls_md5_context MD5_calc;
	mbedtls_md5_init(&MD5_calc);

	DataPackageBuffer dpack_buffer;
	DPB_init(&dpack_buffer);

	NxpFlashingInfo nxp_flash_writer;
	nfi_init(&nxp_flash_writer);

	UINT DA16200MOD_fw_write_address = (UINT)0xFFFFFFFF;	// set to a unavailable address

	sock_remote_app = -1;
	init_automata();

	PRINTF("John_9999...OTA TASK initial\n");
	if (CTE_RECOVER_need_check_dashboard_alive()) {
		PRINTF("check MCU alive...\n");
		bool dashboard_alive = false;
		for (int i = 0; i < CTE_RECOVER_TOTAL_POLLING_ON_STARTUP; i++) {
			vTaskDelay(pdMS_TO_TICKS(CTE_RECOVER_POLLING_INTERVAL));

			if (CTE_NXP_ALIVE()) {
				dashboard_alive = true;
				CTE_DBG_PRINT_TS();
				CTE_DBG("OTAutomata: detect alive\n");
				break;
			} else {
				CTE_DBG_PRINT_TS();
				CTE_DBG("OTAutomata: detecting...\n");
			}
		}

		if (dashboard_alive) {
			PRINTF("remove RECOVER flag\n");
			CTE_RECOVER_clear_flag();
		} else {
			PRINTF("activate Dashboard Recovery Mode from NVM\n");
			CTE_Enter_Dashboard_Recovery_Mode();
		}
	}

	PRINTF("John_9999...Get in OTA loop\n");
	while (1) { //John_9999 enter OTA loop
		// get event
		Event_t ev;
		if (g_trigger_enter_state_event) {
			g_trigger_enter_state_event = false;
			ev.type = EV_ENTER;
		}else if (g_pending_keep_alive && !g_waiting_response) {
			g_pending_keep_alive = false;
			ev.type = EV_KEEPALIVE;
		}else if (xQueueReceive(event_queue, &ev, calc_TicksToWait()) != pdTRUE ) {
			ev.type = EV_TIMEOUT;
			unset_timeout_event();	//停止timeout事件，讓它成為single-shot timer
		}

		{
			CTE_DBG_PRINT_TS();
			CTE_DBG("%s\n", strOtaEvent(ev.type));
		}

		// common event handling
		if (ev.type == EV_DASHBOARD_RECOVERY) {
			dashboard_recovery_mode_state_machine();
			init_automata();
			continue;
		} else if (ev.type == EV_RESET_AUTOMATA) {
			init_automata();
			continue;
		} else if (ev.type == EV_LINK_START) {
            #ifdef ENABLE_CTE_DBG
				profiling_total_tcp_connect_times++;
            #endif
			if (sock_remote_app >= 0) {
				CTE_DBG("close old connection\n");
				closesocket(sock_remote_app);
				sock_remote_app = -1;
			}
			sock_remote_app = ev.sock_client;
			g_waiting_response = false;
			if (xQueueSend(queue_socket, &sock_remote_app, 0) != pdTRUE) {
				CTE_DBG_FATAL("queue_socket full!!\n");
				closesocket(sock_remote_app);
				sock_remote_app = -1;
				continue;
			}
			reset_keep_alive_timer();
		} else if (ev.type == EV_LINK_BREAK) {
			// check EV_LINK_BREAK is the current socket
			if (ev.sock_client == sock_remote_app) {
				xTimerStop(keep_alive_timer, pdMS_TO_TICKS(10));
				CTE_DBG("close socket\n");
				closesocket(sock_remote_app);
				sock_remote_app = -1;
			} else {
				CTE_DBG("LINK_BREAK on old socket %d, current = %d\n", ev.sock_client, sock_remote_app);
			}
		} else if (ev.type == EV_KEEPALIVE) {
			if (g_waiting_response) {
				g_pending_keep_alive = true;
				CTE_DBG("pending KeepAlive\n");
			} else {
				send_KeepAlive();
			}
		} else if (ev.type == EV_BAD_OTA_TYPE) {
			if (g_ota_server_state != eOTAState_Terminated) {
				Terminate(OTA_RESULT_INCONSIST_OTA_TYPE);
			}
		}
		// event handling according to state
	
		switch (g_ota_server_state) {//John_9999 OTA state
			case eOTAState_INIT: {
		    	if (ev.type == EV_LINK_START) {
					CTE_DBG("JJR INIT INSIDE\n");//JJR
		        	release_DataPackageBuffer(&dpack_buffer);
		        	nfi_cleanup(&nxp_flash_writer);
					#ifdef ENABLE_CTE_DBG
		        	profiling_total_tcp_connect_times = 1;
		        	profiling_OTA_start_time = xTaskGetTickCount();
					#endif
		        	stateTransfer(eOTAState_GetMetadata);
		    	}
			} break;
			case eOTAState_GetMetadata: {
		
				if(OneButton) {//JJR
					OneButton = false;
					send_GetMeta(); 
					break;
				} 

		    	if (ev.type == EV_ENTER) {
		        	send_GetMeta();
		    	}
		    	if (ev.type == EV_LINK_START) {
		        	send_GetMeta();
		    	} else if (ev.type == EV_RECV_METADATA) {
		        	if (metadata_is_ready()) {
		            	response_received();
		            	PRINTF("total size = %" PRIu32 "\n", metadata.total_size);
		            	PRINTF("total packages = %" PRIu32 "\n", metadata.total_packages);
		            	CTE_DBG("MD5:\n");
		            	memory_dump(metadata.md5, 16);
		            	if (metadata.total_packages < 2 && metadata.ota_type != OTA_TYPE_EXIT) {//JJR
		                	PRINTF("[BAD META] total packages too few\n");
		                	Terminate(OTA_RESULT_BAD_METADATA);
		            	} else if (!check_packages_amount() && metadata.ota_type != OTA_TYPE_EXIT) {//JJR
		                	PRINTF("[BAD META] total size not match packages\n");
		                	Terminate(OTA_RESULT_BAD_METADATA);
		            	} else {
		                	switch (metadata.ota_type) { // begin switch ota_type
								case OTA_TYPE_BLE: /*JJR*/
								case OTA_TYPE_BT:  /*JJR*/
		                		case OTA_TYPE_MAIN: {
		                    		PRINTF("OTA Update: %d\n",metadata.ota_type);
		                    		if (metadata.total_size > CTE_OTA_MAIN_FW_SIZE_LIMIT) {
		                        		PRINTF("[BAD META] too big\n");
		                        		Terminate(OTA_RESULT_BAD_METADATA);
		                            } else {
		                                stateTransfer(eOTAState_WaitErase); //John_9999 main MCU start point
		                            }
		                    	} break;

		                		case OTA_TYPE_WIFI: {
		                        	PRINTF("OTA Update: WIFI\n");
		                    		if (metadata.total_size > CTE_OTA_WIFI_FW_SIZE_LIMIT) {
		                        		PRINTF("[BAD META] too big\n");
		                        		Terminate(OTA_RESULT_BAD_METADATA);
		                    		} else {
		                        		DA16200MOD_fw_write_address = get_new_firmware_address();
		                        		mbedtls_md5_starts_ret(&MD5_calc);
		                        		req_package_number = 0;
		                        		stateTransfer(eOTAState_WIFI_Programing); //John_9999 wifi start point
		                			}
		                        } break;
		                		case OTA_TYPE_EXIT: {
		                    		PRINTF("Quit OTA\n");
									stateTransfer(eOTAState_DONE); //JJR quit OTA
		                 		} break;

		                		default: {
		                    		PRINTF("Unknown OTA Type = %02X\n", metadata.ota_type);
		                    		Terminate(OTA_RESULT_UNKNOWN_OTA_TYPE);
		                		} break;
		                	} //end switch ota_type
		                }
		            } else {
		            	CTE_DBG_XXX();
		            	CTE_DBG("ghost EV_RECV_METADATA !!\n");
		            }
		        }
		    } break;


			case eOTAState_WaitErase: {
		    	if (ev.type == EV_ENTER) {
		        	write_fw_size();
		        	uint32_t timeout = metadata.total_size / 153 + 10000;
		        	// avoid overflow
		        	if (timeout >= portMAX_DELAY / 1000 - 20000) {
		            	timeout = portMAX_DELAY / 1000 - 20000;
		        	}
		        	CTE_DBG("erase waiting timeout: %" PRIu32 " ms\n", timeout);
		        	set_timeout_event(pdMS_TO_TICKS(timeout + 10000)); //JJR adjust timeout according to firmware size,add 10s margin
		    	} else if (ev.type == EV_ERASE_OK) {
		        	req_package_number = 0;
		        	ack_packages = 0;
					CTE_DBG("JJR ERASEOK\n");//JJR
		        	stateTransfer(eOTAState_MCU_Programing);
		    	} else if (ev.type == EV_ERASE_FAIL) {
		        	Terminate(OTA_RESULT_ERASE_FAIL);
		    	} else if (ev.type == EV_TIMEOUT) {
		        	Terminate(OTA_RESULT_MCU_NO_ANS);
		    	}
			} break;


			case eOTAState_MCU_Programing: {
		    	bool request_data = false;
		    	bool start_nxp_flashing = false;
		    	if (ev.type == EV_ENTER || ev.type == EV_LINK_START) {
		        	if (DPB_is_null(&dpack_buffer)){
		            	request_data = true;
		        	}
		    	} else if (ev.type == EV_RECV_DATAPACK) {
		        	if (req_package_number == DPB_get_body(&ev.data_package_buffer)->data_package_number) {
		            	response_received();
		            	req_package_number++;
		            	if (!DPB_is_null(&dpack_buffer)) {
		                	CTE_DBG_FATAL("OTAutomata: DataPackage Buffer Overrun!!\n");
		                	Terminate(OTA_RESULT_BUFFER_OVERRUN);
		            	} else {
		                	dpack_buffer = DPB_move(&ev.data_package_buffer);
		                	if (req_package_number <= ack_packages + 1) {
		                    	CTE_DBG("write Package.%" PRIu32 "\n", DPB_get_body(&dpack_buffer)->data_package_number);		                    request_data = true;
		                    	start_nxp_flashing = true;
		                	} else {
		                    	CTE_DBG("hold Package.%" PRIu32 "\n", DPB_get_body(&dpack_buffer)->data_package_number);
		                	}
		            	}
		            }
		    	} else if (ev.type == EV_NXP_WRITE_ACK) {
		        	unset_timeout_event();
		        	nfi_next_block(&nxp_flash_writer);
		        	if (nfi_write_flash(&nxp_flash_writer)) {
		            	set_timeout_event(pdMS_TO_TICKS(CTE_OTA_MCU_STUCK_TIMEOUT));
		        	} else {
		            	ack_packages++;
		            	if (ack_packages >= metadata.total_packages) {
		                	stateTransfer(eOTAState_MCU_Verification);
		            	} else if (!DPB_is_null(&dpack_buffer)) {
		                	CTE_DBG_PRINT_TS();
		                	CTE_DBG("resume Package.%" PRIu32 "\n", DPB_get_body(&dpack_buffer)->data_package_number);
		                	request_data = true;
		                	start_nxp_flashing = true;
		            	}
		        	}
		    	} else if (ev.type == EV_NXP_WRITE_ABORT) {
		        	handle_nxp_write_abort(ev.nxp_write_err_code);
		        } else if (ev.type == EV_TIMEOUT) {
		        	Terminate(OTA_RESULT_MCU_NO_ANS);
		    	}

		    	if (request_data) {
		        	if (req_package_number < metadata.total_packages) {
		            	send_RequestData(req_package_number);
		        	}
		    	}

		    	if (start_nxp_flashing) {
		        	nfi_begin(&nxp_flash_writer, DPB_move(&dpack_buffer));
		        	nfi_write_flash(&nxp_flash_writer);
		        	set_timeout_event(pdMS_TO_TICKS(CTE_OTA_MCU_STUCK_TIMEOUT));
		    	}
			} break;


			case eOTAState_MCU_Verification: {
		    	if (ev.type == EV_ENTER) {
		        	set_timeout_event(pdMS_TO_TICKS(CTE_OTA_MCU_STUCK_TIMEOUT));
		    	} else if (ev.type == EV_NXP_WRITE_FINISH) {
		        	acknowledge_finish();
		        	vTaskDelay(pdMS_TO_TICKS(10));
		        	write_MD5();
		        	set_timeout_event(pdMS_TO_TICKS(CTE_OTA_MD5_CALC_WAIT_TIME));
		    	} else if (ev.type == EV_MD5_OK) {
		        	CTE_DBG("DSB Monitor begin.\n");
					if(metadata.ota_type == OTA_TYPE_MAIN) { //JJR
                       CTE_RECOVER_set_flag();
		        	   monitor_dashboard_alive();
					}
		        	stateTransfer(eOTAState_DONE);
		    	} else if (ev.type == EV_MD5_NG) {
		        	Terminate(OTA_RESULT_MD5_ERROR);
		    	} else if (ev.type == EV_NXP_WRITE_ABORT) {
		        	handle_nxp_write_abort(ev.nxp_write_err_code);
		    	} else if (ev.type == EV_TIMEOUT) {
		        	Terminate(OTA_RESULT_MCU_NO_ANS);
		        }
		    } break;


			case eOTAState_WIFI_Programing: {
		    	bool request_data = false;
		    	if (ev.type == EV_ENTER || ev.type == EV_LINK_START) {
		        	request_data = true;
		    	} else if (ev.type == EV_RECV_DATAPACK) {
		        	DataPackageBuffer buf = DPB_move(&ev.data_package_buffer);
		        	if (req_package_number == DPB_get_body(&buf)->data_package_number) {
		            	response_received();
		            	req_package_number++;
		            	CTE_DBG("write DA16200 at 0x%08X\n", DA16200MOD_fw_write_address);
		            	uint8_t *data_pack = DPB_get_body(&buf)->data;
		            	UINT pack_length = DPB_data_length(&buf);
		            	if (TRUE == writeDataToFlash(DA16200MOD_fw_write_address, data_pack, pack_length)) {
		                	mbedtls_md5_update_ret(&MD5_calc, data_pack, pack_length);
		                	DA16200MOD_fw_write_address += pack_length;
		                	request_data = true;
		            	} else {
		                	Terminate(OTA_RESULT_WIFI_FW_CRC);
		            	}
		        	}
		        	release_DataPackageBuffer(&buf);
		    	} else if (ev.type == EV_TIMEOUT) {
		        	Terminate(OTA_RESULT_WIFI_UPDATE_TIMEOUT);
		    	}

		    	if (request_data) {
		        	if (req_package_number < metadata.total_packages) {
		            	send_RequestData(req_package_number);
		            	set_timeout_event(pdMS_TO_TICKS(CTE_OTA_WIFI_DATA_TIMEOUT));
		        	} else {
		            	unsigned char md5_hash[16];
		            	mbedtls_md5_finish_ret(&MD5_calc, md5_hash);
		            	CTE_DBG("calc MD5:\n");
		            	md5dump(md5_hash);
		            	CTE_DBG("recv MD5:\n");
		            	md5dump(metadata.md5);
		            	if (0 != memcmp(md5_hash, metadata.md5, 16)) {
		                	PRINTF("Update DA16200: Transmission Corrupted.\n");
		                	Terminate(OTA_RESULT_MD5_ERROR);
		            	} else if (!ota_update_sflash_crc(get_new_firmware_address())) {
		                	PRINTF("Update DA16200: Bad Firmware/Flash\n");
		                	Terminate(OTA_RESULT_WIFI_FW_CRC);
		            	} else {
		                	PRINTF("Update DA16200: Check OK, current boot_idx = %d\n", get_cur_boot_index());
		                	ota_update_toggle_boot_index();
		                	stateTransfer(eOTAState_DONE);
		            	}
		        	}
		    	}
			} break;

			case eOTAState_DONE: {
		    	if (ev.type == EV_ENTER) {
		        	g_ota_result = OTA_RESULT_DONE;
					#ifdef ENABLE_CTE_DBG
		        		ota_profiling_report();
					#endif
		        	switch (metadata.ota_type) {
		        		case OTA_TYPE_MAIN:
		            		PRINTF("OTAutomata will restart after %d sec...\n", CTE_OTA_RESTART_STATE_MACHINE_DELAY / 1000);
		            		//set_timeout_event(pdMS_TO_TICKS(CTE_OTA_RESTART_STATE_MACHINE_DELAY));//JJR
						    unset_timeout_event();
							send_FIN();//JJR
							init_automata_OneButton();//JJR
							OneButton = true;//JJR
							release_DataPackageBuffer(&dpack_buffer);//JJR
		        	        nfi_cleanup(&nxp_flash_writer);//JJR
							
		            	break;

		        		case OTA_TYPE_WIFI:
		            		PRINTF("DA16200 will reboot after %d sec...\n",  CTE_OTA_RESTART_WIFI_DELAY / 1000);
		            		//set_timeout_event(pdMS_TO_TICKS(CTE_OTA_RESTART_WIFI_DELAY));
							unset_timeout_event();
							send_FIN();//JJR
							init_automata_OneButton();//JJR
							OneButton = true;//JJR
							release_DataPackageBuffer(&dpack_buffer);//JJR
		        	        nfi_cleanup(&nxp_flash_writer);//JJR
		            	break;

		        		case OTA_TYPE_BT://John_9999 reboot BT!
		            		//PRINTF("OTA Update: BT reboot in %d sec\n", CTE_OTA_RESTART_BT_DELAY/1000);
		            		//set_timeout_event(pdMS_TO_TICKS(CTE_OTA_RESTART_BT_DELAY));
							PRINTF("OTA Update: BT reboot in %d sec\n", CTE_OTA_RESTART_STATE_MACHINE_DELAY/1000);
							unset_timeout_event();
							send_FIN();//JJR
							init_automata_OneButton();//JJR
							OneButton = true;//JJR
							release_DataPackageBuffer(&dpack_buffer);//JJR
		        	        nfi_cleanup(&nxp_flash_writer);//JJR
		            	break;

		        		case OTA_TYPE_BLE: //John_9999 reboot BLE!
		            		PRINTF("OTA Update: BLE reboot in %d sec\n", CTE_OTA_RESTART_BLE_DELAY/1000);//JJR
							unset_timeout_event();//JJR
							send_FIN();//JJR
							init_automata_OneButton();//JJR
							OneButton = true;//JJR
							release_DataPackageBuffer(&dpack_buffer);//JJR
		        	        nfi_cleanup(&nxp_flash_writer);//JJR
		            	break;

                        case OTA_TYPE_EXIT: //JJR Quit OTA
		            		PRINTF("OTA Update: Quit OTA in %d sec\n", CTE_OTA_RESTART_BLE_DELAY/1000);
							unset_timeout_event();//JJR
		            		//set_timeout_event(pdMS_TO_TICKS(CTE_OTA_RESTART_BLE_DELAY));
							output_AT_style_async_msg("+METAe00000000");//JJR//for prograss check
							send_DISCON();//JJR
		            	break;
		        		default:
		            		CTE_DBG_FATAL("State DONE: unknown OTA Type %u\n", metadata.ota_type);
		            		//should not happend. restart automata fast.
		            		set_timeout_event(pdMS_TO_TICKS(CTE_OTA_UNKNOW_DELAY));
		        	}
		        	// send_FIN(); JJR
		    	} else if (ev.type == EV_LINK_START) {
		        	g_ota_result = OTA_RESULT_DONE;
		       	 	send_FIN();
		    	} else if (ev.type == EV_TIMEOUT) {
		        	if (metadata.ota_type == OTA_TYPE_WIFI) {
		            	reboot_func(SYS_REBOOT_POR);
					}
		        	init_automata();
		    	}
			} break;


			case eOTAState_Terminated: {
		    	if (ev.type == EV_ENTER) {
		        	send_FIN();
					#ifdef ENABLE_CTE_DBG
		        		ota_profiling_report();
					#else
		        		PRINTF("OTA Update Result = %d\n", g_ota_result);
					#endif

		        	if (g_ota_result == OTA_RESULT_MCU_NO_ANS) {
		            	PRINTF("MCU will reboot soon...\n");
		            	set_timeout_event(pdMS_TO_TICKS(CTE_OTA_MCU_STUCK_REBOOT_DELAY));
		        	}
		    	} else if (ev.type == EV_LINK_START) {
		        	send_FIN();
		    	}	else if (ev.type == EV_TIMEOUT) {
		        	CTE_NXP_BOOTMODE(NXP_BOOT_NORMAL);
		        	CTE_NXP_RESET();
		    	}
			} break;


			default: {
		    	CTE_DBG_FATAL("Unknown OTA Automata State %d\n", g_ota_server_state);
		    	Terminate(OTA_RESULT_DEBUG_2);
			} break;
		}// end switch state
		// cleanup unused data package
		if (ev.type == EV_RECV_DATAPACK){
			release_DataPackageBuffer(&ev.data_package_buffer);
		}
	}// while(1)
}



static void Task_OtaUpdateReceiver(void *args)
{
	DA16X_UNUSED_ARG(args);

	static OTA_packet_header_t header;
	static union {
		OTA_packet_body_AnswerMeta_t AnswerMeta;
		OTA_packet_body_DataPackage_t DataPackage;
	} body;

	while (1)
	{
		int sock;
		if (xQueueReceive(queue_socket, &sock, portMAX_DELAY) == pdTRUE) {
			bool waiting_body = false;
			PacketCollectHelper collector;
			pch_init(&collector);
			ssize_t recv_len;
            #define CLOSE_CONNECTION() recv_len = 0
			pch_expected_length(&collector, &header, sizeof(header));

			do {
				if (pch_receive(&collector, sock, &recv_len)) {
					if (!waiting_body) {
						CTE_DBG_PRINT_TS();
						CTE_DBG("Header:\n");
						memory_dump(&header, sizeof(header));

						header.body_length = ntohs(header.body_length);

						switch (header.ptype) {
							case PTYPE_AnswerMeta:
								if (header.body_length == sizeof(OTA_packet_body_AnswerMeta_t)) {
									pch_expected_length(&collector, &body, header.body_length);
									waiting_body = true;
								} else {
									CTE_DBG("ill-formed AnswerMeta: %u\n", header.body_length);
									CLOSE_CONNECTION();
								}
								break;

							case PTYPE_DataPackage:
								if (header.body_length <= sizeof(OTA_packet_body_DataPackage_t)
									&&  header.body_length >= DATAPACKAGE_BODY_MINIMUM_LENGTH) {
									pch_expected_length(&collector, &body, header.body_length);
									waiting_body = true;
								}	else {
									CTE_DBG("ill-formed DataPackage: %u\n", header.body_length);
									CLOSE_CONNECTION();
								}
								break;

							default://including receive PTYPE_DISCON and send close connection/*JJR*/
								// unknown or unavailable PTYPE, close connection
								CTE_DBG("invalid PTYPE: %02Xh\n", header.ptype);
								CLOSE_CONNECTION();
						}// end switch ptype
					} else {
						waiting_body = false;
						pch_expected_length(&collector, &header, sizeof(header));
						// Check OTA Type
						if (metadata_is_ready() && metadata.ota_type != header.ota_type) {
							CTE_DBG("expected OTA_Type=%u, but %u\n", metadata.ota_type, header.ota_type);
							OTAutomata_event_inconsistent_ota_type();
						} else {
							//收到完整的body了，進行endian轉換並印出一些資訊
							switch (header.ptype) {
								case PTYPE_AnswerMeta:
									CTE_DBG("AnswerMeta:\n");
									memory_dump(&body.AnswerMeta, sizeof(OTA_packet_body_AnswerMeta_t));
									CTE_DBG("JJR total_size = 0x%08" PRIX32 "\n", body.AnswerMeta.total_size);
									write_ota_update_metadata(
										header.ota_type,
										ntohl(body.AnswerMeta.total_size),
										ntohl(body.AnswerMeta.total_data_packages),
										body.AnswerMeta.md5
									);
									OTAutomata_event_recv_metadata();
								break;

								case PTYPE_DataPackage: {
									body.DataPackage.data_package_number = ntohl(body.DataPackage.data_package_number);
									CTE_DBG("DataPackage %" PRIu32 ", len=%" PRIu32 "\n",
									body.DataPackage.data_package_number,
									header.body_length - DATAPACKAGE_DATA_OFFSET
									);

									DataPackageBuffer buf = allocate_DataPackageBuffer();
									if (!DPB_is_null(&buf)) {
										DPB_set_data_length(&buf, header.body_length - DATAPACKAGE_DATA_OFFSET);
										OTA_packet_body_DataPackage_t *dp = DPB_get_body(&buf);
										memcpy(dp, &body.DataPackage, sizeof(OTA_packet_body_DataPackage_t));
										OTAutomata_event_recv_datapack(DPB_move(&buf));
									} else {
										CTE_DBG_FATAL("No memory for DataPackage!\n");
										// run out of memory, just discard DataPackage
									}
								}
								break;

								default:
									CTE_DBG_FATAL("%s(): unknown PType %02X\n", __func__, header.ptype);
							}
						}
					}
				}
			} while (recv_len > 0);

			if (recv_len < 0)
			{
				CTE_DBG("TCP abort: %d\n", errno);
			}
			else
			{
				CTE_DBG("TCP closed.\n");
			}			
		}
	}
#undef CLOSE_CONNECTION
}


static int init_CTE_OTA(void) //John_9999 OTA initial
{
#define ALLOCATION_CHECK(object) do { \
	if (object == NULL) { \
		CTE_DBG_FATAL("Allocate " #object " failed\n"); \
		return -1; \
	} \
} while(0)

	initialize_ota_update_metadata();
	initialize_DataPackageBuffer_pool();

	keep_alive_timer = xTimerCreate(
			"KEEPALIVE",
			pdMS_TO_TICKS(CTE_OTA_KEEPALIVE_INTERVAL),
			pdFALSE,
			NULL,
			OTAutomata_event_keepalive_timeout);
	ALLOCATION_CHECK(keep_alive_timer);

	queue_socket = xQueueCreate(1, sizeof(int));
	ALLOCATION_CHECK(queue_socket);

	event_queue = xQueueCreate(64, sizeof(Event_t));
	ALLOCATION_CHECK(event_queue);

	TaskHandle_t OtaUpdateAutomata;
	xTaskCreate(
			Task_OtaUpdateAutomata,
			"OTA FSM",
			768,
			NULL,
			uxTaskPriorityGet(NULL) + 2,
			&OtaUpdateAutomata
		);
	ALLOCATION_CHECK(OtaUpdateAutomata);

	TaskHandle_t OtaUpdateReceiver;
	xTaskCreate(
			Task_OtaUpdateReceiver,
			"OTA RECV",
			512,
			NULL,
			uxTaskPriorityGet(NULL) + 1,
			&OtaUpdateReceiver
		);
	ALLOCATION_CHECK(OtaUpdateReceiver);


	int sock_server = socket(PF_INET, SOCK_STREAM, 0);
	if (sock_server < 0)
	{
		CTE_DBG_FATAL("socket() error: %d\n", errno);
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


void UserApp_CTE_OTA(void *args) //John_9999
{
	DA16X_UNUSED_ARG(args);

	int sock_server;
	if ((sock_server = init_CTE_OTA()) >= 0)
	{
		TickType_t last_connection = 0;

		while (1)
		{
			int fd = accept(sock_server, NULL, NULL);

			if (fd < 0) //error status
			{
				//我也不知道什麼狀況會進到這裡啊，加個500ms delay好了.
				vTaskDelay(pdMS_TO_TICKS(500));
				continue;
			}

			if (xTaskGetTickCount() - last_connection < pdMS_TO_TICKS(CTE_OTA_TCP_ACCEPT_COOLDOWN))
			{
				CTE_DBG("Reject Connection: in accept cool down\n");
				vTaskDelay(pdMS_TO_TICKS(CTE_OTA_TCP_ACCEPT_COOLDOWN * 6 / 10));
				closesocket(fd);
			}
			else
			{
				CTE_DBG("new Network Connection\n");
				last_connection = xTaskGetTickCount();
				OTAutomata_event_TCP_connected(fd);
			}
		}
	}

	while (1)
	{
		vTaskSuspend(NULL);
	}
}
