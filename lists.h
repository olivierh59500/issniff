/* $Id$ */

/*
 * Primary data structures.
 */
/* Connection data. */
typedef struct PList {
  struct PList *next, *prev;
  ADDR_T daddr, saddr;
  PORT_T dport, sport;		/* dport redundant, but saves some arg passes */
  UDATA *data;			/* 2:1 byte ratio for data; gives direction. */
  UINT dlen, pkts[2];
  time_t stime, timeout;
  int caught_syn;
} PList;

/* Pseudo-hash. */
typedef struct Ports {
  short int port;
  short int twoway;
  PList *next;
} Ports;

enum { pkt_to, pkt_from };
enum { with_syn, without_syn, first_fin };

/*
 * Major functionality is provided by these macros.
 */
#define EXPAND_CACHE { \
  UCHAR *blk = NULL; \
  int i; \
  PList *cnode = cache; \
  if (!(blk = (UCHAR *)malloc ((sizeof (PList) + sizeof (UDATA) * maxdata) * \
			       cache_increment))) { \
    perror ("** malloc"); 	/* Not fatal, though recovery is untested. */ \
  } else { \
    for (i = 0; i < cache_increment; i++, blk += sizeof (UDATA) * maxdata) { \
      cnode->next = (PList *)blk; \
      cnode->next->data = (UDATA *)(blk += sizeof (PList)); \
      cnode = cnode->next; \
    } \
    cache_max += cache_increment; \
    cache_size += cache_increment; \
  } \
}

#define END_NODE(NODE, PORT, REASON) { \
  dump_node ((NODE), (REASON)); \
  if ((NODE)->next) { \
    (NODE)->next->prev = (NODE)->prev; \
  } \
  if ((NODE)->prev) { \
    (NODE)->prev->next = (NODE)->next; \
  } else { \
    ports[(PORT)].next = (NODE)->next; \
  } \
  (NODE)->next = cache->next; \
  cache->next = (NODE); \
  ++cache_size; \
  --curr_conn; \
}

#define ADD_DATA(NODE, BUF, IPH, TCPH, SHIFT) { \
  int i = 0; \
  int blen = ntohs (IPLEN((IPH))) - IPHLEN((IPH)) - DOFF((TCPH)); \
  int todo = ((NODE)->dlen + blen > maxdata) ? maxdata - (NODE)->dlen : blen; \
  while (i < todo) { \
    (NODE)->data[(NODE)->dlen + i] = (UDATA)((BUF)[i++]) << (SHIFT); \
  } \
  (NODE)->dlen += todo; \
  if ((NODE)->dlen == maxdata) { \
    ++stats[s_maxdata]; \
    END_NODE ((NODE), (NODE)->dport, "MAXDATA"); \
  } else { \
    time (&(NODE)->timeout); \
  } \
}

#define ADD_NODE(DPORT, DADDR, SPORT, SADDR, HAS_SYN, BUF, IPH, TCPH, SHIFT) { \
  PList *new = NULL; \
  if (!cache_size) { \
    EXPAND_CACHE; \
  } \
  if (cache_size) { \
    new = cache->next; \
    cache->next = cache->next->next; \
    --cache_size; \
    ++curr_conn; \
    new->prev = NULL; \
    new->daddr = (DADDR); \
    new->saddr = (SADDR); \
    new->dport = (DPORT); \
    new->sport = (SPORT); \
    new->pkts[pkt_to] = 1; \
    new->pkts[pkt_from] = 0; \
    new->dlen = 0; \
    new->caught_syn = (HAS_SYN); \
    memset (new->data, 0, sizeof (UDATA) * maxdata); \
    time (&new->stime); \
    new->timeout = new->stime; \
    if (!ports[(DPORT)].next) { \
      new->next = NULL; \
      ports[(DPORT)].next = new; \
    } else { \
      ports[(DPORT)].next->prev = new; \
      new->next = ports[(DPORT)].next; \
      ports[(DPORT)].next = new; \
    } \
    ADD_DATA (new, (BUF), (IPH), (TCPH), (SHIFT)); \
  } else { \
    MENTION (DPORT, DADDR, SPORT, SADDR, "No memory; NOT MONITORING"); \
  } \
}

#define MENTION(DPORT, DADDR, SPORT, SADDR, MSG) { \
  struct in_addr ia; \
  ia.s_addr = (SADDR); \
  fprintf (stderr, "*  %s: %s:%d %s ", (MSG), inet_ntoa (ia), (SPORT), \
	   ports[(DPORT)].twoway ? "<->" : "->"); \
  ia.s_addr = (DADDR); \
  fprintf (stderr, "%s:%d\n", inet_ntoa (ia), (DPORT)); \
}
