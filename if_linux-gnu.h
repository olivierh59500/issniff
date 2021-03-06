/*
 * Interface type prefixes and their link-level packet header sizes.
 */
static struct {
  const char *type;
  int hdr_len;
} if_types[] = {
  { "eth", sizeof (struct ethhdr) },
  { "em", sizeof (struct ethhdr) },
  { "br", sizeof (struct ethhdr) },
  { "wlan", sizeof (struct ethhdr) },
  { "tun", 0 },
  { "ppp", 0 },
  { "sl", 0 },
  { "lo", sizeof (struct ethhdr) },
  { "dummy", sizeof (struct ethhdr) },
  { NULL }
};
