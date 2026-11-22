/* Seam: packet intake (espnow_link.c). Expected values come from the design doc:
 * packet is 27 bytes; team ID 0xEE14 (GLOVE_MAGIC); version 1; newest packet wins one slot. */
#include <string.h>
#include "tu.h"
#include "receiver_config.h"
#include "espnow_link.h"

static GlovePacket packet(uint32_t seq, float pitch) {
  GlovePacket g; memset(&g, 0, sizeof g);
  g.magic = 0xEE14; g.version = 1; g.finger_count = 5; g.seq = seq; g.pitch_deg = pitch;
  return g;
}

int main(void) {
  link_stats st;
  GlovePacket out;

  link_begin(false);
  CHECK(!packet_fetch(&out), "empty slot: packet_fetch returns false");

  GlovePacket ok = packet(1, 12.5f);
  link_accept((const uint8_t *)&ok, 27);
  CHECK(packet_fetch(&out) && out.seq == 1 && out.pitch_deg == 12.5f, "a valid 27-byte packet is delivered");
  CHECK(!packet_fetch(&out), "a delivered packet is not delivered twice");

  link_begin(false);
  GlovePacket ok2 = packet(2, 0.0f);
  link_accept((const uint8_t *)&ok2, 26);
  CHECK(!packet_fetch(&out), "26 bytes is dropped");
  link_accept((const uint8_t *)&ok2, 28);
  CHECK(!packet_fetch(&out), "28 bytes is dropped");
  link_accept(NULL, 27);
  CHECK(!packet_fetch(&out), "NULL data is dropped");

  GlovePacket badmagic = packet(3, 0.0f); badmagic.magic = 0x1234;
  link_accept((const uint8_t *)&badmagic, 27);
  CHECK(!packet_fetch(&out), "wrong team ID 0x1234 is dropped");

  GlovePacket badver = packet(4, 0.0f); badver.version = 2;
  link_accept((const uint8_t *)&badver, 27);
  CHECK(!packet_fetch(&out), "version 2 is dropped (only version 1 is accepted)");

  link_stats_tick(&st);
  CHECK(st.received == 1 && st.rejected == 5, "counters: 1 received, 5 rejected so far (26 B, 28 B, NULL, bad team ID, bad version)");

  GlovePacket a = packet(10, 1.0f), b = packet(11, 2.0f);
  link_accept((const uint8_t *)&a, 27);
  link_accept((const uint8_t *)&b, 27);
  CHECK(packet_fetch(&out) && out.seq == 11, "two packets before a fetch: the newest one is kept");
  CHECK(!packet_fetch(&out), "after that fetch the slot is empty");

  link_stats_tick(&st);
  CHECK(st.received == 3 && st.rejected == 5, "counters: 3 received, 5 rejected in total");

  /* seq gaps: so far seq 1, 10, 11 were accepted -> 8 missing (2..9) */
  link_stats_tick(&st);
  CHECK(st.gaps == 8, "gaps: seq 1 then 10 counts 8 missing numbers");
  GlovePacket s12 = packet(12, 0.0f), s15 = packet(15, 0.0f), s3 = packet(3, 0.0f), s4 = packet(4, 0.0f);
  link_accept((const uint8_t *)&s12, 27);
  link_accept((const uint8_t *)&s15, 27);
  link_stats_tick(&st);
  CHECK(st.gaps == 10, "gaps: 12 then 15 adds 2 (13, 14)");
  link_accept((const uint8_t *)&s3, 27);
  link_accept((const uint8_t *)&s4, 27);
  link_stats_tick(&st);
  CHECK(st.gaps == 10, "a backward seq (15 -> 3) is a glove restart, not a gap; 3 -> 4 adds none");
  CHECK(st.received == 7, "counters: 7 received");
  packet_fetch(&out);

  /* IMU-failed flag: packet_take drops it; a good packet after it is delivered */
  GlovePacket bad_imu = packet(5, 0.0f); bad_imu.flags = GLOVE_FLAG_IMU_FAILED;
  link_accept((const uint8_t *)&bad_imu, 27);
  CHECK(!packet_take(&out), "packet_take drops a packet with the IMU-failed flag");
  link_accept((const uint8_t *)&bad_imu, 27);
  CHECK(!packet_take(&out), "a second IMU-failed packet is dropped too");
  GlovePacket good = packet(6, 7.0f);
  link_accept((const uint8_t *)&good, 27);
  CHECK(packet_take(&out) && out.seq == 6, "packet_take delivers a good packet after IMU failures");
  CHECK(!packet_take(&out), "packet_take on an empty slot returns false");

  TU_RESULT();
}
