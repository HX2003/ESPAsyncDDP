#ifndef _ESPAsyncDDP_H_
#define _ESPAsyncDDP_H_

#include <WiFi.h>
#include <AsyncUDP.h>
#include <vector>

#define DDP_ID_DISPLAY 1
#define DDP_ID_CONTROL 246
#define DDP_ID_CONFIG 250
#define DDP_ID_STATUS 251
#define DDP_ID_DMX 254

#define DPP_FLAGS1_VER 0xC0 ///< @brief version mask

#define DPP_FLAGS1_VER1 0x40 ///< @brief version = 1
#define DPP_FLAGS1_PUSH 0x01
#define DPP_FLAGS1_QUERY 0x02
#define DPP_FLAGS1_REPLY 0x04
#define DPP_FLAGS1_STORAGE 0x08
#define DPP_FLAGS1_TIME 0x10

struct ddp_header
{
  uint8_t flags1;
  uint8_t flags2;
  uint8_t type;
  uint8_t id;
  uint8_t offset1; ///< @brief MSB
  uint8_t offset2;
  uint8_t offset3;
  uint8_t offset4;
  uint8_t len1; ///< @brief MSB
  uint8_t len2;
} __attribute__((packed));

struct endpoint
{
  IPAddress ip;
  uint32_t port;
};

struct discoverer
{
  endpoint sender;
  ddp_header my_ddp_header;
  uint8_t delay;    ///< @brief Delay between when packet was received and when we reply
  time_t timestamp; ///< @brief Time when the packet was received
  bool in_use;
};

class ESPAsyncDDP
{
public:
  using query_callback_t = std::function<std::vector<uint8_t>(ddp_header my_ddp_header)>;
  using write_callback_t = std::function<void(ddp_header my_ddp_header, std::vector<uint8_t> &)>;

  ESPAsyncDDP();
  ~ESPAsyncDDP();

  /**
   * @brief Does necessary setup
   * @return True for successful setup
   */
  bool begin();

  /**
   * @brief Closes UDP connection, call begin to start listening again
   */
  void close();

  /**
   * @brief This method should be called regularly.
   */
  void handle();

  void register_query_cb(query_callback_t callback);
  void unregister_query_cb();

  void register_write_cb(write_callback_t callback);
  void unregister_write_cb();

private:
  AsyncUDP _udp;
  discoverer _discoverer; ///< @brief Just handle the case of 1 discoverer

  query_callback_t _query_callback;
  write_callback_t _write_callback;

  uint8_t _sequence = 0;

  void parse_packet(AsyncUDPPacket packet);

  /**
   * @param my_ddp_header ddp_header struct
   * @param sender endpoint struct which holds address and port of sender we are going to reply to
   */
  void process_query(ddp_header my_ddp_header, endpoint sender);
};

#endif
