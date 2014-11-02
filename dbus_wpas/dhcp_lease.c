#include <stdio.h>
#include <string.h>
#include <glib.h>

enum {
  STATE_FIND_LEASE,
  STATE_PARSE_LEASE,
  STATE_FINISH
};

#define STR_LEASE   "lease"
#define STR_COMMENT "#"

const char* mac_to_ip(const char* lease_file, const char* mac)
{
  g_debug("mac_to_ip(%s, %s)", lease_file, mac);
  g_assert(NULL != lease_file);
  g_assert(NULL != mac);

  FILE* fp = fopen(lease_file, "r");
  if (NULL == fp)
  {
    g_error("Failed to open file %s.", lease_file);
    return NULL;
  }

  static char ip[20];
  static char line[1024];
  char* result = NULL;
  int state = STATE_FIND_LEASE;

  while(STATE_FINISH != state)
  {
    if (NULL == fgets(line, sizeof(line), fp))
    {
      g_debug("EOF");
      state = STATE_FINISH;
      break;
    }
    if (line[0] == '#') continue;
    if (strlen(line) == 0) continue;
    if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';
    g_debug("Examine line: \'%s\'", line);
    if (state == STATE_FIND_LEASE)
    {
      if (strstr(line, "lease") == line)
      {
        g_debug("Lease start found.");
        char* ip_begin = strchr(line, ' ');
        g_assert(NULL != ip_begin);
        ip_begin += 1;
        char* ip_end = strchr(ip_begin, ' ');
        g_assert(NULL != ip_end);
        strncpy(ip, ip_begin, ip_end-ip_begin);
        g_debug("Ip address %s.", ip);
        state = STATE_PARSE_LEASE;
      }
    }
    else if (state == STATE_PARSE_LEASE)
    {
      static const char* mac_key = "hardware ethernet";
      char* key_start = strstr(line, mac_key); 
      if (NULL != key_start)
      {
        g_debug("Scanning line for MAC address.");
        char* mac_start = key_start + strlen(mac_key) + 1;
        g_assert(strlen(line) > mac_start - line);
        char* mac_end = strchr(mac_start, ';');
        g_assert(NULL != mac_end);
        mac_end[0] = '\0';
        g_debug("Found MAC: %s.", mac_start);
        if (strcmp(mac_start, mac) == 0)
        {
          g_debug("MAC match.");
          result = ip;
          state = STATE_FINISH;
        }
      }
      else if (line[0] == '}')
      {
        g_debug("Lease end found.");
        state = STATE_FIND_LEASE;
      }
    }
  }

  fclose(fp);
  return result;
}
