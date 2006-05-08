/*
 *	The PCI Library -- ID to Name Translation
 *
 *	Copyright (c) 1997--2005 Martin Mares <mj@ucw.cz>
 *
 *	Modified and ported to HelenOS by Jakub Jermar.
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "internal.h"
#include "pci_ids.h"

struct id_entry {
  struct id_entry *next;
  u32 id12, id34;
  byte cat;
  byte name[1];
};

enum id_entry_type {
  ID_UNKNOWN,
  ID_VENDOR,
  ID_DEVICE,
  ID_SUBSYSTEM,
  ID_GEN_SUBSYSTEM,
  ID_CLASS,
  ID_SUBCLASS,
  ID_PROGIF
};

struct id_bucket {
  struct id_bucket *next;
  unsigned int full;
};

#define MAX_LINE 1024
#define BUCKET_SIZE 8192
#define HASH_SIZE 4099

#ifdef __GNUC__
#define BUCKET_ALIGNMENT __alignof__(struct id_bucket)
#else
union id_align {
  struct id_bucket *next;
  unsigned int full;
};
#define BUCKET_ALIGNMENT sizeof(union id_align)
#endif
#define BUCKET_ALIGN(n) ((n)+BUCKET_ALIGNMENT-(n)%BUCKET_ALIGNMENT)

static void *id_alloc(struct pci_access *a, unsigned int size)
{
  struct id_bucket *buck = a->current_id_bucket;
  unsigned int pos;
  if (!buck || buck->full + size > BUCKET_SIZE)
    {
      buck = pci_malloc(a, BUCKET_SIZE);
      buck->next = a->current_id_bucket;
      a->current_id_bucket = buck;
      buck->full = BUCKET_ALIGN(sizeof(struct id_bucket));
    }
  pos = buck->full;
  buck->full = BUCKET_ALIGN(buck->full + size);
  return (byte *)buck + pos;
}

static inline u32 id_pair(unsigned int x, unsigned int y)
{
  return ((x << 16) | y);
}

static inline unsigned int id_hash(int cat, u32 id12, u32 id34)
{
  unsigned int h;

  h = id12 ^ (id34 << 3) ^ (cat << 5);
  return h % HASH_SIZE;
}

static struct id_entry *id_lookup(struct pci_access *a, int cat, int id1, int id2, int id3, int id4)
{
  struct id_entry *n;
  u32 id12 = id_pair(id1, id2);
  u32 id34 = id_pair(id3, id4);

  n = a->id_hash[id_hash(cat, id12, id34)];
  while (n && (n->id12 != id12 || n->id34 != id34 || n->cat != cat))
    n = n->next;
  return n;
}

static int id_insert(struct pci_access *a, int cat, int id1, int id2, int id3, int id4, byte *text)
{
  u32 id12 = id_pair(id1, id2);
  u32 id34 = id_pair(id3, id4);
  unsigned int h = id_hash(cat, id12, id34);
  struct id_entry *n = a->id_hash[h];
  int len = strlen((char *) text);

  while (n && (n->id12 != id12 || n->id34 != id34 || n->cat != cat))
    n = n->next;
  if (n)
    return 1;
  n = id_alloc(a, sizeof(struct id_entry) + len);
  n->id12 = id12;
  n->id34 = id34;
  n->cat = cat;
  memcpy(n->name, text, len+1);
  n->next = a->id_hash[h];
  a->id_hash[h] = n;
  return 0;
}

static int id_hex(byte *p, int cnt)
{
  int x = 0;
  while (cnt--)
    {
      x <<= 4;
      if (*p >= '0' && *p <= '9')
	x += (*p - '0');
      else if (*p >= 'a' && *p <= 'f')
	x += (*p - 'a' + 10);
      else if (*p >= 'A' && *p <= 'F')
	x += (*p - 'A' + 10);
      else
	return -1;
      p++;
    }
  return x;
}

static inline int id_white_p(int c)
{
  return (c == ' ') || (c == '\t');
}

static const char *id_parse_list(struct pci_access *a, int *lino)
{
  byte *line;
  byte *p;
  int id1=0, id2=0, id3=0, id4=0;
  int cat = -1;
  int nest;
  static const char parse_error[] = "Parse error";
  int i;

  *lino = 0;
  for (i = 0; i < sizeof(pci_ids)/sizeof(char *); i++) {
      line = (byte *) pci_ids[i];
      (*lino)++;
      p = line;
      while (*p)
	p++;
      if (p > line && (p[-1] == ' ' || p[-1] == '\t'))
	*--p = 0;

      p = line;
      while (id_white_p(*p))
	p++;
      if (!*p || *p == '#')
	continue;

      p = line;
      while (*p == '\t')
	p++;
      nest = p - line;

      if (!nest)					/* Top-level entries */
	{
	  if (p[0] == 'C' && p[1] == ' ')		/* Class block */
	    {
	      if ((id1 = id_hex(p+2, 2)) < 0 || !id_white_p(p[4]))
		return parse_error;
	      cat = ID_CLASS;
	      p += 5;
	    }
	  else if (p[0] == 'S' && p[1] == ' ')
	    {						/* Generic subsystem block */
	      if ((id1 = id_hex(p+2, 4)) < 0 || p[6])
		return parse_error;
	      if (!id_lookup(a, ID_VENDOR, id1, 0, 0, 0))
		return "Vendor does not exist";
	      cat = ID_GEN_SUBSYSTEM;
	      continue;
	    }
	  else if (p[0] >= 'A' && p[0] <= 'Z' && p[1] == ' ')
	    {						/* Unrecognized block (RFU) */
	      cat = ID_UNKNOWN;
	      continue;
	    }
	  else						/* Vendor ID */
	    {
	      if ((id1 = id_hex(p, 4)) < 0 || !id_white_p(p[4]))
		return parse_error;
	      cat = ID_VENDOR;
	      p += 5;
	    }
	  id2 = id3 = id4 = 0;
	}
      else if (cat == ID_UNKNOWN)			/* Nested entries in RFU blocks are skipped */
	continue;
      else if (nest == 1)				/* Nesting level 1 */
	switch (cat)
	  {
	  case ID_VENDOR:
	  case ID_DEVICE:
	  case ID_SUBSYSTEM:
	    if ((id2 = id_hex(p, 4)) < 0 || !id_white_p(p[4]))
	      return parse_error;
	    p += 5;
	    cat = ID_DEVICE;
	    id3 = id4 = 0;
	    break;
	  case ID_GEN_SUBSYSTEM:
	    if ((id2 = id_hex(p, 4)) < 0 || !id_white_p(p[4]))
	      return parse_error;
	    p += 5;
	    id3 = id4 = 0;
	    break;
	  case ID_CLASS:
	  case ID_SUBCLASS:
	  case ID_PROGIF:
	    if ((id2 = id_hex(p, 2)) < 0 || !id_white_p(p[2]))
	      return parse_error;
	    p += 3;
	    cat = ID_SUBCLASS;
	    id3 = id4 = 0;
	    break;
	  default:
	    return parse_error;
	  }
      else if (nest == 2)				/* Nesting level 2 */
	switch (cat)
	  {
	  case ID_DEVICE:
	  case ID_SUBSYSTEM:
	    if ((id3 = id_hex(p, 4)) < 0 || !id_white_p(p[4]) || (id4 = id_hex(p+5, 4)) < 0 || !id_white_p(p[9]))
	      return parse_error;
	    p += 10;
	    cat = ID_SUBSYSTEM;
	    break;
	  case ID_CLASS:
	  case ID_SUBCLASS:
	  case ID_PROGIF:
	    if ((id3 = id_hex(p, 2)) < 0 || !id_white_p(p[2]))
	      return parse_error;
	    p += 3;
	    cat = ID_PROGIF;
	    id4 = 0;
	    break;
	  default:
	    return parse_error;
	  }
      else						/* Nesting level 3 or more */
	return parse_error;
      while (id_white_p(*p))
	p++;
      if (!*p)
	return parse_error;
      if (id_insert(a, cat, id1, id2, id3, id4, p))
	return "Duplicate entry";
    }
  return NULL;
}

int
pci_load_name_list(struct pci_access *a)
{
  int lino;
  const char *err;

  pci_free_name_list(a);
  a->id_hash = pci_malloc(a, sizeof(struct id_entry *) * HASH_SIZE);
  bzero(a->id_hash, sizeof(struct id_entry *) * HASH_SIZE);
  err = id_parse_list(a, &lino);
  if (err)
    a->error("%s at %s, element %d\n", err, "pci_ids.h", lino);
  return 1;
}

void
pci_free_name_list(struct pci_access *a)
{
  pci_mfree(a->id_hash);
  a->id_hash = NULL;
  while (a->current_id_bucket)
    {
      struct id_bucket *buck = a->current_id_bucket;
      a->current_id_bucket = buck->next;
      pci_mfree(buck);
    }
}

static struct id_entry *id_lookup_subsys(struct pci_access *a, int iv, int id, int isv, int isd)
{
  struct id_entry *d = NULL;
  if (iv > 0 && id > 0)						/* Per-device lookup */
    d = id_lookup(a, ID_SUBSYSTEM, iv, id, isv, isd);
  if (!d)							/* Generic lookup */
    d = id_lookup(a, ID_GEN_SUBSYSTEM, isv, isd, 0, 0);
  if (!d && iv == isv && id == isd)				/* Check for subsystem == device */
    d = id_lookup(a, ID_DEVICE, iv, id, 0, 0);
  return d;
}

char *
pci_lookup_name(struct pci_access *a, char *buf, int size, int flags, ...)
{
  va_list args;
  int num, res, synth;
  struct id_entry *v, *d, *cls, *pif;
  int iv, id, isv, isd, icls, ipif;

  va_start(args, flags);

  num = 0;
  if ((flags & PCI_LOOKUP_NUMERIC) || a->numeric_ids)
    {
      flags &= ~PCI_LOOKUP_NUMERIC;
      num = 1;
    }
  else if (!a->id_hash)
    {
      if (!pci_load_name_list(a))
	num = a->numeric_ids = 1;
    }

  if (flags & PCI_LOOKUP_NO_NUMBERS)
    {
      flags &= ~PCI_LOOKUP_NO_NUMBERS;
      synth = 0;
      if (num)
	return NULL;
    }
  else
    synth = 1;

  switch (flags)
    {
    case PCI_LOOKUP_VENDOR:
      iv = va_arg(args, int);
      if (num)
	res = snprintf(buf, size, "%04x", iv);
      else if (v = id_lookup(a, ID_VENDOR, iv, 0, 0, 0))
	return (char *) v->name;
      else
	res = snprintf(buf, size, "Unknown vendor %04x", iv);
      break;
    case PCI_LOOKUP_DEVICE:
      iv = va_arg(args, int);
      id = va_arg(args, int);
      if (num)
	res = snprintf(buf, size, "%04x", id);
      else if (d = id_lookup(a, ID_DEVICE, iv, id, 0, 0))
	return (char *) d->name;
      else if (synth)
	res = snprintf(buf, size, "Unknown device %04x", id);
      else
	return NULL;
      break;
    case PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE:
      iv = va_arg(args, int);
      id = va_arg(args, int);
      if (num)
	res = snprintf(buf, size, "%04x:%04x", iv, id);
      else
	{
	  v = id_lookup(a, ID_VENDOR, iv, 0, 0, 0);
	  d = id_lookup(a, ID_DEVICE, iv, id, 0, 0);
	  if (v && d)
	    res = snprintf(buf, size, "%s %s", v->name, d->name);
	  else if (!synth)
	    return NULL;
	  else if (!v)
	    res = snprintf(buf, size, "Unknown device %04x:%04x", iv, id);
	  else	/* !d */
	    res = snprintf(buf, size, "%s Unknown device %04x", v->name, id);
	}
      break;
    case PCI_LOOKUP_SUBSYSTEM | PCI_LOOKUP_VENDOR:
      isv = va_arg(args, int);
      if (num)
	res = snprintf(buf, size, "%04x", isv);
      else if (v = id_lookup(a, ID_VENDOR, isv, 0, 0, 0))
	return (char *) v->name;
      else if (synth)
	res = snprintf(buf, size, "Unknown vendor %04x", isv);
      else
	return NULL;
      break;
    case PCI_LOOKUP_SUBSYSTEM | PCI_LOOKUP_DEVICE:
      iv = va_arg(args, int);
      id = va_arg(args, int);
      isv = va_arg(args, int);
      isd = va_arg(args, int);
      if (num)
	res = snprintf(buf, size, "%04x", isd);
      else if (d = id_lookup_subsys(a, iv, id, isv, isd))
	return (char *) d->name;
      else if (synth)
	res = snprintf(buf, size, "Unknown device %04x", isd);
      else
	return NULL;
      break;
    case PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE | PCI_LOOKUP_SUBSYSTEM:
      iv = va_arg(args, int);
      id = va_arg(args, int);
      isv = va_arg(args, int);
      isd = va_arg(args, int);
      if (num)
	res = snprintf(buf, size, "%04x:%04x", isv, isd);
      else
	{
	  v = id_lookup(a, ID_VENDOR, isv, 0, 0, 0);
	  d = id_lookup_subsys(a, iv, id, isv, isd);
	  if (v && d)
	    res = snprintf(buf, size, "%s %s", v->name, d->name);
	  else if (!synth)
	    return NULL;
	  else if (!v)
	    res = snprintf(buf, size, "Unknown device %04x:%04x", isv, isd);
	  else /* !d */
	    res = snprintf(buf, size, "%s Unknown device %04x", v->name, isd);
	}
      break;
    case PCI_LOOKUP_CLASS:
      icls = va_arg(args, int);
      if (num)
	res = snprintf(buf, size, "%04x", icls);
      else if (cls = id_lookup(a, ID_SUBCLASS, icls >> 8, icls & 0xff, 0, 0))
	return (char *) cls->name;
      else if (cls = id_lookup(a, ID_CLASS, icls, 0, 0, 0))
	res = snprintf(buf, size, "%s [%04x]", cls->name, icls);
      else if (synth)
	res = snprintf(buf, size, "Class %04x", icls);
      else
	return NULL;
      break;
    case PCI_LOOKUP_PROGIF:
      icls = va_arg(args, int);
      ipif = va_arg(args, int);
      if (num)
	res = snprintf(buf, size, "%02x", ipif);
      else if (pif = id_lookup(a, ID_PROGIF, icls >> 8, icls & 0xff, ipif, 0))
	return (char *) pif->name;
      else if (icls == 0x0101 && !(ipif & 0x70))
	{
	  /* IDE controllers have complex prog-if semantics */
	  res = snprintf(buf, size, "%s%s%s%s%s",
			 (ipif & 0x80) ? "Master " : "",
			 (ipif & 0x08) ? "SecP " : "",
			 (ipif & 0x04) ? "SecO " : "",
			 (ipif & 0x02) ? "PriP " : "",
			 (ipif & 0x01) ? "PriO " : "");
	  if (res > 0 && res < size)
	    buf[--res] = 0;
	}
      else if (synth)
	res = snprintf(buf, size, "ProgIf %02x", ipif);
      else
	return NULL;
      break;
    default:
      return "<pci_lookup_name: invalid request>";
    }
  if (res < 0 || res >= size)
    return "<pci_lookup_name: buffer too small>";
  else
    return buf;
}
